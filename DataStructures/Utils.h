#ifndef _GENERAL_UTILS_H_
#define _GENERAL_UTILS_H_

#include <algorithm>
#include <cstdint>

template <typename T>
struct SizeOf {
    static constexpr size_t value = sizeof(T);

    static constexpr size_t data(const T& data) {
        return value;
    }
};

template <typename T>
struct EnoughDataFor {
    static constexpr bool in(const uint8_t* dataBuffer,
                             size_t bufferStart,
                             size_t bufferSize,
                             size_t bufferCapacity) {
        size_t dataSize = SizeOf<T>::value;
        return bufferSize >= dataSize;
    }
};

template <typename T>
struct FromDataPtr {
    static constexpr size_t size(const uint8_t* dataBuffer,
                                 size_t bufferStart,
                                 size_t bufferCapacity) {
        return SizeOf<T>::value;
    }

    static T get(const uint8_t* data, size_t dataSize) {
        T dataValue;
        uint8_t* dataPtr = reinterpret_cast<uint8_t*>(&dataValue);
        for (size_t i = 0; i < dataSize; ++i) {
            dataPtr[i] = data[i];
        }
        return dataValue;
    }
};

template <typename T>
struct ToDataPtr {
    const T& Data;

    ToDataPtr(const T& data) : Data(data) {}

    uint8_t operator[](size_t byteIndex) {
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&Data);
        return dataPtr[byteIndex];
    }

    size_t size() const {
        return SizeOf<T>::data(Data);
    }

    /*static uint8_t* get(const T& data)
    {
        size_t dataSize = SizeOf<T>::data(Data);
        uint8_t* dataBuffer = new uint8_t[dataSize];
        const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&data);
        for (size_t i = 0; i < dataSize; ++i)
        {
            dataBuffer[i] = dataPtr[i];
        }
        return dataBuffer; // Attention, il faut que l'appelant fasse un
    delete[] lui-meme.
    }*/
};

#endif //_GENERAL_UTILS_H_
