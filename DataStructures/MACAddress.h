#ifndef _GENERAL_MAC_ADDRESS_H_
#define _GENERAL_MAC_ADDRESS_H_

#include <cstdint>
#include <iostream>
#include <string>

class Configuration;

class MACAddress {
    uint8_t m_address[6];

public:
    MACAddress();
    MACAddress(const uint8_t[6]);
    MACAddress(const Configuration& config);
    MACAddress(const MACAddress& other) = default;
    ~MACAddress() = default;

    bool operator==(const MACAddress& other) const;
    bool operator!=(const MACAddress& other) const;

    bool operator<(const MACAddress& other)
        const; // Pour pouvoir utiliser la MACAddress comme clé dans un map

    bool isUnicast() const;
    bool isMulticast() const;

    bool isLocallyAdministered() const;
    bool isGloballyUnique() const;

    std::string toString() const;

    friend std::ostream& operator<<(std::ostream& out,
                                    const MACAddress& address);
};

#endif //_GENERAL_MAC_ADDRESS_H_
