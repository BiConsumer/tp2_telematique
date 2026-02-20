#ifndef _TRANSMISSION_TRANSMISSION_H_
#define _TRANSMISSION_TRANSMISSION_H_

#include "../DataStructures/CircularQueue.h"
#include "../DataStructures/DataBuffer.h"
#include "Interferences.h"

#include <atomic>
#include <mutex>
#include <thread>
#include <unordered_set>

class Cable;
class Computer;
class Configuration;

class TransmissionHub {
private:
    struct HubData {
        Cable* from;
        DynamicDataBuffer data;
    };

    std::unordered_set<Cable*> m_connections;

    CircularQueue m_dataQueue;

    TransmissionHub& operator=(const TransmissionHub&) = delete;
    TransmissionHub(const TransmissionHub&) = delete;

    std::unique_ptr<Interference> m_interference;

    std::atomic<bool> m_stop;
    std::mutex m_mutex;
    std::thread m_transmissionThread;

    void transmit();

    void noise(DynamicDataBuffer& data);

    // Necessaire pour permettre la specialisation des structures SizeOf,
    // EnoughDataFor, ToDataPtr, FromDataPtr
    friend struct SizeOf<TransmissionHub::HubData>;
    friend struct EnoughDataFor<TransmissionHub::HubData>;
    friend struct ToDataPtr<TransmissionHub::HubData>;
    friend struct FromDataPtr<TransmissionHub::HubData>;

public:
    TransmissionHub(const Configuration& config);
    ~TransmissionHub();

    void start();
    void stop();

    void connect_computer(Computer* computer);

    void receive_data_to_dispatch(const DynamicDataBuffer& data, Cable* from);
};

// Utilitaire pour verifier la taille reelle d'un objet de type
// TransmissionHub::HubData Cette specialisation remplace l'implementation de
// base de la structure dans Utils.h lorsque le type T est
// TransmissionHub::HubData
template <>
struct SizeOf<TransmissionHub::HubData> {
    static size_t data(const TransmissionHub::HubData& data) {
        return sizeof(Cable*) + data.data.size();
    }
};

// Utilitaire pour verifier s'il y a assez de donnee dans une suite d'octet pour
// reconstruire correctement un objet de type TransmissionHub::HubData Cette
// specialisation remplace l'implementation de base de la structure dans Utils.h
// lorsque le type T est TransmissionHub::HubData
template <>
struct EnoughDataFor<TransmissionHub::HubData> {
    static bool in(const uint8_t* dataBuffer,
                   size_t bufferStart,
                   size_t bufferSize,
                   size_t bufferCapacity) {
        if (bufferSize > sizeof(Cable*)) {
            return EnoughDataFor<DynamicDataBuffer>::in(
                dataBuffer,
                (bufferStart + sizeof(Cable*)) % bufferCapacity,
                bufferSize - sizeof(Cable*),
                bufferCapacity);
        } else {
            return false;
        }
    }
};

// Utilitaire pour acceder a la representation equivalente a une suite d'octet
// pour ecrire correctement un objet de type TransmissionHub::HubData Cette
// specialisation remplace l'implementation de base de la structure dans Utils.h
// lorsque le type T est TransmissionHub::HubData
template <>
struct ToDataPtr<TransmissionHub::HubData> {
    const TransmissionHub::HubData& Data;
    ToDataPtr<DynamicDataBuffer> DynamicDataBufferToDataPtr;

    ToDataPtr(const TransmissionHub::HubData& data)
        : Data(data), DynamicDataBufferToDataPtr(data.data) {}

    uint8_t operator[](size_t byteIndex) {
        if (byteIndex < sizeof(Cable*)) {
            const uint8_t* dataPtr = reinterpret_cast<const uint8_t*>(&Data);
            return dataPtr[byteIndex];
        } else {
            return DynamicDataBufferToDataPtr[byteIndex - sizeof(Cable*)];
        }
    }

    size_t size() const {
        return DynamicDataBufferToDataPtr.size() + sizeof(Cable*);
    }
};

// Utilitaire pour lire un objet dans une suite d'octet pour reconstruire
// correctement un objet de type TransmissionHub::HubData Cette specialisation
// remplace l'implementation de base de la structure dans Utils.h lorsque le
// type T est TransmissionHub::HubData
template <>
struct FromDataPtr<TransmissionHub::HubData> {
    static size_t size(const uint8_t* dataBuffer,
                       size_t bufferStart,
                       size_t bufferCapacity) {
        return sizeof(Cable*) +
               FromDataPtr<DynamicDataBuffer>::size(
                   dataBuffer,
                   (bufferStart + sizeof(Cable*)) % bufferCapacity,
                   bufferCapacity);
    }

    static TransmissionHub::HubData get(const uint8_t* data, size_t dataSize) {
        union {
            Cable* Ptr;
            uint8_t ByteData[sizeof(Cable*)];
        } cablePtr;

        for (size_t i = 0; i < sizeof(Cable*); ++i) {
            cablePtr.ByteData[i] = data[i];
        }

        size_t dynamicBufferSize = FromDataPtr<DynamicDataBuffer>::size(
            &data[sizeof(Cable*)],
            0,
            dataSize - sizeof(Cable*));
        TransmissionHub::HubData dataValue = {
            cablePtr.Ptr,
            FromDataPtr<DynamicDataBuffer>::get(&data[sizeof(Cable*)],
                                                dynamicBufferSize)};
        return dataValue;
    }
};

#endif //_TRANSMISSION_TRANSMISSION_H_
