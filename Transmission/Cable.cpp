#include "Cable.h"
#include "Transmission.h"

#include "../Computer/Hardware/NetworkInterfaceCard.h"
#include "../DataStructures/DataBuffer.h"

Cable::Cable(TransmissionHub* hub, NetworkInterfaceCard& card)
    : m_hub(hub), m_nic(card) {
    m_nic.connect(this);
}

const NetworkInterfaceCard& Cable::getConnectedNIC() const {
    return m_nic;
}

void Cable::sendToHub(DynamicDataBuffer& data) {
    m_hub->receive_data_to_dispatch(data, this);
}

void Cable::sendToCard(DynamicDataBuffer& data) {
    m_nic.receive(data);
}
