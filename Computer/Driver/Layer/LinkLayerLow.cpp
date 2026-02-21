#include "LinkLayerLow.h"

#include "LinkLayer.h"
#include "../NetworkDriver.h"
#include "../../../DataStructures/DataBuffer.h"
#include "../../../General/Configuration.h"
#include "../../../General/Logger.h"

#include <array>
#include <cstdint>
#include <iostream>

namespace crc16 {
static constexpr uint16_t CRC16_POLY = 0x1021;
static constexpr uint16_t CRC16_INITIAL_REMAINDER = 0xFFFF;

static constexpr uint16_t table_entry(const uint8_t byte) {
    uint16_t crc = static_cast<uint16_t>(byte) << 8;
    for (size_t bit = 0; bit < 8; ++bit) {
        if (crc & 0x8000) {
            crc = (crc << 1) ^ CRC16_POLY;
        } else {
            crc <<= 1;
        }
    }

    return crc;
}

static constexpr std::array<uint16_t, 256> make_table() {
    std::array<uint16_t, 256> table = {};
    for (size_t i = 0; i < 256; ++i) {
        table[i] = table_entry(static_cast<uint8_t>(i));
    }

    return table;
}

static constexpr std::array<uint16_t, 256> CRC16_TABLE = make_table();

// static uint16_t calculate(const uint8_t* data, const size_t length) {
//     uint16_t crc = CRC16_INITIAL_REMAINDER;
//     for (size_t i = 0; i < length; ++i) {
//         crc ^= static_cast<uint16_t>(data[i]) << 8;
//         for (size_t bit = 0; bit < 8; ++bit) {
//             if (crc & 0x8000) {
//                 crc = (crc << 1) ^ CRC16_POLY;
//             } else {
//                 crc <<= 1;
//             }
//         }
//     }
//
//     return crc;
// }

static uint16_t calculate(const uint8_t* data, const size_t length) {
    uint16_t crc = CRC16_INITIAL_REMAINDER;

    for (size_t i = 0; i < length; ++i) {
        const uint8_t table_index = static_cast<uint8_t>((crc >> 8) ^ data[i]);
        crc = static_cast<uint16_t>((crc << 8) ^ CRC16_TABLE[table_index]);
    }

    return crc;
}
} // namespace crc16

std::unique_ptr<DataEncoderDecoder> DataEncoderDecoder::CreateEncoderDecoder(
    const Configuration& config) {
    int encoderDecoderConfig = config.get(
        Configuration::LINK_LAYER_LOW_DATA_ENCODER_DECODER);
    if (encoderDecoderConfig == 1) {
        return std::make_unique<HammingDataEncoderDecoder>();
    } else if (encoderDecoderConfig == 2) {
        return std::make_unique<CRCDataEncoderDecoder>();
    } else {
        return std::make_unique<PassthroughDataEncoderDecoder>();
    }
}

DynamicDataBuffer PassthroughDataEncoderDecoder::encode(
    const DynamicDataBuffer& data) const {
    return data;
}

std::pair<bool, DynamicDataBuffer> PassthroughDataEncoderDecoder::decode(
    const DynamicDataBuffer& data) const {
    return std::pair<bool, DynamicDataBuffer>(true, data);
}

//===================================================================
// Hamming Encoder decoder implementation
//===================================================================
HammingDataEncoderDecoder::HammingDataEncoderDecoder() {
    // � faire TP
}

HammingDataEncoderDecoder::~HammingDataEncoderDecoder() {
    // � faire TP
}

DynamicDataBuffer HammingDataEncoderDecoder::encode(
    const DynamicDataBuffer& data) const {
    // � faire TP
    return data;
}

std::pair<bool, DynamicDataBuffer> HammingDataEncoderDecoder::decode(
    const DynamicDataBuffer& data) const {
    // � faire TP
    return std::pair<bool, DynamicDataBuffer>(true, data);
}

//===================================================================
// CRC Encoder decoder implementation
//===================================================================
CRCDataEncoderDecoder::CRCDataEncoderDecoder() {}

CRCDataEncoderDecoder::~CRCDataEncoderDecoder() {}

DynamicDataBuffer CRCDataEncoderDecoder::encode(
    const DynamicDataBuffer& data) const {
    DynamicDataBuffer out{data.size() + 2, data.data()};
    const uint16_t crc = crc16::calculate(data.data(), data.size());
    out.write<uint8_t>(crc >> 8, data.size());
    out.write<uint8_t>(crc & 0xFF, data.size() + 1);
    return out;
}

