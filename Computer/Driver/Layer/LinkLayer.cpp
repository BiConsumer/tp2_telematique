#include "LinkLayer.h"
#include "../NetworkDriver.h"

#include "../../../General/Configuration.h"
#include "../../../General/Logger.h"

#include <iostream>
#include <functional>
#include <map>
#include <set>
#include <thread>

LinkLayer::LinkLayer(NetworkDriver* driver, const Configuration& config)
    : m_driver(driver), m_address(config),
      m_receivingQueue(
          config.get(Configuration::LINK_LAYER_RECEIVING_BUFFER_SIZE)),
      m_sendingQueue(config.get(Configuration::LINK_LAYER_SENDING_BUFFER_SIZE)),
      m_maximumBufferedFrameCount(
          config.get(Configuration::LINK_LAYER_MAXIMUM_BUFFERED_FRAME)),
      m_transmissionTimeout(config.get(Configuration::LINK_LAYER_TIMEOUT)),
      m_executeReceiving(false), m_executeSending(false) {
    m_maximumSequence = m_maximumBufferedFrameCount * 2 - 1;
    m_ackTimeout = m_transmissionTimeout / 4;
    m_timers = std::make_unique<Timer>();
}

LinkLayer::~LinkLayer() {
    stop();
    m_driver = nullptr;
}

const MACAddress& LinkLayer::getMACAddress() const {
    return m_address;
}

// Demarre les fils d'execution pour l'envoi et la reception des trames
void LinkLayer::start() {
    stop();

    m_timers->start();

    m_executeReceiving = true;
    m_receiverThread = std::thread(&LinkLayer::receiverCallback, this);

    m_executeSending = true;
    m_senderThread = std::thread(&LinkLayer::senderCallback, this);
}

// Arrete les fils d'execution pour l'envoi et la reception des trames
void LinkLayer::stop() {
    m_timers->stop();

    m_executeReceiving = false;
    if (m_receiverThread.joinable()) {
        m_receiverThread.join();
    }

    m_executeSending = false;
    if (m_senderThread.joinable()) {
        m_senderThread.join();
    }
}

// Indique vrai si on peut envoyer des donnees dans le buffer de sortie, faux si
// le buffer est plein
bool LinkLayer::canSendData(const Frame& data) const {
    return m_sendingQueue.canWrite<Frame>(data);
}

// Indique vrai si des donnees sont disponibles dans le buffer d'entree, faux
// s'il n'y a rien
bool LinkLayer::dataReceived() const {
    return m_receivingQueue.canRead<Frame>();
}

// Indique vrai s'il y a des donnees dans le buffer de sortie
bool LinkLayer::dataReady() const {
    return m_sendingQueue.canRead<Frame>();
}

// Recupere la prochaine donnee du buffer de sortie
Frame LinkLayer::getNextData() {
    return m_sendingQueue.pop<Frame>();
}

// Envoit une trame dans le buffer de sortie
// Cette fonction retourne faux si la trame n'a pas ete envoyee. Ce cas arrive
// seulement si le programme veut se terminer. Fait de l'attente active jusqu'a
// ce qu'il puisse envoyer la trame sinon.
bool LinkLayer::sendFrame(const Frame& frame) {
    while (m_executeSending) {
        if (canSendData(frame)) {
            // Vous pouvez d�commenter ce code pour avoir plus de d�tails
            // dans la console lors de l'ex�cution Logger log(std::cout); if
            // (frame.Size == FrameType::NAK)
            //{
            //    log << frame.Source << " : Sending NAK  to " <<
            //    frame.Destination << " : " << frame.Ack << std::endl;
            //}
            // else if (frame.Size == FrameType::ACK)
            //{
            //    log << frame.Source << " : Sending ACK  to " <<
            //    frame.Destination << " : " << frame.Ack << std::endl;
            //}
            // else
            //{
            //    log << frame.Source << " : Sending DATA to " <<
            //    frame.Destination << " : " << frame.NumberSeq << std::endl;
            //}
            m_sendingQueue.push(frame);
            return true;
        }
    }
    return false;
}

