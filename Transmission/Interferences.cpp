#include "Interferences.h"
#include "../DataStructures/DataBuffer.h"
#include "../General/Configuration.h"
#include "../General/Logger.h"

#include <cmath>
#include <iostream>

std::unique_ptr<Interference> Interference::CreateInterferenceImplementation(
    const Configuration& config) {
    int noiseConfig = config.get(Configuration::TRANSMISSION_HUB_NOISE);
    if (noiseConfig == 1) {
        return std::make_unique<RandomInterference>(
            config.get(Configuration::TRANSMISSION_HUB_NOISE_FREQUENCY),
            config.get(
                Configuration::TRANSMISSION_HUB_NOISE_BYTE_ERROR_FREQUENCY));
    } else {
        return std::make_unique<NoInterference>();
    }
}

void NoInterference::noise(DynamicDataBuffer& data) {}

RandomInterference::RandomInterference(unsigned int frequency,
                                       unsigned int byteErrorFrequency)
    : m_randomGenerator(), m_frequency(frequency),
      m_frequencyDistribution(0, 100), m_errorGenerator(1, 255),
      m_byteErrorFrequency(byteErrorFrequency) {}

void RandomInterference::noise(DynamicDataBuffer& data) {
    bool applyNoise = m_frequencyDistribution(m_randomGenerator) <= m_frequency;
    if (applyNoise) {
        Logger log(std::cout);
        log << "Random Noise applied on data" << std::endl;
        auto distribution = std::uniform_int_distribution<unsigned int>(
            0,
            data.size() - 1);
        // On veut au moins 1 octet modifie
        unsigned int errorByteCount = (unsigned int)std::ceil(
            (data.size() * m_byteErrorFrequency) / 100.0);
        for (unsigned int i = 0; i < errorByteCount; ++i) {
            unsigned int byteNumberInError = distribution(m_randomGenerator);
            uint8_t error = (uint8_t)m_errorGenerator(m_randomGenerator);
            data[byteNumberInError] = data[byteNumberInError] ^
                                      error; // OU Exclusif
        }
    }
}
