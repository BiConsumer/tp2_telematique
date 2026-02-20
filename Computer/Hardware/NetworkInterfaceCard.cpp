#include "NetworkInterfaceCard.h"

#include "../../DataStructures/DataBuffer.h"
#include "../../DataStructures/MACAddress.h"
#include "../../Transmission/Cable.h"

NetworkInterfaceCard::NetworkInterfaceCard(const Configuration& config)
    : m_driver(std::make_unique<NetworkDriver>(this, config)) {}

NetworkInterfaceCard::~NetworkInterfaceCard() {}

NetworkDriver& NetworkInterfaceCard::getDriver() {
    return *m_driver;
}

const NetworkDriver& NetworkInterfaceCard::getDriver() const {
    return *m_driver;
}

void NetworkInterfaceCard::connect(Cable* cable) {
    m_cableConnected = cable;
}

void NetworkInterfaceCard::send(DynamicDataBuffer& data) {
    // Si aucun cable n'est connecte, on ne peut envoyer nulle part
    if (m_cableConnected) {
        m_cableConnected->sendToHub(data);
    }
}

void NetworkInterfaceCard::receive(const DynamicDataBuffer& data) {
    m_driver->receiveFromCard(data);
}

void NetworkInterfaceCard::start_sending_process(const MACAddress& to,
                                                 const std::string& fileName) {
    m_driver->start_sending_process(to, fileName);
}

bool NetworkInterfaceCard::sendingFinished() const {
    return m_driver->sendingFinished();
}

