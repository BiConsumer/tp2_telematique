#ifndef _TRANSMISSION_CABLE_H_
#define _TRANSMISSION_CABLE_H_

#include <cstdint>

class DynamicDataBuffer;
class NetworkInterfaceCard;
class TransmissionHub;

class Cable {
    NetworkInterfaceCard& m_nic;
    TransmissionHub* m_hub;

public:
    Cable(TransmissionHub* hub, NetworkInterfaceCard& card);

    const NetworkInterfaceCard& getConnectedNIC() const;

    void sendToHub(DynamicDataBuffer& data);
    void sendToCard(DynamicDataBuffer& data);
};

#endif //_TRANSMISSION_CABLE_H_
