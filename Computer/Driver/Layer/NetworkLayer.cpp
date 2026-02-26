#include "NetworkLayer.h"

#include "../NetworkDriver.h"
#include "../../../General/Configuration.h"

#include <fstream>
#include <map>
#include <sstream>

NetworkLayer::NetworkLayer(NetworkDriver* driver, const Configuration& config)
    : m_driver(driver),
      m_receivingQueue(
          config.get(Configuration::NETWORK_LAYER_RECEIVING_BUFFER_SIZE)),
      m_sendingQueue(
          config.get(Configuration::NETWORK_LAYER_SENDING_BUFFER_SIZE)),
      m_packetSize(config.get(Configuration::NETWORK_LAYER_DATA_SIZE)),
      m_executeReceiving(false), m_executeSending(false),
      m_currentlySendingFile(false), m_address(config) {}

NetworkLayer::~NetworkLayer() {
    stop();
}

void NetworkLayer::start() {
    stop();
    startListening();
}

void NetworkLayer::stop() {
    stopSending();
    stopListening();
}

unsigned int NetworkLayer::receivedFileCount() const {
    return m_receivedFileCount;
}

void NetworkLayer::sendToLinkLayer(const Packet& data) {
    // Attente active pour pouvoir continuer d'envoyer. Si le buffer d'envoi est
    // plein, on attend.
    while (!m_sendingQueue.canWrite<Packet>(data)) {
        // On veut arreter le thread d'envoi, on sort.
        if (!m_executeSending) {
            return;
        }
    }
    m_sendingQueue.push(data);
}

void NetworkLayer::splitFileNameToPackets(const std::string& fileName,
                                          uint64_t fileSize,
                                          std::vector<Packet>& packetList) {
    // Le format d'envoi d'un fichier est :
    // FileNameSize (4 octets) + FileName (FileNameSize octets) + FileSize (8
    // octets) + Data (FileSize octets) Il faut donc creer les premiers paquet
    // pour supporter les premieres donnees.

    uint32_t fileNameSize = (uint32_t)fileName.size();

    // Calcule le nombre de paquet complet requis pour emmagasinner toutes les
    // donnees. Le nombre sera 1 plus petit que ce qu'on a besoin En effet, si
    // les donnees entrent toutes dans un paquet, la division entiere retournera
    // 0. Si les donnees entrent juste, on va creer un paquet vide qui sera de
    // toute facon le premier paquet remplit pour les donnees.
    uint32_t numberOfPacketNeeded = (sizeof(uint32_t) + fileNameSize +
                                     sizeof(uint64_t)) /
                                    m_packetSize;

    // On copie l'ensemble des donnees dans un buffer contigue pour se faciliter
    // la vie
    DynamicDataBuffer buffer = DynamicDataBuffer(
        sizeof(uint32_t) + fileNameSize + sizeof(uint64_t));
    uint32_t nextIndex = buffer.write(fileNameSize);
    nextIndex = buffer.write(fileNameSize,
                             reinterpret_cast<const uint8_t*>(fileName.c_str()),
                             nextIndex);
    buffer.write(fileSize, nextIndex);

    // Cree tous les premiers paquets complet
    uint32_t start = sizeof(uint32_t);
    uint32_t remainderDataSize = buffer.size();
    for (uint32_t i = 0; i < numberOfPacketNeeded; ++i) {
        Packet packet;
        packet.Data = DynamicDataBuffer(m_packetSize,
                                        &buffer.data()[i * m_packetSize]);
        remainderDataSize -= m_packetSize;
        packet.DataCount = m_packetSize;
        packetList.push_back(packet);
    }
    // Le dernier packet a creer
    Packet packet;
    packet.Data = DynamicDataBuffer(m_packetSize);
    packet.Data.write(remainderDataSize,
                      &buffer.data()[numberOfPacketNeeded * m_packetSize]);
    packet.DataCount = remainderDataSize;
    packetList.push_back(packet);
}

