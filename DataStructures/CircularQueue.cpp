#include "CircularQueue.h"

CircularQueue::CircularQueue(size_t capacity)
    : m_size(0), m_capacity(capacity), m_head(0), m_tail(0) {
    m_buffer = new uint8_t[m_capacity];
}

CircularQueue::~CircularQueue() {
    delete[] m_buffer;
}

size_t CircularQueue::size() const {
    return m_size;
}

size_t CircularQueue::capacity() const {
    return m_capacity;
}

void CircularQueue::push(const uint8_t& value) {
    m_buffer[m_tail] = value;
    m_tail = (m_tail + 1) % m_capacity;
    ++m_size;
}

uint8_t CircularQueue::pop() {
    size_t pos = m_head;
    m_head = (m_head + 1) % m_capacity;
    uint8_t data = m_buffer[pos];
    --m_size;
    return data;
}

bool CircularQueue::enoughSpaceFor(size_t numberOfByte) const {
    return size() + numberOfByte <= capacity();
}

