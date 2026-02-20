#ifndef _COMPUTER_DRIVER_LAYER_LINK_LAYER_H_
#define _COMPUTER_DRIVER_LAYER_LINK_LAYER_H_

#include "DataType.h"
#include "../../../DataStructures/CircularQueue.h"
#include "../../../DataStructures/DataBuffer.h"
#include "../../../DataStructures/MACAddress.h"
#include "../../../General/Timer.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <queue>
#include <mutex>
#include <thread>
#include <map>

class Configuration;
class NetworkDriver;

inline constexpr NumberSequence NO_ACK = static_cast<NumberSequence>(-1);

class LinkLayer {
private:
	using window_t = std::map<NumberSequence, Frame>;

    enum class EventType {
        INVALID,          // Un evenement invalide
        ACK_TIMEOUT,      // On doit envoyer un ack, parce qu'on n'a pas fait de
                          // piggybacking
        SEND_TIMEOUT,     // On n'a pas recu de reponse du receveur, on doit
                          // reenvoyer la trame
        ACK_RECEIVED,     // On a recu un ACK
        NAK_RECEIVED,     // On a recu un NAK
        SEND_ACK_REQUEST, // On doit envoyer ce ACK
        SEND_NAK_REQUEST, // On doit envoyer ce NAK
        STOP_ACK_TIMER_REQUEST, // On veut arreter les timers de ACK pour une
                                // adresse particuliere
    };

	struct PendingAck {
		MACAddress Address;
		NumberSequence AckNumber;
	};
	
    struct Event {
        EventType Type = EventType::INVALID;
        size_t Number = 0;
        size_t TimerID = 0;
        MACAddress Address;
        NumberSequence Next = 0;

        static Event Invalid() {
            return Event{};
        }
    };

    NetworkDriver* m_driver;
    std::unique_ptr<Timer> m_timers;

    MACAddress m_address;

    NumberSequence m_maximumSequence;
    NumberSequence m_maximumBufferedFrameCount;

    std::chrono::milliseconds m_transmissionTimeout;
    std::chrono::milliseconds m_ackTimeout;
    std::queue<Event> m_receivingEventQueue;
    std::queue<Event> m_sendingEventQueue;

    CircularQueue m_sendingQueue;
    CircularQueue m_receivingQueue;

    std::atomic<bool> m_executeReceiving;
    std::atomic<bool> m_executeSending;

	// tp2
	std::map<MACAddress, PendingAck> m_pending_acks{};

    std::mutex m_mutex;
    std::mutex m_receiveEventMutex;
    std::mutex m_sendEventMutex;

    std::thread m_senderThread;
    std::thread m_receiverThread;

    void receiverCallback();
    void senderCallback();

    bool canSendData(const Frame& data) const;
    bool between(NumberSequence value,
                 NumberSequence first,
                 NumberSequence last) const;

    void sendAck(const MACAddress& to, NumberSequence ackNumber);
    void sendNak(const MACAddress& to, NumberSequence nakNumber);
    bool sendFrame(const Frame& frame);

    void notifyNAK(const Frame& frame);
    void notifyACK(const Frame& frame, NumberSequence piggybackAck);

    void transmissionTimeout(size_t timerID, NumberSequence numberData);
    void ackTimeout(size_t timerID, NumberSequence numberData);

    size_t startAckTimer(size_t existingTimerID, NumberSequence ackNumber);
    void stopAckTimer(size_t timerID);
    void notifyStopAckTimers(const MACAddress& to);

    size_t startTimeoutTimer(NumberSequence number);

    Event getNextSendingEvent();
    Event getNextReceivingEvent();

    MACAddress arp(const Packet& p)
        const; // Retourne la MACAddress de destination du packet
    bool canReceiveDataFromPhysicalLayer(const Frame& data) const;

public:
    LinkLayer(NetworkDriver* driver, const Configuration& config);
    ~LinkLayer();

    const MACAddress& getMACAddress() const;

    void start();
    void stop();

    bool dataReady() const;
    Frame getNextData();

    bool dataReceived() const;
    void receiveData(Frame data);
};

#endif //_COMPUTER_DRIVER_LAYER_LINK_LAYER_H_
