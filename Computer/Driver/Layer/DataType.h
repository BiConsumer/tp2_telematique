#ifndef _COMPUTER_DRIVER_LAYER_DATA_TYPE_H_
#define _COMPUTER_DRIVER_LAYER_DATA_TYPE_H_

#include <cstdint>

#include "../../../DataStructures/DataBuffer.h"
#include "../../../DataStructures/MACAddress.h"
#include "../../../DataStructures/Utils.h"

using NumberSequence = uint16_t;

// Une valeur < 1500 indique des donnees. Les valeurs plus grande indiquer un
// ACK seul ou un NAK
enum FrameType {
    ACK = 0x601,
    NAK = 0x602,
};

// L'ordre dans les structures est importante afin de garder les valeurs
// alignées (uint32_t sur 4 octets)
struct Packet {
    MACAddress Destination; // 6 octets
    MACAddress Source;      // 6 octets
    NumberSequence Number;  // 2 octets
    uint16_t DataCount;     // 2 octets
    DynamicDataBuffer
        Data; // 4 + X octets. Les 4 premiers octets indique la valeur de X
};

struct Frame {
    MACAddress Destination;   // 6 octets
    MACAddress Source;        // 6 octets
    NumberSequence Ack;       // 2 octets
    NumberSequence NumberSeq; // 2 octets
    uint32_t Size;            // 4 octets
    DynamicDataBuffer
        Data; // 4 + X octets. Les 4 premiers octets indique la valeur de X
};

// Structure utilitaire permettant de creer un Buffer a partir d'un objet ou de
// creer un objet a partir d'un Buffer Voir PhysicalLayer.cpp, methodes
// sending() et receiving() pour un exemple d'utilisation
struct Buffering {
    template <typename T>
    static DynamicDataBuffer pack(const T& value) {
        ToDataPtr<T> toDataPtr(value);
        uint32_t bufferSize = (uint32_t)toDataPtr.size();
        DynamicDataBuffer buffer = DynamicDataBuffer(bufferSize);
        for (uint32_t i = 0; i < bufferSize; ++i) {
            buffer[i] = toDataPtr[i];
        }
        return buffer;
    }

    template <typename T>
    static T unpack(const DynamicDataBuffer& buffer) {
        return FromDataPtr<T>::get(buffer.data(), buffer.size());
    }
};

// Utilitaire pour verifier la taille reelle d'un objet de type Packet et Frame
// Cette specialisation remplace l'implementation de base de la structure dans
// Utils.h lorsque le type T est Packet ou Frame
template <>
struct SizeOf<Packet> {
    static size_t data(const Packet& data) {
        return 2 * SizeOf<MACAddress>::value + sizeof(NumberSequence) +
               sizeof(uint16_t) + SizeOf<DynamicDataBuffer>::data(data.Data);
    }
};

template <>
struct SizeOf<Frame> {
    static size_t data(const Frame& data) {
        return 2 * SizeOf<MACAddress>::value + 2 * sizeof(NumberSequence) +
               sizeof(uint32_t) + SizeOf<DynamicDataBuffer>::data(data.Data);
    }
};

// Utilitaire pour verifier s'il y a assez de donnee dans une suite d'octet pour
// reconstruire correctement un objet de type Packet et Frame Cette
// specialisation remplace l'implementation de base de la structure dans Utils.h
// lorsque le type T est Packet ou Frame
template <>
struct EnoughDataFor<Packet> {
    static bool in(const uint8_t* dataBuffer,
                   size_t bufferStart,
                   size_t bufferSize,
                   size_t bufferCapacity) {
        size_t minimumSizeNeeded = 2 * SizeOf<MACAddress>::value +
                                   sizeof(NumberSequence) + sizeof(uint16_t);
        if (bufferSize > minimumSizeNeeded) {
            return EnoughDataFor<DynamicDataBuffer>::in(
                dataBuffer,
                bufferStart + minimumSizeNeeded,
                bufferSize - minimumSizeNeeded,
                bufferCapacity);
        } else {
            return false;
        }
    }
};

template <>
struct EnoughDataFor<Frame> {
    static bool in(const uint8_t* dataBuffer,
                   size_t bufferStart,
                   size_t bufferSize,
                   size_t bufferCapacity) {
        size_t minimumSizeNeeded = 2 * SizeOf<MACAddress>::value +
                                   2 * sizeof(NumberSequence) +
                                   sizeof(uint32_t);
        if (bufferSize > minimumSizeNeeded) {
            return EnoughDataFor<DynamicDataBuffer>::in(
                dataBuffer,
                bufferStart + minimumSizeNeeded,
                bufferSize - minimumSizeNeeded,
                bufferCapacity);
        } else {
            return false;
        }
    }
};

