#ifndef _COMPUTER_DRIVER_LAYER_LINK_LAYER_LOW_H_
#define _COMPUTER_DRIVER_LAYER_LINK_LAYER_LOW_H_

#include "DataType.h"
#include "../../../DataStructures/CircularQueue.h"
#include "../../../DataStructures/DataBuffer.h"

#include <atomic>
#include <thread>

class Configuration;
class NetworkDriver;

class DataEncoderDecoder {
public:
    static std::unique_ptr<DataEncoderDecoder> CreateEncoderDecoder(
        const Configuration& conf);

    // Cette fonction transforme l'ensemble des octets a envoyer pour lui
    // ajouter de l'information pour la correction et la detection d'erreur.
    // Retourne le nouveau flux d'octet a envoyer.
    virtual DynamicDataBuffer encode(const DynamicDataBuffer& data) const = 0;

    // Cette fonction transforme l'ensemble des octets recus pour detecter les
    // erreurs et potentiellement les corriger Cette fonction retourne une paire
    // dont le premier element est un booleen indiquant si le deuxieme parametre
    // contient des valeurs valides ou s'il y avait des erreurs non corrigees
    // dans le flux d'entree.
    virtual std::pair<bool, DynamicDataBuffer> decode(
        const DynamicDataBuffer& data) const = 0;
};

class PassthroughDataEncoderDecoder : public DataEncoderDecoder {
public:
    DynamicDataBuffer encode(const DynamicDataBuffer& data) const override;
    std::pair<bool, DynamicDataBuffer> decode(
        const DynamicDataBuffer& data) const override;
};

class HammingDataEncoderDecoder : public DataEncoderDecoder {
    // À faire TP
public:
    HammingDataEncoderDecoder();
    ~HammingDataEncoderDecoder();
    DynamicDataBuffer encode(const DynamicDataBuffer& data) const override;
    std::pair<bool, DynamicDataBuffer> decode(
        const DynamicDataBuffer& data) const override;
};

class CRCDataEncoderDecoder : public DataEncoderDecoder {
public:
    CRCDataEncoderDecoder();
    ~CRCDataEncoderDecoder();
    DynamicDataBuffer encode(const DynamicDataBuffer& data) const override;
    std::pair<bool, DynamicDataBuffer> decode(
        const DynamicDataBuffer& data) const override;
};

class LinkLayerLow {
private:
    NetworkDriver* m_driver;
    std::unique_ptr<DataEncoderDecoder> m_encoderDecoder;

    CircularQueue m_sendingBuffer;
    CircularQueue m_receivingBuffer;

    std::thread m_sendingThread;
    std::thread m_receivingThread;

    std::atomic<bool> m_stopReceiving;
    std::atomic<bool> m_stopSending;

    void sending();
    void receiving();

    DynamicDataBuffer encode(const DynamicDataBuffer& data) const;
    std::pair<bool, DynamicDataBuffer> decode(
        const DynamicDataBuffer& data) const;

    void sendData(DynamicDataBuffer data);

    void start_receiving();
    void stop_receiving();

    void start_sending();
    void stop_sending();

public:
    LinkLayerLow(NetworkDriver* driver, const Configuration& config);
    ~LinkLayerLow();

    void start();
    void stop();

    bool dataReceived() const;
    void receiveData(const DynamicDataBuffer& data);
};

#endif //_COMPUTER_DRIVER_LAYER_LINK_LAYER_LOW_H_
