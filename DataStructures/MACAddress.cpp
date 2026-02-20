#include "MACAddress.h"
#include "../General/Configuration.h"

#include <iomanip>
#include <sstream>

// Utilitaire pour l'affichage en hexadecimal
struct HexStruct {
    uint8_t c;
    HexStruct(uint8_t _c) : c(_c) {}
};

inline std::ostream& operator<<(std::ostream& out, const HexStruct& hs) {
    auto flags = out.flags();
    out << std::setprecision(2) << std::hex << (uint32_t)hs.c;
    out.flags(flags);
    return out;
}

inline HexStruct hex(uint8_t _c) {
    return HexStruct(_c);
}
//===========================================

MACAddress::MACAddress() {
    for (size_t i = 0; i < 6; ++i) {
        m_address[i] = (uint8_t)0xFF;
    }
}

MACAddress::MACAddress(const uint8_t address[6]) {
    for (size_t i = 0; i < 6; ++i) {
        m_address[i] = address[i];
    }
}

MACAddress::MACAddress(const Configuration& config) {
    m_address[0] = (uint8_t)config.get(Configuration::MAC_ADDRESS_BYTE_1);
    m_address[1] = (uint8_t)config.get(Configuration::MAC_ADDRESS_BYTE_2);
    m_address[2] = (uint8_t)config.get(Configuration::MAC_ADDRESS_BYTE_3);
    m_address[3] = (uint8_t)config.get(Configuration::MAC_ADDRESS_BYTE_4);
    m_address[4] = (uint8_t)config.get(Configuration::MAC_ADDRESS_BYTE_5);
    m_address[5] = (uint8_t)config.get(Configuration::MAC_ADDRESS_BYTE_6);
}

bool MACAddress::operator==(const MACAddress& other) const {
    for (size_t i = 0; i < 6; ++i) {
        if (m_address[i] != other.m_address[i]) {
            return false;
        }
    }
    return true;
}

bool MACAddress::operator!=(const MACAddress& other) const {
    return !(*this == other);
}

bool MACAddress::operator<(const MACAddress& other) const {
	for (size_t i = 0; i < 6; ++i) {
		if (m_address[i] < other.m_address[i]) {
			return true;
		} else if (m_address[i] > other.m_address[i]) {
			return false;
		}
	}

    return false;
}

bool MACAddress::isUnicast() const {
    return ((uint32_t)m_address[0] & (uint32_t)0x1) == 0;
}

bool MACAddress::isMulticast() const {
    return !isUnicast();
}

bool MACAddress::isGloballyUnique() const {
    return ((uint32_t)m_address[0] & (uint32_t)0x2) == 0;
}

bool MACAddress::isLocallyAdministered() const {
    return !isGloballyUnique();
}

std::string MACAddress::toString() const {
    std::stringstream ss;
    ss << hex(m_address[0]) << '-';
    ss << hex(m_address[1]) << '-';
    ss << hex(m_address[2]) << '-';
    ss << hex(m_address[3]) << '-';
    ss << hex(m_address[4]) << '-';
    ss << hex(m_address[5]) << '-';
    return ss.str();
}

std::ostream& operator<<(std::ostream& out, const MACAddress& address) {
    out << hex(address.m_address[0]) << ':';
    out << hex(address.m_address[1]) << ':';
    out << hex(address.m_address[2]) << ':';
    out << hex(address.m_address[3]) << ':';
    out << hex(address.m_address[4]) << ':';
    out << hex(address.m_address[5]);
    return out;
}

