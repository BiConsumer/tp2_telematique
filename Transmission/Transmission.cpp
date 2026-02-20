#include "Transmission.h"
#include "Cable.h"
#include "../Computer/Computer.h"
#include "../Computer/Hardware/NetworkInterfaceCard.h"

#include "../General/Logger.h"

#include <iostream>

TransmissionHub::TransmissionHub(const Configuration& config)
    : m_transmissionThread(),
      m_dataQueue(config.get(Configuration::TRANSMISSION_HUB_BUFFER_SIZE)),
      m_stop(true) {
    m_interference = Interference::CreateInterferenceImplementation(config);
}

TransmissionHub::~TransmissionHub() {
    stop();
    for (auto it = m_connections.begin(); it != m_connections.end(); ++it) {
        delete *it;
    }
    m_connections.clear();
}

void TransmissionHub::start() {
    m_stop = false;
    m_transmissionThread = std::thread(&TransmissionHub::transmit, this);
}

void TransmissionHub::stop() {
    m_stop = true;
    if (m_transmissionThread.joinable()) {
        m_transmissionThread.join();
    }
}

void TransmissionHub::connect_computer(Computer* computer) {
    Cable* cable = new Cable(this, computer->getNetworkInterfaceCard());
    m_connections.insert(cable);
}

void TransmissionHub::transmit() {
    while (!m_stop) {
        if (m_dataQueue.canRead<HubData>()) {
            HubData data = m_dataQueue.pop<HubData>();
            for (auto it = m_connections.cbegin(); it != m_connections.cend();
                 ++it) {
                if (data.from != (*it)) {
                    // Logger log(std::cout);
                    // log << "HUB - Sending data to " <<
                    // (*it)->getConnectedNIC().getDriver().getMACAddress() <<
                    // std::endl;
                    DynamicDataBuffer buffer = data.data;
                    (*it)->sendToCard(buffer);
                }
            }
        }
    }
}

void TransmissionHub::noise(DynamicDataBuffer& buffer) {
    m_interference->noise(buffer);
}

void TransmissionHub::receive_data_to_dispatch(const DynamicDataBuffer& data,
                                               Cable* from) {
    // Plusieurs ordinateurs peuvent envoyer un signal en meme temps. Il faut
    // les synchroniser!
    std::lock_guard<std::mutex> guard(m_mutex);
    HubData hubData = {from, data};
    // On applique le bruit sur le signal
    noise(hubData.data);
    if (m_dataQueue.canWrite<HubData>(hubData)) {
        // Logger log(std::cout);
        // log << "HUB - Received data from " <<
        // from->getConnectedNIC()->getDriver()->getMACAddress() << std::endl;
        m_dataQueue.push(hubData);
    } else {
        Logger log(std::cout);
        log << "Data lost - Transmission buffer full : " << std::endl;
        log << "\tFrom : "
            << from->getConnectedNIC().getDriver().getMACAddress() << std::endl;
    }
}