// Recupere le prochain evenement de communication a gerer pour l'envoi de
// donnees
LinkLayer::Event LinkLayer::getNextSendingEvent() {
    std::lock_guard<std::mutex> lock(m_sendEventMutex);
    if (m_sendingEventQueue.size() > 0) {
        Event ev = m_sendingEventQueue.front();
        m_sendingEventQueue.pop();
        return ev;
    }
    return Event::Invalid();
}

// Recupere le prochain evenement de communication a gerer pour la reception de
// donnees
LinkLayer::Event LinkLayer::getNextReceivingEvent() {
    std::lock_guard<std::mutex> lock(m_receiveEventMutex);
    if (m_receivingEventQueue.size() > 0) {
        Event ev = m_receivingEventQueue.front();
        m_receivingEventQueue.pop();
        return ev;
    }
    return Event::Invalid();
}

// Indique si la valeur est comprise entre first et last de facon circulaire
bool LinkLayer::between(NumberSequence value,
                        NumberSequence first,
                        NumberSequence last) const {
    // Value is between first and last, circular style
    return ((first <= value) && (value < last)) ||
           ((last < first) && (first <= value)) ||
           ((value < last) && (last < first));
}

// Envoit un evenement de communication pour indiquer a l'envoi d'envoyer un ACK
// L'evenement contiendra l'adresse a qui il faut envoyer un ACK et le numero du
// ACK
void LinkLayer::sendAck(const MACAddress& to, NumberSequence ackNumber) {
    Event ev = Event::Invalid();
    ev.Type = EventType::SEND_ACK_REQUEST;
    ev.Number = ackNumber;
    ev.Address = to;
    std::lock_guard<std::mutex> lock(m_sendEventMutex);
    m_sendingEventQueue.push(ev);
}

// Envoit un evenement de communication pour indiquer a l'envoi d'envoyer un NAK
// L'evenement contiendra l'adresse a qui il faut envoyer un ACK et le numero du
// NAK
void LinkLayer::sendNak(const MACAddress& to, NumberSequence nakNumber) {
    Event ev = Event::Invalid();
    ev.Type = EventType::SEND_NAK_REQUEST;
    ev.Number = nakNumber;
    ev.Address = to;
    std::lock_guard<std::mutex> lock(m_sendEventMutex);
    m_sendingEventQueue.push(ev);
}

// Envoit un evenement de communication pour indiquer a l'envoi qu'on a recu une
// trame avec potentiellement un ACK (piggybacking) L'evenement contiendra
// l'adresse d'ou provient l'information, le numero du ACK et le prochain ACK
// qu'on devrait nous-meme envoyer (pour le piggybacking)
void LinkLayer::notifyACK(const Frame& frame, NumberSequence piggybackAck) {
    Event ev = Event::Invalid();
    ev.Type = EventType::ACK_RECEIVED;
    ev.Number = frame.Ack;
    ev.Address = frame.Source;
    ev.Next = piggybackAck;
    std::lock_guard<std::mutex> lock(m_sendEventMutex);
    m_sendingEventQueue.push(ev);

    {
        Logger log{std::cout};
        log << "Notify ACK " << frame.Ack << " from " << frame.Source
            << " at computer: " << m_address << std::endl;
    }
}

// Envoit un evenement de communication pour indiquer a l'envoi qu'on a recu un
// NAK L'evenement contiendra l'adresse d'ou provient l'information et le numero
// du NAK
void LinkLayer::notifyNAK(const Frame& frame) {
    Event ev = Event::Invalid();
    ev.Type = EventType::NAK_RECEIVED;
    ev.Number = frame.Ack;
    ev.Address = frame.Source;
    std::lock_guard<std::mutex> lock(m_sendEventMutex);
    m_sendingEventQueue.push(ev);
}

// Envoit un evenement de communication pour indiquer au recepteur qu'on a
// atteint un timeout pour un ACK L'evenement contiendra le numero du Timer qui
// est arrive a echeance et le numero de la trame associe au Timer
void LinkLayer::ackTimeout(size_t timerID, NumberSequence numberData) {
    Event ev;
    ev.Type = EventType::ACK_TIMEOUT;
    ev.Number = numberData;
    ev.TimerID = timerID;
    std::lock_guard<std::mutex> guard(m_receiveEventMutex);
    m_receivingEventQueue.push(ev);
}