// Utilitaire pour acceder a la representation equivalente a une suite d'octet
// pour ecrire correctement un objet de type Packet et Frame Cette
// specialisation remplace l'implementation de base de la structure dans Utils.h
// lorsque le type T est Packet ou Frame
template <>
struct ToDataPtr<Packet> {
    const Packet& Data;
    ToDataPtr<DynamicDataBuffer> DataBufferToPtrObj;

    ToDataPtr(const Packet& data) : Data(data), DataBufferToPtrObj(data.Data) {}

    uint8_t operator[](size_t byteIndex) {
        size_t bufferOffset = 2 * SizeOf<MACAddress>::value +
                              sizeof(NumberSequence) + sizeof(uint16_t);
        if (byteIndex < bufferOffset) {
            const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&Data);
            return dataPtr[byteIndex];
        } else {
            return DataBufferToPtrObj[byteIndex - bufferOffset];
        }
    }

    size_t size() const {
        return SizeOf<Packet>::data(Data);
    }
};

template <>
struct ToDataPtr<Frame> {
    const Frame& Data;
    ToDataPtr<DynamicDataBuffer> DataBufferToPtrObj;

    ToDataPtr(const Frame& data) : Data(data), DataBufferToPtrObj(data.Data) {}

    uint8_t operator[](size_t byteIndex) {
        size_t bufferOffset = 2 * SizeOf<MACAddress>::value +
                              2 * sizeof(NumberSequence) + sizeof(uint32_t);
        if (byteIndex < bufferOffset) {
            const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&Data);
            return dataPtr[byteIndex];
        } else {
            return DataBufferToPtrObj[byteIndex - bufferOffset];
        }
    }

    size_t size() const {
        return SizeOf<Frame>::data(Data);
    }
};

// Utilitaire pour lire un objet dans une suite d'octet pour reconstruire
// correctement un objet de type Packet et Frame Cette specialisation remplace
// l'implementation de base de la structure dans Utils.h lorsque le type T est
// Packet ou Frame
template <>
struct FromDataPtr<Packet> {
    static size_t size(const uint8_t* dataBuffer,
                       size_t bufferStart,
                       size_t bufferCapacity) {
        size_t bufferOffset = 2 * SizeOf<MACAddress>::value +
                              sizeof(NumberSequence) + sizeof(uint16_t);
        return bufferOffset + FromDataPtr<DynamicDataBuffer>::size(
                                  dataBuffer,
                                  (bufferStart + bufferOffset) % bufferCapacity,
                                  bufferCapacity);
    }

    static Packet get(const uint8_t* data, size_t dataSize) {
        Packet packet;
        uint8_t* packetData = reinterpret_cast<uint8_t*>(&packet);
        size_t bufferOffset = 2 * SizeOf<MACAddress>::value +
                              sizeof(NumberSequence) + sizeof(uint16_t);
        for (size_t i = 0; i < bufferOffset; ++i) {
            packetData[i] = data[i];
        }
        packet.Data = FromDataPtr<DynamicDataBuffer>::get(&data[bufferOffset],
                                                          dataSize -
                                                              bufferOffset);
        return packet;
    }
};

template <>
struct FromDataPtr<Frame> {
    static size_t size(const uint8_t* dataBuffer,
                       size_t bufferStart,
                       size_t bufferCapacity) {
        size_t bufferOffset = 2 * SizeOf<MACAddress>::value +
                              2 * sizeof(NumberSequence) + sizeof(uint32_t);
        return bufferOffset + FromDataPtr<DynamicDataBuffer>::size(
                                  dataBuffer,
                                  (bufferStart + bufferOffset) % bufferCapacity,
                                  bufferCapacity);
    }

    static Frame get(const uint8_t* data, size_t dataSize) {
        Frame packet;
        uint8_t* packetData = reinterpret_cast<uint8_t*>(&packet);
        size_t bufferOffset = 2 * SizeOf<MACAddress>::value +
                              2 * sizeof(NumberSequence) + sizeof(uint32_t);
        for (size_t i = 0; i < bufferOffset; ++i) {
            packetData[i] = data[i];
        }
        packet.Data = FromDataPtr<DynamicDataBuffer>::get(&data[bufferOffset],
                                                          dataSize -
                                                              bufferOffset);
        return packet;
    }
};

#endif //_COMPUTER_DRIVER_LAYER_DATA_TYPE_H_
