#include "Configuration.h"

#include <fstream>
#include <stdexcept>

const std::string Configuration::
    NETWORK_LAYER_RECEIVING_BUFFER_SIZE = "NetworkLayerReceivingBufferSize";
const std::string Configuration::
    NETWORK_LAYER_SENDING_BUFFER_SIZE = "NetworkLayerSendingBufferSize";
const std::string
    Configuration::NETWORK_LAYER_DATA_SIZE = "NetworkLayerDataSize";

const std::string Configuration::
    LINK_LAYER_RECEIVING_BUFFER_SIZE = "LinkLayerReceivingBufferSize";
const std::string Configuration::
    LINK_LAYER_SENDING_BUFFER_SIZE = "LinkLayerSendingBufferSize";
const std::string Configuration::
    LINK_LAYER_MAXIMUM_BUFFERED_FRAME = "LinkLayerMaximumBufferedFrame";
const std::string Configuration::LINK_LAYER_TIMEOUT = "LinkLayerTimeout";

const std::string Configuration::
    LINK_LAYER_LOW_RECEIVING_BUFFER_SIZE = "LinkLayerLowReceivingBufferSize";
const std::string Configuration::
    LINK_LAYER_LOW_SENDING_BUFFER_SIZE = "LinkLayerLowSendingBufferSize";
const std::string Configuration::
    LINK_LAYER_LOW_DATA_ENCODER_DECODER = "LinkLayerLowDataEncoderDecoder";

const std::string
    Configuration::TRANSMISSION_HUB_BUFFER_SIZE = "TransmissionHubBufferSize";
const std::string
    Configuration::TRANSMISSION_HUB_NOISE = "TransmissionHubNoise";
const std::string Configuration::
    TRANSMISSION_HUB_NOISE_FREQUENCY = "TransmissionHubNoiseFrequency";
const std::string Configuration::TRANSMISSION_HUB_NOISE_BYTE_ERROR_FREQUENCY =
    "TransmissionHubNoiseByteErrorFrequency";

const std::string Configuration::MAC_ADDRESS_BYTE_1 = "MacAddressByte1";
const std::string Configuration::MAC_ADDRESS_BYTE_2 = "MacAddressByte2";
const std::string Configuration::MAC_ADDRESS_BYTE_3 = "MacAddressByte3";
const std::string Configuration::MAC_ADDRESS_BYTE_4 = "MacAddressByte4";
const std::string Configuration::MAC_ADDRESS_BYTE_5 = "MacAddressByte5";
const std::string Configuration::MAC_ADDRESS_BYTE_6 = "MacAddressByte6";

Configuration::Configuration(const std::string& configFilename) {
    init();
    std::ifstream configFile(configFilename);
    if (configFile.is_open()) {
        std::string line;
        while (std::getline(configFile, line)) {
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string param = line.substr(0, pos);
                std::string value = line.substr(pos + 1);

                m_configs[param] = std::stoi(value, nullptr, 0);
            }
        }

        configFile.close();
    }
}

void Configuration::init() {
    m_configs[Configuration::NETWORK_LAYER_RECEIVING_BUFFER_SIZE] =
        Configuration::NETWORK_LAYER_RECEIVING_BUFFER_SIZE_DEFAULT_VALUE;
    m_configs[Configuration::NETWORK_LAYER_SENDING_BUFFER_SIZE] =
        Configuration::NETWORK_LAYER_SENDING_BUFFER_SIZE_DEFAULT_VALUE;
    m_configs[Configuration::NETWORK_LAYER_DATA_SIZE] = Configuration::
        NETWORK_LAYER_DATA_SIZE_DEFAULT_VALUE;

    m_configs[Configuration::LINK_LAYER_RECEIVING_BUFFER_SIZE] = Configuration::
        LINK_LAYER_RECEIVING_BUFFER_SIZE_DEFAULT_VALUE;
    m_configs[Configuration::LINK_LAYER_SENDING_BUFFER_SIZE] = Configuration::
        LINK_LAYER_SENDING_BUFFER_SIZE_DEFAULT_VALUE;
    m_configs[Configuration::LINK_LAYER_MAXIMUM_BUFFERED_FRAME] =
        Configuration::LINK_LAYER_MAXIMUM_BUFFERED_FRAME_DEFAULT_VALUE;
    m_configs[Configuration::LINK_LAYER_TIMEOUT] = Configuration::
        LINK_LAYER_TIMEOUT_DEFAULT_VALUE;

    m_configs[Configuration::LINK_LAYER_LOW_RECEIVING_BUFFER_SIZE] =
        Configuration::LINK_LAYER_LOW_RECEIVING_BUFFER_SIZE_DEFAULT_VALUE;
    m_configs[Configuration::LINK_LAYER_LOW_SENDING_BUFFER_SIZE] =
        Configuration::LINK_LAYER_LOW_SENDING_BUFFER_SIZE_DEFAULT_VALUE;
    m_configs[Configuration::LINK_LAYER_LOW_DATA_ENCODER_DECODER] =
        Configuration::LINK_LAYER_LOW_DATA_ENCODER_DECODER_DEFAULT_VALUE;

    m_configs[Configuration::TRANSMISSION_HUB_BUFFER_SIZE] = Configuration::
        TRANSMISSION_HUB_BUFFER_SIZE_DEFAULT_VALUE;
    m_configs[Configuration::TRANSMISSION_HUB_NOISE] = Configuration::
        TRANSMISSION_HUB_NOISE_DEFAULT_VALUE;
    m_configs[Configuration::TRANSMISSION_HUB_NOISE_FREQUENCY] = Configuration::
        TRANSMISSION_HUB_NOISE_FREQUENCY_DEFAULT_VALUE;
    m_configs[Configuration::
                  TRANSMISSION_HUB_NOISE_BYTE_ERROR_FREQUENCY] = Configuration::
        TRANSMISSION_HUB_NOISE_BYTE_ERROR_FREQUENCY_DEFAULT_VALUE;

    m_configs[Configuration::MAC_ADDRESS_BYTE_1] = Configuration::
        MAC_ADDRESS_BYTE_1_DEFAULT_VALUE;
    m_configs[Configuration::MAC_ADDRESS_BYTE_2] = Configuration::
        MAC_ADDRESS_BYTE_2_DEFAULT_VALUE;
    m_configs[Configuration::MAC_ADDRESS_BYTE_3] = Configuration::
        MAC_ADDRESS_BYTE_3_DEFAULT_VALUE;
    m_configs[Configuration::MAC_ADDRESS_BYTE_4] = Configuration::
        MAC_ADDRESS_BYTE_4_DEFAULT_VALUE;
    m_configs[Configuration::MAC_ADDRESS_BYTE_5] = Configuration::
        MAC_ADDRESS_BYTE_5_DEFAULT_VALUE;
    m_configs[Configuration::MAC_ADDRESS_BYTE_6] = Configuration::
        MAC_ADDRESS_BYTE_6_DEFAULT_VALUE;
}

int Configuration::get(const std::string& paramName) const {
    auto it = m_configs.find(paramName);
    if (it != m_configs.end()) {
        return (*it).second;
    }
    throw std::invalid_argument(
        "Le parametre demande n'existe pas dans la configuration.");
}