// Envoit un evenement de communication pour indiquer a l'envoi qu'on n'a aps
// recu de reponse a un envoit et qu'il faut reenvoyer la trame L'evenement
// contiendra le numero de la trame et le numero du Timer qui est arrive a
// echeance
void LinkLayer::transmissionTimeout(size_t timerID, NumberSequence numberData) {
    Event ev;
    ev.Type = EventType::SEND_TIMEOUT;
    ev.Number = numberData;
    ev.TimerID = timerID;
    std::lock_guard<std::mutex> guard(m_sendEventMutex);
    m_sendingEventQueue.push(ev);
}

// Demarre un nouveau Timer d'attente pour l'envoi a nouveau d'une trame
// La methode retourne le numero du Timer qui vient d'etre demarre. Cette valeur
// doit etre garder pour pouvoir retrouver quel evenement y sera associe lorsque
// le timer arrivera a echeance
size_t LinkLayer::startTimeoutTimer(NumberSequence numberData) {
    return m_timers->addTimer(m_transmissionTimeout,
                              std::bind(&LinkLayer::transmissionTimeout,
                                        this,
                                        std::placeholders::_1,
                                        std::placeholders::_2),
                              numberData);
}

// Demarre un nouveau Timer pour l'envoi d'un ACK, pour garantir un niveau de
// service minimal dans une communication unidirectionnelle Retourne le numero
// du Timer qui vient d'etre demarre. La methode prend en parametre le numero
// actuel du Timer de ACK afin de le redemarrer s'il existe encore
size_t LinkLayer::startAckTimer(size_t existingTimerID,
                                NumberSequence ackNumber) {
    if (!m_timers->restartTimer(existingTimerID, ackNumber)) {
        return m_timers->addTimer(m_ackTimeout,
                                  std::bind(&LinkLayer::ackTimeout,
                                            this,
                                            std::placeholders::_1,
                                            std::placeholders::_2),
                                  ackNumber);
    }
    return existingTimerID;
}

// Envoit un evenement de communication pour indiquer a la fonction de reception
// qu'une ACK vient d'etre envoyer (en piggybacking) et qu'on n'a pas besoin
// d'envoyer le ACK en attente
void LinkLayer::notifyStopAckTimers(const MACAddress& to) {
    Event ev;
    ev.Type = EventType::STOP_ACK_TIMER_REQUEST;
    ev.Address = to;
    std::lock_guard<std::mutex> guard(m_receiveEventMutex);
    m_receivingEventQueue.push(ev);
}

// Arrete le Timer de ACK avec le TimerID specifie
void LinkLayer::stopAckTimer(size_t timerID) {
    m_timers->removeTimer(timerID);
}

// Indique s'il y a assez de place dans le buffer de reception pour recevoir des
// donnees de la couche physique
bool LinkLayer::canReceiveDataFromPhysicalLayer(const Frame& data) const {
    return m_receivingQueue.canWrite<Frame>(data);
}

// Recoit des donnees de la couche physique
void LinkLayer::receiveData(Frame data) {
    // Si la couche est pleine, la trame est perdue. Elle devra etre envoye a
    // nouveau par l'emetteur
    if (canReceiveDataFromPhysicalLayer(data)) {
        // Est-ce que la trame re�ue est pour nous?
        if (data.Destination == m_address || data.Destination.isMulticast()) {
            m_receivingQueue.push(data);
        }
    }
}

// Fonction qui retourne l'adresse MAC du destinataire d'un packet particulier
// de la couche Reseau. Dans la realite, cette fonction ferait un lookup dans
// une table a partir des adresses IP pour recupere les addresse MAC. Ici, on
// utilise directement seulement les adresse MAC.
MACAddress LinkLayer::arp(const Packet& packet) const {
    return packet.Destination;
}

