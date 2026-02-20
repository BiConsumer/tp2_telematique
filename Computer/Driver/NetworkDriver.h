#ifndef _COMPUTER_DRIVER_NETWORK_DRIVER_H_
#define _COMPUTER_DRIVER_NETWORK_DRIVER_H_

#include "Layer/LinkLayerLow.h"
#include "Layer/LinkLayer.h"
#include "Layer/NetworkLayer.h"

#include <string>

class Configuration;
class MACAddress;
class NetworkInterfaceCard;

class NetworkDriver {
    NetworkInterfaceCard* m_hardware;
    std::unique_ptr<LinkLayerLow> m_linkLayerLow;
    std::unique_ptr<LinkLayer> m_linkLayer;
    std::unique_ptr<NetworkLayer> m_networkLayer;

public:
    NetworkDriver(NetworkInterfaceCard* hardware, const Configuration& config);
    ~NetworkDriver();

    LinkLayerLow& getLinkLayerLow();
    LinkLayer& getLinkLayer();
    NetworkLayer& getNetworkLayer();

    const MACAddress& getMACAddress() const;

    void start_sending_process(const MACAddress& to,
                               const std::string& filename);
    bool sendingFinished() const;

    void sendToCard(DynamicDataBuffer& data);
    void receiveFromCard(const DynamicDataBuffer& data);
};

#endif //_COMPUTER_DRIVER_NETWORK_DRIVER_H_

