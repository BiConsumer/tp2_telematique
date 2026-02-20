#ifndef _COMPUTER_DRIVER_LAYER_NETWORK_LAYER_H_
#define _COMPUTER_DRIVER_LAYER_NETWORK_LAYER_H_

#include "DataType.h"
#include "../../../DataStructures/CircularQueue.h"
#include "../../../DataStructures/DataBuffer.h"
#include "../../../DataStructures/MACAddress.h"

#include <atomic>
#include <cstdint>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

class Configuration;
class NetworkDriver;

class NetworkLayer {
    struct FileDataInfo {
        std::ofstream* File; // Le pointeur sur le fichier a ecrire
        union {
            std::uint32_t FileNameSize; // La taille du nom du fichier
            std::uint8_t FileNameSizeData[sizeof(uint32_t)];
        };

        union {
            std::uint64_t FileSize;
            std::uint8_t FileSizeData[sizeof(uint64_t)];
        };
        std::uint32_t
            FileSizeDataIndexToRead; // Le numero d'index du tableau d'octets
                                     // contenant la taille des donnees lu
        std::uint64_t FileDataRead;  // Le nombre de donnees recu

        std::uint32_t FileNameIndexToRead; // Le nombre de caracteres du nom de
                                           // fichier deja lu
        std::uint8_t*
            FileNameData; // Le buffer contenant les donnees du nom de fichier
    };

    NetworkDriver* m_driver;
    MACAddress
        m_address; // Dans ce simulateur, on utilise l'adresse MAC, mais ce
                   // devrait plutot etre une adresse IP dans la couche reseau

    uint32_t m_packetSize;

    std::thread m_sendingThread;
    std::thread m_receivingThread;

    std::atomic<bool> m_executeSending;
    std::atomic<bool> m_executeReceiving;
    std::atomic<bool> m_currentlySendingFile;
    std::atomic<unsigned int> m_receivedFileCount;

    CircularQueue m_receivingQueue;
    CircularQueue m_sendingQueue;

    void sendingFile(const MACAddress& to, const std::string& fileName);
    void receiving();

    void sendToLinkLayer(const Packet& data);

    void splitFileNameToPackets(const std::string& fileName,
                                uint64_t fileSize,
                                std::vector<Packet>& packetList);
    std::string constructReceivedFileName(const uint8_t* fileNameData,
                                          size_t fileNameSize,
                                          const Packet& packet) const;

    void startListening();
    void stopListening();
    void stopSending();

public:
    NetworkLayer(NetworkDriver* driver, const Configuration& config);
    ~NetworkLayer();

    bool dataReady() const;
    Packet getNextData();

    unsigned int receivedFileCount() const;

    void receiveData(const Packet& packet);

    void start();
    void stop();

    bool startSending(const MACAddress& to, const std::string& filename);

    bool currentSendingFinished() const;
};

#endif //_COMPUTER_DRIVER_LAYER_NETWORK_LAYER_H_
