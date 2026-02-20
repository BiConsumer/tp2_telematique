#ifndef _GENERAL_DATA_BUFFER_H_
#define _GENERAL_DATA_BUFFER_H_

#include <algorithm>
#include <cstdint>

#include "Utils.h"

class DynamicDataBuffer {
    uint32_t m_size;
    uint8_t* m_data;

public:
    DynamicDataBuffer();
    DynamicDataBuffer(uint32_t dataSize);
    DynamicDataBuffer(uint32_t dataSize, const uint8_t* data);
    DynamicDataBuffer(const DynamicDataBuffer& other);
    DynamicDataBuffer(DynamicDataBuffer&& other);
    ~DynamicDataBuffer();

    DynamicDataBuffer& operator=(DynamicDataBuffer&& other);
    DynamicDataBuffer& operator=(const DynamicDataBuffer& other);

    bool operator==(const DynamicDataBuffer& other) const;
    bool operator!=(const DynamicDataBuffer& other) const;

    template <typename T>
    uint32_t write(const T& data) {
        return write((uint32_t)SizeOf<T>::data(data),
                     reinterpret_cast<const uint8_t*>(&data),
                     0);
    }

    template <typename T>
    uint32_t write(const T& data, uint32_t start) {
        return write((uint32_t)SizeOf<T>::data(data),
                     reinterpret_cast<const uint8_t*>(&data),
                     start);
    }

    uint32_t write(uint32_t count, const uint8_t* data) {
        return write(count, data, 0);
    }

    uint32_t write(uint32_t count, const uint8_t* data, uint32_t start) {
        uint32_t end = std::min(m_size, count + start);
        for (uint32_t i = start; i < end; ++i) {
            m_data[i] = data[i - start];
        }
        return end;
    }

    template <typename T>
    T read(uint32_t start = 0) const {
        return *reinterpret_cast<T*>(((void*)&(m_data[start])));
    }

    void readTo(uint8_t* data, uint32_t start, uint32_t size) {
        for (uint32_t i = 0; i < size; ++i) {
            data[i] = m_data[start + i];
        }
    }

    void replaceData(const uint8_t* newData,
                     uint32_t newDataSize,
                     uint32_t start = 0);
    void replaceData(const DynamicDataBuffer& buffer, uint32_t start = 0);

    uint8_t operator[](size_t index) const;
    uint8_t& operator[](size_t index);

    uint8_t* data();
    const uint8_t* data() const;

    uint32_t size() const;
};

// Utilitaire pour verifier la taille reelle d'un objet de type
// DynamicDataBuffer Cette specialisation remplace l'implementation de base de
// la structure dans Utils.h lorsque le type T est DynamicDataBuffer
template <>
struct SizeOf<DynamicDataBuffer> {
    static constexpr size_t value = sizeof(DynamicDataBuffer);

    static size_t data(const DynamicDataBuffer& data) {
        return sizeof(uint32_t) + data.size();
    }
};

// Utilitaire pour verifier s'il y a assez de donnee dans une suite d'octet pour
// reconstruire correctement un objet de type DynamicDataBuffer Cette
// specialisation remplace l'implementation de base de la structure dans Utils.h
// lorsque le type T est DynamicDataBuffer
template <>
struct EnoughDataFor<DynamicDataBuffer> {
    static bool in(const uint8_t* dataBuffer,
                   size_t bufferStart,
                   size_t bufferSize,
                   size_t bufferCapacity) {
        if (bufferSize >= sizeof(uint32_t)) {
            union {
                uint32_t Size;
                uint8_t ByteData[sizeof(uint32_t)];
            } bufferSizeNeeded;

            for (size_t i = 0; i < sizeof(uint32_t); ++i) {
                bufferSizeNeeded.ByteData[i] = dataBuffer[(bufferStart + i) %
                                                          bufferCapacity];
            }

            return bufferSize >= sizeof(uint32_t) + bufferSizeNeeded.Size;
        } else {
            return false;
        }
    }
};

// Utilitaire pour acceder a la representation equivalente a une suite d'octet
// pour ecrire correctement un objet de type DynamicDataBuffer Cette
// specialisation remplace l'implementation de base de la structure dans Utils.h
// lorsque le type T est DynamicDataBuffer
template <>
struct ToDataPtr<DynamicDataBuffer> {
    const DynamicDataBuffer& Data;

    ToDataPtr(const DynamicDataBuffer& data) : Data(data) {}

    uint8_t operator[](size_t byteIndex) {
        if (byteIndex < sizeof(uint32_t)) {
            const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&Data);
            return dataPtr[byteIndex];
        } else {
            return Data[byteIndex - sizeof(uint32_t)];
        }
    }

    size_t size() const {
        return Data.size() + sizeof(uint32_t);
    }
};

// Utilitaire pour lire un objet dans une suite d'octet pour reconstruire
// correctement un objet de type DynamicDataBuffer Cette specialisation remplace
// l'implementation de base de la structure dans Utils.h lorsque le type T est
// DynamicDataBuffer
template <>
struct FromDataPtr<DynamicDataBuffer> {
    static size_t size(const uint8_t* dataBuffer,
                       size_t bufferStart,
                       size_t bufferCapacity) {
        union {
            uint32_t Size;
            uint8_t ByteData[sizeof(uint32_t)];
        } bufferSize;
        for (size_t i = 0; i < sizeof(uint32_t); ++i) {
            bufferSize
                .ByteData[i] = dataBuffer[(bufferStart + i) % bufferCapacity];
        }

        return sizeof(uint32_t) + bufferSize.Size;
    }

    static DynamicDataBuffer get(const uint8_t* data, size_t dataSize) {
        DynamicDataBuffer dataValue((uint32_t)(dataSize - sizeof(uint32_t)),
                                    &data[sizeof(uint32_t)]);
        return dataValue;
    }
};

#endif //_GENERAL_DATA_BUFFER_H_