// Fonction qui fait l'envoi des trames et qui gere la fenetre d'envoi
void LinkLayer::senderCallback() {
    const NumberSequence max_sequence_plus_one = m_maximumSequence + 1;
    using timers_t = std::map<NumberSequence, size_t>;

    std::map<MACAddress, window_t> windows_by_address{};
    std::map<MACAddress, NumberSequence> ack_expected_by_address{};
    std::map<MACAddress, NumberSequence> next_frame_by_address{};
    std::map<MACAddress, timers_t> timers_by_address{};
    std::map<size_t, MACAddress> address_by_timer_id;

    std::map<MACAddress, std::queue<Packet>> outgoing_buffers;

    while (m_executeSending) {
        const Event event = getNextSendingEvent();
        switch (event.Type) {
        case EventType::SEND_ACK_REQUEST: {
            Frame ack{};
            ack.Destination = event.Address;
            ack.Source = m_address;
            ack.Ack = static_cast<NumberSequence>(event.Number);
            ack.Size = FrameType::ACK;

            {
                Logger log{std::cout};
                log << "Sending ACK " << ack.Ack << " to " << ack.Destination
                    << std::endl;
            }

            sendFrame(ack);
            break;
        }
        case EventType::SEND_NAK_REQUEST: {
            Frame nak{};
            nak.Destination = event.Address;
            nak.Source = m_address;
            nak.Ack = static_cast<NumberSequence>(event.Number);
            nak.Size = FrameType::NAK;

            {
                Logger log{std::cout};
                log << "Sending NAK " << nak.Ack << " to " << nak.Destination
                    << std::endl;
            }

            sendFrame(nak);
            break;
        }

        case EventType::SEND_TIMEOUT: {
            const auto address_it = address_by_timer_id.find(event.TimerID);
            if (address_it == address_by_timer_id.end()) {
                break;
            }

            const MACAddress address = address_it->second;
            const auto window_it = windows_by_address.find(address);
            if (window_it == windows_by_address.end()) {
                break;
            }

            const NumberSequence number = event.Number;

            {
                Logger log{std::cout};
                log << "Send timeout for " << number << std::endl;
            }

            window_t& window = window_it->second;
            timers_t& timers = timers_by_address[address];

            const auto frame_it = window.find(number);
            if (frame_it == window.end()) {
                break;
            }

            sendFrame(frame_it->second);
            if (!m_timers->restartTimer(timers[number], number)) {
                address_by_timer_id.erase(address_it);
                const size_t timer_id = startTimeoutTimer(number);
                timers[number] = timer_id;
                address_by_timer_id[timer_id] = address;
            }

            break;
        }

        case EventType::NAK_RECEIVED: {
            const auto window_it = windows_by_address.find(event.Address);
            if (window_it == windows_by_address.end()) {
                break;
            }

            const NumberSequence nak = event.Number;

            {
                Logger log{std::cout};
                log << "Received NAK " << nak << " from " << event.Address
                    << std::endl;
            }

            window_t& window = window_it->second;
            const auto frame_it = window.find(nak);
            if (frame_it == window.end()) {
                break;
            }

            timers_t& timers = timers_by_address[event.Address];
            sendFrame(frame_it->second);

            const auto timer_it = timers.find(nak);
            if (timer_it == timers.end()) {
                break;
            }

            const size_t timer_id = timer_it->second;
            if (!m_timers->restartTimer(timer_id, nak)) {
                timers[nak] = startTimeoutTimer(nak);
            }

            break;
        }
        case EventType::ACK_RECEIVED: {
            const NumberSequence ack = event.Number;
            const MACAddress address = event.Address;
            {
                Logger log{std::cout};
                log << "Received ACK " << ack << " from " << address
                    << std::endl;
            }

            const auto window_it = windows_by_address.find(address);
            if (window_it == windows_by_address.end()) {
                Logger log{std::cout};
                log << "ACK received but no window for address!" << std::endl;
                break;
            }

            window_t& window = window_it->second;
            timers_t& timers = timers_by_address[address];
            NumberSequence& ack_expected = ack_expected_by_address[address];
            NumberSequence& next_frame = next_frame_by_address[address];

            while (between(ack, ack_expected, next_frame) &&
                   window.count(ack_expected)) {
                {
                    Logger log{std::cout};
                    log << "Handled ACK " << ack_expected << std::endl;
                }

                m_timers->removeTimer(timers[ack_expected]);
                window.erase(ack_expected);
                timers.erase(ack_expected);

                ack_expected = (ack_expected + 1) % (max_sequence_plus_one);
            }

            break;
        }

        default:
            break;
        }

        // Drain all network packets
        while (m_driver->getNetworkLayer().dataReady()) {
            const Packet packet = m_driver->getNetworkLayer().getNextData();
            outgoing_buffers[packet.Destination].push(packet);
        }

        // Handle all outgoing buffers
        for (auto& [destination, queue] : outgoing_buffers) {
            if (queue.empty()) {
                continue;
            }

            if (!windows_by_address.count(destination)) {
                windows_by_address[destination] = window_t{};
                timers_by_address[destination] = timers_t{};
                ack_expected_by_address[destination] = 0;
                next_frame_by_address[destination] = 0;
            }

            window_t& window = windows_by_address[destination];
            timers_t& timers = timers_by_address[destination];
            NumberSequence& next_frame = next_frame_by_address[destination];

            // No more space for this destination
            if (window.size() >= m_maximumBufferedFrameCount) {
                continue;
            }

            Packet packet = queue.front();
            queue.pop();

            {
                Logger log{std::cout};
                log << "Network Layer Data Ready, sending to " << destination
                    << std::endl;
            }

            Frame frame{};
            frame.Destination = destination;
            frame.Source = m_address;
            frame.NumberSeq = next_frame;
            frame.Data = Buffering::pack(packet);
            frame.Size = frame.Data.size();
            frame.Ack = NO_ACK;

            // Check if there is a pending ACK to piggyback for the destination
            std::lock_guard<std::mutex> lock{m_mutex};
            const auto pending_it = m_pending_acks.find(destination);
            if (pending_it != m_pending_acks.end()) [[likely]] {
                frame.Ack = pending_it->second.AckNumber;
                m_pending_acks.erase(pending_it);
                notifyStopAckTimers(frame.Destination);

                {
                    Logger log{std::cout};
                    log << "Piggybacked ACK " << frame.Ack << std::endl;
                }
            }

            if (sendFrame(frame)) {
                {
                    Logger log{std::cout};
                    log << "Correctly sent packet #" << frame.NumberSeq
                        << " to " << frame.Destination << std::endl;
                }

                const std::size_t timer_id = startTimeoutTimer(next_frame);
                window[next_frame] = frame;
                timers[next_frame] = timer_id;
                address_by_timer_id[timer_id] = destination;
                next_frame = (next_frame + 1) % (max_sequence_plus_one);
            }
        }
    }
}

