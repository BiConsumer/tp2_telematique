#ifndef _GENERAL_CONFIGURATION_H_
#define _GENERAL_CONFIGURATION_H_

#include <map>
#include <string>

class Configuration {
    std::map<std::string, int> m_configs;

    void init();

public:
    static const std::string NETWORK_LAYER_RECEIVING_BUFFER_SIZE;
    static const std::string NETWORK_LAYER_SENDING_BUFFER_SIZE;
    static const std::string NETWORK_LAYER_DATA_SIZE;
    static const int NETWORK_LAYER_RECEIVING_BUFFER_SIZE_DEFAULT_VALUE = 500000;
    static const int NETWORK_LAYER_SENDING_BUFFER_SIZE_DEFAULT_VALUE = 500000;
    static const int NETWORK_LAYER_DATA_SIZE_DEFAULT_VALUE = 50;

    static const std::string LINK_LAYER_RECEIVING_BUFFER_SIZE;
    static const std::string LINK_LAYER_SENDING_BUFFER_SIZE;
    static const std::string LINK_LAYER_MAXIMUM_BUFFERED_FRAME;
    static const std::string LINK_LAYER_TIMEOUT;
    static const int LINK_LAYER_RECEIVING_BUFFER_SIZE_DEFAULT_VALUE = 500000;
    static const int LINK_LAYER_SENDING_BUFFER_SIZE_DEFAULT_VALUE = 500000;
    static const int LINK_LAYER_MAXIMUM_BUFFERED_FRAME_DEFAULT_VALUE = 4;
    static const int
        LINK_LAYER_TIMEOUT_DEFAULT_VALUE = 1000; // En millisecondes

    static const std::string LINK_LAYER_LOW_RECEIVING_BUFFER_SIZE;
    static const std::string LINK_LAYER_LOW_SENDING_BUFFER_SIZE;
    static const std::string LINK_LAYER_LOW_DATA_ENCODER_DECODER;
    static const int
        LINK_LAYER_LOW_RECEIVING_BUFFER_SIZE_DEFAULT_VALUE = 500000;
    static const int LINK_LAYER_LOW_SENDING_BUFFER_SIZE_DEFAULT_VALUE = 500000;
    static const int LINK_LAYER_LOW_DATA_ENCODER_DECODER_DEFAULT_VALUE = 0;

    static const std::string TRANSMISSION_HUB_BUFFER_SIZE;
    static const std::string TRANSMISSION_HUB_NOISE;
    static const std::string TRANSMISSION_HUB_NOISE_FREQUENCY;
    static const std::string TRANSMISSION_HUB_NOISE_BYTE_ERROR_FREQUENCY;
    static const int TRANSMISSION_HUB_BUFFER_SIZE_DEFAULT_VALUE = 500000;
    static const int TRANSMISSION_HUB_NOISE_DEFAULT_VALUE = 1;
    static const int TRANSMISSION_HUB_NOISE_FREQUENCY_DEFAULT_VALUE = 10;
    static const int
        TRANSMISSION_HUB_NOISE_BYTE_ERROR_FREQUENCY_DEFAULT_VALUE = 1;

    static const std::string MAC_ADDRESS_BYTE_1;
    static const std::string MAC_ADDRESS_BYTE_2;
    static const std::string MAC_ADDRESS_BYTE_3;
    static const std::string MAC_ADDRESS_BYTE_4;
    static const std::string MAC_ADDRESS_BYTE_5;
    static const std::string MAC_ADDRESS_BYTE_6;
    static const int MAC_ADDRESS_BYTE_1_DEFAULT_VALUE = 0xDE;
    static const int MAC_ADDRESS_BYTE_2_DEFAULT_VALUE = 0xAD;
    static const int MAC_ADDRESS_BYTE_3_DEFAULT_VALUE = 0xBE;
    static const int MAC_ADDRESS_BYTE_4_DEFAULT_VALUE = 0xEF;
    static const int MAC_ADDRESS_BYTE_5_DEFAULT_VALUE = 0x00;
    static const int MAC_ADDRESS_BYTE_6_DEFAULT_VALUE = 0x01;

    Configuration(const std::string& configFilename);
    ~Configuration() = default;
    int get(const std::string& paramName) const;
};

#endif //_GENERAL_CONFIGURATION_H_
