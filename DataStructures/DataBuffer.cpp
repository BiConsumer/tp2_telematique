#include "DataBuffer.h"

#include <algorithm>

DynamicDataBuffer::DynamicDataBuffer() : m_size(0) {
    m_data = nullptr;
}

DynamicDataBuffer::DynamicDataBuffer(uint32_t size) : m_size(size) {
    m_data = new uint8_t[m_size];
}

DynamicDataBuffer::DynamicDataBuffer(uint32_t size, const uint8_t* data)
    : DynamicDataBuffer(size) {
    for (uint32_t i = 0; i < m_size; ++i) {
        m_data[i] = data[i];
    }
}

DynamicDataBuffer::DynamicDataBuffer(const DynamicDataBuffer& other)
    : DynamicDataBuffer(other.m_size, other.m_data) {}

DynamicDataBuffer::DynamicDataBuffer(DynamicDataBuffer&& other)
    : m_size(other.m_size), m_data(other.m_data) {
    other.m_data = nullptr;
    other.m_size = 0;
}

DynamicDataBuffer::~DynamicDataBuffer() {
    delete[] m_data;
}

bool DynamicDataBuffer::operator==(const DynamicDataBuffer& other) const {
    if (m_size == other.size()) {
        for (size_t i = 0; i < m_size; ++i) {
            if (m_data[i] != other[i]) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool DynamicDataBuffer::operator!=(const DynamicDataBuffer& other) const {
    return !(*this == other);
}

DynamicDataBuffer& DynamicDataBuffer::operator=(DynamicDataBuffer&& other) {
    // On évite l'autoassignement
    if (&other != this) {
        delete[] m_data;
        m_data = other.m_data;
        m_size = other.m_size;
        other.m_data = nullptr;
        other.m_size = 0;
    }
    return *this;
}

DynamicDataBuffer& DynamicDataBuffer::operator=(
    const DynamicDataBuffer& other) {
    // On évite l'autoassignement
    if (&other != this) {
        delete[] m_data;
        m_data = new uint8_t[other.m_size];
        m_size = other.m_size;
        for (uint32_t i = 0; i < m_size; ++i) {
            m_data[i] = other.m_data[i];
        }
    }
    return *this;
}

uint8_t DynamicDataBuffer::operator[](size_t index) const {
    return m_data[index];
}

uint8_t& DynamicDataBuffer::operator[](size_t index) {
    return m_data[index];
}

uint32_t DynamicDataBuffer::size() const {
    return m_size;
}

const uint8_t* DynamicDataBuffer::data() const {
    return m_data;
}

uint8_t* DynamicDataBuffer::data() {
    return m_data;
}

void DynamicDataBuffer::replaceData(const uint8_t* data,
                                    uint32_t newDataSize,
                                    uint32_t start) {
    uint32_t end = std::min(start + newDataSize, m_size);
    for (uint32_t i = start, j = 0; i < end; ++i, ++j) {
        m_data[i] = data[j];
    }
}

void DynamicDataBuffer::replaceData(const DynamicDataBuffer& buffer,
                                    uint32_t start) {
    replaceData(buffer.data(), buffer.size(), start);
}

