#include "Computer.h"

#include "../DataStructures/MACAddress.h"
#include "../General/Logger.h"

#include <fstream>
#include <iostream>

Computer::Computer(size_t numberID)
    : m_id(numberID), m_configuration(std::string("Computer") +
                                      std::to_string(numberID) + ".txt"),
      m_sendingFileCount(0), m_continueSending(false) {
    m_card = std::make_unique<NetworkInterfaceCard>(m_configuration);

    std::ifstream filesNames(std::string("Computer") +
                             std::to_string(numberID) + "Files.txt");
    if (filesNames.is_open()) {
        std::string line;
        while (std::getline(filesNames, line)) {
            m_filesToTransfert.push_back(line);
        }
        filesNames.close();
    }
}

Computer::~Computer() {
    if (m_sendingThread.joinable()) {
        m_sendingThread.join();
    }
}

void Computer::sendAllFiles() {
    for (auto it = m_filesToTransfert.cbegin(); it != m_filesToTransfert.cend();
         ++it) {
        if (m_continueSending) {
            const std::string& line = *it;
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string fileName = line.substr(0, pos);
                std::string mac = line.substr(pos + 1);

                uint8_t adr[6] = {(uint8_t)0xFF,
                                  (uint8_t)0xFF,
                                  (uint8_t)0xFF,
                                  (uint8_t)0xFF,
                                  (uint8_t)0xFF,
                                  (uint8_t)0xFF};
                size_t posMac = mac.find(':');
                size_t start = 0;
                size_t index = 0;
                while (posMac != std::string::npos) {
                    std::string subAdr = mac.substr(start, posMac - start);
                    adr[index] = (uint8_t)std::stoi(subAdr, nullptr, 16);
                    ++index;
                    start = posMac + 1;
                    posMac = mac.find(':', posMac + 1);
                }
                std::string lastSubAdr = mac.substr(start, mac.size() - start);
                adr[index] = (uint8_t)std::stoi(lastSubAdr, nullptr, 16);
                MACAddress address = MACAddress(adr);
                send_file_to(address, fileName);
                m_sendingFileCount++;
            }
        } else {
            break;
        }
    }
}

void Computer::start() {
    m_continueSending = true;
    m_sendingThread = std::thread(&Computer::sendAllFiles, this);
}

bool Computer::sendingTerminated() const {
    return m_sendingFileCount == m_filesToTransfert.size();
}

unsigned int Computer::sentFileCount() const {
    return m_sendingFileCount;
}

unsigned int Computer::receivedFileCount() const {
    return m_card->getDriver().getNetworkLayer().receivedFileCount();
}

NetworkInterfaceCard& Computer::getNetworkInterfaceCard() {
    return *m_card;
}

void Computer::send_file_to(const MACAddress& to, const std::string& fileName) {
    {
        Logger log(std::cout);
        log << "Debut de l'envoi du fichier " << fileName
            << " par l'ordinateur " << m_id << " a l'adresse " << to
            << std::endl;
    }
    m_card->start_sending_process(to, fileName);
    while (m_continueSending && !m_card->sendingFinished())
        ; // Attente active pour l'envoi complet du fichier

    if (m_continueSending) {
        Logger log(std::cout);
        log << "Fichier " << fileName << " envoye." << std::endl;
    } else {
        Logger log(std::cout);
        log << "Envoi arrete" << std::endl;
    }
}