// Fonction qui s'occupe de la reception des trames
void LinkLayer::receiverCallback() {
    const NumberSequence max_sequence_plus_one = m_maximumSequence + 1;
    std::size_t next_timer_id{0};

    std::map<MACAddress, window_t> windows_by_address{};
    std::map<MACAddress, NumberSequence> ack_expected_by_address{};
    std::map<MACAddress, std::size_t> timer_id_by_address{};

    std::map<std::size_t, MACAddress> address_by_timer_id{};

    while (m_executeReceiving) {
        const Event event = getNextReceivingEvent();
        switch (event.Type) {
        case EventType::STOP_ACK_TIMER_REQUEST: {
            auto it = timer_id_by_address.find(event.Address);
            if (it == timer_id_by_address.end()) {
                break;
            }

            {
                Logger log{std::cout};
                log << "Ack timer stop for " << event.Address << std::endl;
            }

            stopAckTimer(it->second);
            break;
        }

        case EventType::ACK_TIMEOUT: {
            const auto address_it = address_by_timer_id.find(event.TimerID);
            if (address_it == address_by_timer_id.end()) {
                break;
            }

            const MACAddress address = address_it->second;
            address_by_timer_id.erase(address_it);

            std::lock_guard<std::mutex> lock{m_mutex};
            const auto pending_it = m_pending_acks.find(address);
            if (pending_it == m_pending_acks.end()) {
                break;
            }

            const PendingAck pending = pending_it->second;
            m_pending_acks.erase(pending_it);

            {
                Logger log{std::cout};
                log << "Ack timeout for " << pending.AckNumber
                    << " Timer ID: " << event.TimerID << std::endl;
            }

            sendAck(pending.Address, pending.AckNumber);
            break;
        }

        default:
            break;
        }

        if (!m_receivingQueue.canRead<Frame>()) {
            continue;
        }

        const Frame frame = m_receivingQueue.pop<Frame>();
        if (frame.Size == FrameType::ACK) {
            notifyACK(frame, frame.Ack);
            continue;
        }

        if (frame.Size == FrameType::NAK) {
            notifyNAK(frame);
            continue;
        }

        {
            Logger log{std::cout};
            log << "Received data from " << frame.Source << std::endl;
        }

        if (!windows_by_address.count(frame.Source)) {
            windows_by_address[frame.Source] = window_t{};
            ack_expected_by_address[frame.Source] = 0;
        }

        window_t& window = windows_by_address[frame.Source];
        NumberSequence& ack_expected = ack_expected_by_address[frame.Source];

        // Check if inside window
        const NumberSequence upper_bound = (ack_expected +
                                            m_maximumBufferedFrameCount) %
                                           max_sequence_plus_one;

        const bool in_window = between(frame.NumberSeq,
                                       ack_expected,
                                       upper_bound);

        // Duplicate frame, just resend ACK
        const NumberSequence lower_old = (ack_expected + max_sequence_plus_one -
                                          m_maximumBufferedFrameCount) %
                                         max_sequence_plus_one;

        if ((in_window && window.count(frame.NumberSeq)) ||
            (!in_window && between(frame.NumberSeq, lower_old, ack_expected))) {
            {
                Logger log{std::cout};
                log << "Received duplicate frame, sending last ack"
                    << frame.NumberSeq << std::endl;
            }

            const NumberSequence last_ack = (ack_expected - 1 +
                                             max_sequence_plus_one) %
                                            max_sequence_plus_one;
            sendAck(frame.Source, last_ack);
            continue;
        }

        // Still outside of window
        if (!in_window) {
            {
                Logger log{std::cout};
                log << "Received frame outside of window, ignoring "
                    << frame.NumberSeq << " " << ack_expected << " "
                    << upper_bound << std::endl;
            }

            sendNak(frame.Source, ack_expected);
            continue;
        }

        // Notify pigyback ACK if there is one
        if (frame.Ack != NO_ACK) {
            {
                Logger log{std::cout};
                log << "Received piggyback ACK " << frame.Ack << std::endl;
            }

            notifyACK(frame, frame.Ack);
        }

        if (frame.NumberSeq != ack_expected) {
            {
                Logger log{std::cout};
                log << "Received frame out of order, expected " << ack_expected
                    << " but got " << frame.NumberSeq << std::endl;
            }

            sendNak(frame.Source, ack_expected);
        }

        window[frame.NumberSeq] = frame;

        // Deliver packets in order to upper layer
        std::lock_guard<std::mutex> lock{m_mutex};
        while (window.count(ack_expected)) {
            const Frame next_frame = window[ack_expected];

            const Packet packet = Buffering::unpack<Packet>(
                window[ack_expected].Data);

            m_driver->getNetworkLayer().receiveData(packet);
            window.erase(ack_expected);

            std::size_t timer_id = next_timer_id;
            if (timer_id_by_address.count(next_frame.Source)) {
                timer_id = timer_id_by_address[next_frame.Source];
            } else {
                next_timer_id++;
            }

            {
                Logger log{std::cout};
                log << "Delivering packet #" << next_frame.NumberSeq << " from "
                    << next_frame.Source << std::endl;
            }

            m_pending_acks[next_frame.Source] = PendingAck{next_frame.Source,
                                                           ack_expected};

            ack_expected = (ack_expected + 1) % max_sequence_plus_one;
            timer_id = startAckTimer(timer_id, next_frame.NumberSeq);
            address_by_timer_id[timer_id] = next_frame.Source;
            timer_id_by_address[next_frame.Source] = timer_id;
            next_timer_id = std::max(next_timer_id, timer_id + 1);
        }
    }
}