void NetworkLayer::sendingFile(const MACAddress& to,
                               const std::string& fileName) {
    m_currentlySendingFile = true;
    std::ifstream file;
    file.open(fileName, std::ios::in | std::ios::binary | std::ios::ate);
    if (file.is_open()) {
        std::streampos fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        NumberSequence packetNumber = 0;

        std::vector<Packet> firstPackets;
        splitFileNameToPackets(fileName, (uint64_t)fileSize, firstPackets);
        // Envoit des premiers paquets complets
        for (size_t i = 0; i < firstPackets.size() - 1; ++i) {
            Packet& p = firstPackets[i];
            p.Destination = to;
            p.Source = m_address;
            p.Number = packetNumber;
            ++packetNumber;
            sendToLinkLayer(p);
        }

        if (m_executeSending) {
            // On ajoute les premieres donnees au dernier packet. Dans le pire
            // des cas, le dernier est vide
            Packet& last = firstPackets.back();
            file.read(
                reinterpret_cast<char*>(&(last.Data.data()[last.DataCount])),
                m_packetSize - last.DataCount);
            std::streamsize numberRead = file.gcount();
            last.DataCount += (uint32_t)numberRead;
            last.Number = packetNumber;
            last.Destination = to;
            last.Source = m_address;
            ++packetNumber;
            sendToLinkLayer(last);

            while (!file.eof() && m_executeSending) {
                Packet packet;
                packet.Data = DynamicDataBuffer(m_packetSize);
                file.read(reinterpret_cast<char*>(packet.Data.data()),
                          packet.Data.size());
                std::streamsize numberRead = file.gcount();
                packet.Number = packetNumber;
                packet.Destination = to;
                packet.Source = m_address;
                packet.DataCount = (uint32_t)numberRead;
                ++packetNumber;
                sendToLinkLayer(packet);
            }
        }
        file.close();
    }
    m_currentlySendingFile = false;
}

bool NetworkLayer::startSending(const MACAddress& to,
                                const std::string& filename) {
    if (!m_executeSending) {
        m_executeSending = true;
        m_sendingThread = std::thread(&NetworkLayer::sendingFile,
                                      this,
                                      to,
                                      filename);
        return true;
    }
    return false;
}

void NetworkLayer::stopSending() {
    m_executeSending = false;
    if (m_sendingThread.joinable()) {
        m_sendingThread.join();
    }
}

bool NetworkLayer::currentSendingFinished() const {
    return !m_currentlySendingFile;
}

void NetworkLayer::startListening() {
    if (!m_executeReceiving) {
        m_executeReceiving = true;
        m_receivingThread = std::thread(&NetworkLayer::receiving, this);
    }
}

void NetworkLayer::stopListening() {
    m_executeReceiving = false;
    if (m_receivingThread.joinable()) {
        m_receivingThread.join();
    }
}

std::string NetworkLayer::constructReceivedFileName(
    const uint8_t* fileNameData,
    size_t fileNameSize,
    const Packet& packet) const {
    std::string fileName(reinterpret_cast<const char*>(fileNameData),
                         fileNameSize);
    std::stringstream ss = std::stringstream();
    ss << "From_" << packet.Source.toString() << "_To_"
       << packet.Destination.toString() << "_" << fileName;
    return ss.str();
}

