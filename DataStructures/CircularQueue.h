#ifndef _GENERAL_CIRCULAR_QUEUE_H_
#define _GENERAL_CIRCULAR_QUEUE_H_

#include <atomic>
#include <cstdint>
#include <stdexcept>

#include "Utils.h"

// Buffer circulaire d'octet de style FIFO
// Thread-safe pour 1 consommateur et 1 producteur seulement
class CircularQueue {
    size_t m_capacity;
    size_t m_head;
    size_t m_tail;
    std::atomic<size_t> m_size;
    uint8_t* m_buffer;

    CircularQueue& operator=(const CircularQueue&) = delete;
    CircularQueue(const CircularQueue&) = delete;

    bool enoughSpaceFor(size_t numberOfByte) const;

    template <typename T>
    bool enoughDataFor() const {
        return EnoughDataFor<T>::in(m_buffer, m_head, size(), capacity());
    }

public:
    CircularQueue(size_t capacity);

    ~CircularQueue();

    size_t size() const;
    size_t capacity() const;

    template <typename T>
    bool canWrite(const T& data) const {
        size_t dataSize = SizeOf<T>::data(data);
        return enoughSpaceFor(dataSize);
    }

    void push(const uint8_t& value);

    template <typename T>
    void push(const T& value) {
        // ToDataPtr est une structure utilitaire qui permet d'acceder a des
        // octets particuliers d'un objet quelconque a l'aide d'un index
        ToDataPtr<T> toDataPtr(value);
        size_t dataSize = toDataPtr.size();
        if (enoughSpaceFor(dataSize)) {
            for (size_t i = 0; i < dataSize; ++i) {
                push(toDataPtr[i]);
            }
        } else {
            throw std::out_of_range(
                "Il n'y a pas assez d'espace dans le buffer pour ecrire les "
                "donnees demandees.");
        }
    }

    template <typename T>
    bool canRead() const {
        return enoughDataFor<T>();
    }

    uint8_t pop();

    template <typename T>
    T pop() {
        if (canRead<T>()) {
            size_t dataSize = FromDataPtr<T>::size(m_buffer,
                                                   m_head,
                                                   capacity());
            uint8_t* dataPtr = new uint8_t[dataSize];
            for (size_t i = 0; i < dataSize; ++i) {
                dataPtr[i] = pop();
            }
            T data = FromDataPtr<T>::get(dataPtr, dataSize);
            delete[] dataPtr;
            return data;
        }
        throw std::out_of_range("Il n'y a pas assez d'element dans le buffer "
                                "pour construire un objet de ce type.");
    }
};

#endif //_GENERAL_CIRCULAR_QUEUE_H_
