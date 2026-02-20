#ifndef _COMPUTER_COMPUTER_H_
#define _COMPUTER_COMPUTER_H_

#include <atomic>
#include <string>
#include <thread>
#include <vector>

#include "../General/Configuration.h"
#include "Hardware/NetworkInterfaceCard.h"

class MACAddress;

class Computer {
    size_t m_id;
    Configuration m_configuration;
    std::unique_ptr<NetworkInterfaceCard> m_card;
    std::vector<std::string> m_filesToTransfert;
    std::atomic<bool> m_continueSending;
    std::atomic<unsigned int> m_sendingFileCount;
    std::thread m_sendingThread;

    void sendAllFiles();

public:
    Computer(size_t numberID);
    ~Computer();

    bool sendingTerminated() const;
    unsigned int sentFileCount() const;
    unsigned int receivedFileCount() const;

    void start();

    NetworkInterfaceCard& getNetworkInterfaceCard();

    void send_file_to(const MACAddress& address, const std::string& fileName);
};

#endif //_COMPUTER_COMPUTER_H_
