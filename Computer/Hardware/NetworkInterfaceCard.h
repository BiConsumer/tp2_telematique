#ifndef _COMPUTER_HARDWARE_NETWORK_INTERFACE_CARD_H_
#define _COMPUTER_HARDWARE_NETWORK_INTERFACE_CARD_H_

#include <cstdint>

#include "../Driver/NetworkDriver.h"

class Cable;
class Configuration;
class DynamicDataBuffer;
class MACAddress;

// Represente une carte reseau
class NetworkInterfaceCard {
    std::unique_ptr<NetworkDriver> m_driver;
    Cable* m_cableConnected;

public:
    NetworkInterfaceCard(const Configuration& config);
    ~NetworkInterfaceCard();

    NetworkDriver& getDriver();
    const NetworkDriver& getDriver() const;

    void connect(Cable* cable);

    void send(DynamicDataBuffer& data);
    void receive(const DynamicDataBuffer& data);

    void start_sending_process(const MACAddress& to,
                               const std::string& fileName);

    bool sendingFinished() const;
};

#endif //_COMPUTER_HARDWARE_NETWORK_INTERFACE_CARD_H_