void NetworkLayer::receiving() {
    // Lorsqu'on recoit un fichier, les donnees du fichiers sont envoyes comme
    // ceci : FileNameSize (4 octets) + FileName (FileNameSize octets) + Nombre
    // d'octets dans le fichier (8 octets) + Donnees du fichier (x octets) Ce
    // nombre d'octet est separe en sous packet Packet. Il faut donc relire les
    // donnees dans cet ordre.

    /*
    struct FileDataInfo
    {
        std::ofstream* File; // Le pointeur sur le fichier a ecrire
        union {
            std::uint32_t FileNameSize; // La taille du nom du fichier
            std::uint8_t FileNameSizeData[sizeof(uint32_t)];
        };

        union {
            std::uint64_t FileSize;
            std::uint8_t FileSizeData[sizeof(uint64_t)];
        };
        std::uint32_t FileSizeDataIndexToRead; // Le numero d'index du tableau
    d'octets contenant la taille des donnees lu std::uint64_t FileDataRead; //
    Le nombre de donnees recu

        std::uint32_t FileNameIndexToRead; // Le nombre de caracteres du nom de
    fichier deja lu std::uint8_t* FileNameData; // Le buffer contenant les
    donnees du nom de fichier
    };
    */

    // On pourrait recevoir de plus qu'un ordinateur, on doit donc garder les
    // infos recue pas adresse d'origine
    std::map<MACAddress, FileDataInfo> fileDataInfo;
    while (m_executeReceiving) {
        if (m_receivingQueue.canRead<Packet>()) {
            Packet p = m_receivingQueue.pop<Packet>();
            auto infoIt = fileDataInfo.find(p.Source);
            if (infoIt == fileDataInfo.end()) {
                FileDataInfo info;
                infoIt = fileDataInfo.insert(std::make_pair(p.Source, info))
                             .first;
            }
            FileDataInfo& info = (*infoIt).second;
            uint32_t dataToRead = 0;
            uint32_t indexInReadBuffer = 0;
            if (p.Number == 0) {
                info.File = nullptr;
                info.FileNameSize = p.Data.read<uint32_t>();
                info.FileNameData = new uint8_t[info.FileNameSize];
                info.FileSize = 0;
                info.FileNameIndexToRead = 0;
                info.FileSizeDataIndexToRead = 0;
                info.FileDataRead = 0;
                indexInReadBuffer = sizeof(uint32_t);
            }

            // Le fichier sera ouvert que si le nom est completement lu, sinon,
            // on doit continuer de lire le nom
            if (info.File == nullptr) {
                dataToRead = std::min(
                    info.FileNameSize,
                    p.Data.size() -
                        indexInReadBuffer); // Tout ce qu'on veut ou ce qui
                                            // reste dans le buffer
                p.Data.readTo(&info.FileNameData[info.FileNameIndexToRead],
                              indexInReadBuffer,
                              dataToRead);
                info.FileNameIndexToRead = dataToRead;
                indexInReadBuffer += dataToRead;

                if (info.FileNameIndexToRead >= info.FileNameSize) {
                    info.File = new std::ofstream(
                        constructReceivedFileName(info.FileNameData,
                                                  info.FileNameSize,
                                                  p),
                        std::ios::binary);

                    // On lit la taille
                    dataToRead = std::min(
                        (uint32_t)sizeof(uint64_t),
                        p.Data.size() -
                            indexInReadBuffer); // Tout ce qu'on veut ou ce qui
                                                // reste dans le buffer
                    p.Data.readTo(
                        &info.FileSizeData[info.FileSizeDataIndexToRead],
                        indexInReadBuffer,
                        dataToRead);
                    info.FileSizeDataIndexToRead = dataToRead;
                    indexInReadBuffer += dataToRead;

                    if (info.FileSizeDataIndexToRead >= sizeof(uint64_t)) {
                        // On peut maintenant lire les donnees
                        dataToRead = (uint32_t)std::min(
                            info.FileSize - info.FileDataRead,
                            (uint64_t)p.Data.size() -
                                (uint64_t)
                                    indexInReadBuffer); // Tout ce qu'on veut ou
                                                        // ce qui reste dans le
                                                        // buffer
                        uint8_t* bufferData = new uint8_t[dataToRead];
                        p.Data.readTo(bufferData,
                                      indexInReadBuffer,
                                      dataToRead);
                        info.File->write(reinterpret_cast<char*>(bufferData),
                                         dataToRead);
                        info.FileDataRead += dataToRead;
                        delete[] bufferData;
                        if (info.FileDataRead == info.FileSize) {
                            info.File->close();
                            delete[] info.FileNameData;
                            delete info.File;
                            info.File = nullptr;
                            fileDataInfo.erase(infoIt);
                            ++m_receivedFileCount;
                        }
                    }
                }
            } else {
                // On lit encore la taille pour commencer
                if (info.FileSizeDataIndexToRead < sizeof(uint64_t)) {
                    dataToRead = std::min((uint32_t)sizeof(uint64_t) -
                                              info.FileSizeDataIndexToRead,
                                          p.Data.size() - indexInReadBuffer);
                    p.Data.readTo(
                        &info.FileSizeData[info.FileSizeDataIndexToRead],
                        indexInReadBuffer,
                        dataToRead);
                    info.FileSizeDataIndexToRead += dataToRead;
                    indexInReadBuffer += dataToRead;
                }

                // On peut maintenant lire les donnees
                dataToRead = (uint32_t)std::min(
                    info.FileSize - info.FileDataRead,
                    (uint64_t)p.Data.size() -
                        (uint64_t)
                            indexInReadBuffer); // Tout ce qu'on veut ou ce qui
                                                // reste dans le buffer
                uint8_t* bufferData = new uint8_t[dataToRead];
                p.Data.readTo(bufferData, indexInReadBuffer, dataToRead);
                info.File->write(reinterpret_cast<char*>(bufferData),
                                 dataToRead);
                info.FileDataRead += dataToRead;
                delete[] bufferData;
                if (info.FileDataRead == info.FileSize) {
                    info.File->close();
                    delete[] info.FileNameData;
                    delete info.File;
                    info.File = nullptr;
                    fileDataInfo.erase(infoIt);
                    ++m_receivedFileCount;
                }
            }
        }
    }
}

bool NetworkLayer::dataReady() const {
    return m_sendingQueue.canRead<Packet>();
}

Packet NetworkLayer::getNextData() {
    return m_sendingQueue.pop<Packet>();
}

void NetworkLayer::receiveData(const Packet& packet) {
    // Attente active pour pouvoir continuer de recevoir. Si le buffer de
    // r�ception est plein, on attend.
    while (!m_receivingQueue.canWrite<Packet>(packet)) {
        // On veut arreter le thread de r�ception, on sort.
        if (!m_executeReceiving) {
            return;
        }
    }
    m_receivingQueue.push(packet);
}