std::pair<bool, DynamicDataBuffer> CRCDataEncoderDecoder::decode(
    const DynamicDataBuffer& data) const {
    // we need at least 2 bytes for the CRC
    if (data.size() < 2) {
        return std::pair<bool, DynamicDataBuffer>{false, DynamicDataBuffer{}};
    }

    const uint32_t data_size = data.size() - 2;
    const uint16_t received_crc = (static_cast<uint16_t>(data[data_size])
                                   << 8) |
                                  static_cast<uint16_t>(data[data_size + 1]);
    const uint16_t computed_crc = crc16::calculate(data.data(), data_size);

	Logger log{std::cout};
    if (received_crc != computed_crc) {
		// log << "CRC check failed" << std::endl;
        return std::pair<bool, DynamicDataBuffer>{false, DynamicDataBuffer{}};
    }

	// log << "CRC check passed" << std::endl;
    return std::pair<bool, DynamicDataBuffer>{
        true,
        DynamicDataBuffer{data_size, data.data()}};
}

//===================================================================
// Network Driver Physical layer implementation
//===================================================================
LinkLayerLow::LinkLayerLow(NetworkDriver* driver, const Configuration& config)
    : m_driver(driver), m_sendingBuffer(config.get(
                            Configuration::LINK_LAYER_LOW_SENDING_BUFFER_SIZE)),
      m_receivingBuffer(
          config.get(Configuration::LINK_LAYER_LOW_RECEIVING_BUFFER_SIZE)),
      m_stopReceiving(true), m_stopSending(true) {
    m_encoderDecoder = DataEncoderDecoder::CreateEncoderDecoder(config);
}

LinkLayerLow::~LinkLayerLow() {
    stop();
    m_driver = nullptr;
}

void LinkLayerLow::start() {
    stop();

    start_receiving();
    start_sending();
}

void LinkLayerLow::stop() {
    stop_receiving();
    stop_sending();
}

bool LinkLayerLow::dataReceived() const {
    return m_receivingBuffer.canRead<DynamicDataBuffer>();
}

DynamicDataBuffer LinkLayerLow::encode(const DynamicDataBuffer& data) const {
    return m_encoderDecoder->encode(data);
}

std::pair<bool, DynamicDataBuffer> LinkLayerLow::decode(
    const DynamicDataBuffer& data) const {
    return m_encoderDecoder->decode(data);
}

void LinkLayerLow::start_receiving() {
    m_stopReceiving = false;
    m_receivingThread = std::thread(&LinkLayerLow::receiving, this);
}

void LinkLayerLow::stop_receiving() {
    m_stopReceiving = true;
    if (m_receivingThread.joinable()) {
        m_receivingThread.join();
    }
}

void LinkLayerLow::start_sending() {
    m_stopSending = false;
    m_sendingThread = std::thread(&LinkLayerLow::sending, this);
}

void LinkLayerLow::stop_sending() {
    m_stopSending = true;
    if (m_sendingThread.joinable()) {
        m_sendingThread.join();
    }
}

void LinkLayerLow::receiving() {
    while (!m_stopReceiving) {
        if (dataReceived()) {
            DynamicDataBuffer data = m_receivingBuffer.pop<DynamicDataBuffer>();
            std::pair<bool, DynamicDataBuffer> dataBuffer = decode(data);
            if (dataBuffer.first) // Les donnees recues sont correctes et
                                  // peuvent etre utilisees
            {
                Frame frame = Buffering::unpack<Frame>(dataBuffer.second);
                m_driver->getLinkLayer().receiveData(frame);
            } else {
                // Les donnees recues sont corrompues et doivent etre delaissees
                Logger log(std::cout);
                log << m_driver->getMACAddress() << " : Corrupted data received"
                    << std::endl;
            }
        }
    }
}

void LinkLayerLow::sending() {
    while (!m_stopSending) {
        if (m_driver->getLinkLayer().dataReady()) {
            Frame dataFrame = m_driver->getLinkLayer().getNextData();
            DynamicDataBuffer buffer = encode(
                Buffering::pack<Frame>(dataFrame));
            sendData(buffer);
        }
    }
}

void LinkLayerLow::receiveData(const DynamicDataBuffer& data) {
    // Si le buffer est plein, on fait juste oublier les octets recus du cable
    // Sinon, on ajoute les octets au buffer
    if (m_receivingBuffer.canWrite<DynamicDataBuffer>(data)) {
        m_receivingBuffer.push(data);
    } else {
        Logger log(std::cout);
        log << m_driver->getMACAddress()
            << " : Physical reception buffer full... data discarded"
            << std::endl;
    }
}

void LinkLayerLow::sendData(DynamicDataBuffer data) {
    // Envoit une suite d'octet sur le cable connecte
    m_driver->sendToCard(data);
}
