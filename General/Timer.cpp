#include "Timer.h"

#include <algorithm>

Timer::Timer() : m_timerRunning(false), m_nextID(1) {}

Timer::~Timer() {
    stop();
}

void Timer::innerTimer() {
    while (m_timerRunning) {
        std::lock_guard<std::mutex> lockGuard(m_lockMutex);
        if (m_timersHeap.size() > 0) {
            auto now = std::chrono::system_clock::now();
            TimerInfo& info = m_timersHeap.front();
            while (now > info.NextTick) {
                info.Function(info.ID, info.NumberData);
                // Enleve le timer et recupere le prochain
                std::pop_heap(m_timersHeap.begin(),
                              m_timersHeap.end(),
                              m_comparer);
                m_timersHeap.pop_back();
                if (m_timersHeap.size() > 0) {
                    info = m_timersHeap.front();
                } else {
                    break;
                }
            }
        }
    }
}

void Timer::start() {
    stop();
    m_timerRunning = true;
    // Initialize tous les timers deja enregistres
    for (auto it = m_timersHeap.begin(); it != m_timersHeap.end(); ++it) {
        (*it).Start = std::chrono::system_clock::now();
        (*it).NextTick = (*it).Start + (*it).Interval;
    }
    // Ordonne les timers par priorite
    std::make_heap(m_timersHeap.begin(), m_timersHeap.end(), m_comparer);
    m_timerThread = std::thread(&Timer::innerTimer, this);
}

void Timer::stop() {
    m_timerRunning = false;
    if (m_timerThread.joinable()) {
        m_timerThread.join();
    }
}

size_t Timer::addTimer(std::chrono::milliseconds interval,
                       std::function<void(size_t, NumberSequence)> function,
                       NumberSequence numberData) {
    TimerInfo info;
    info.Interval = interval;
    info.Function = function;
    info.NumberData = numberData;
    info.Start = std::chrono::system_clock::now();
    info.NextTick = info.Start + info.Interval;

    std::lock_guard<std::mutex> lockGuard(m_lockMutex);
    info.ID = m_nextID++;
    m_timersHeap.push_back(info);
    std::push_heap(m_timersHeap.begin(), m_timersHeap.end(), m_comparer);
    return info.ID;
}

bool Timer::restartTimer(size_t timerID, NumberSequence numberData) {
    std::lock_guard<std::mutex> lockGuard(m_lockMutex);
    for (auto it = m_timersHeap.begin(); it != m_timersHeap.end(); ++it) {
        if ((*it).ID == timerID) {
            (*it).Start = std::chrono::system_clock::now();
            (*it).NextTick = (*it).Start + (*it).Interval;
            (*it).NumberData = numberData;
            std::iter_swap(it, --m_timersHeap.end());
            std::make_heap(m_timersHeap.begin(),
                           m_timersHeap.end(),
                           m_comparer);
            return true;
        }
    }
    return false;
}

void Timer::removeTimer(size_t id) {
    std::lock_guard<std::mutex> lockGuard(m_lockMutex);
    for (auto it = m_timersHeap.begin(); it != m_timersHeap.end(); ++it) {
        if (it->ID == id) {
            std::iter_swap(it, --m_timersHeap.end());
            m_timersHeap.pop_back();
            std::make_heap(m_timersHeap.begin(),
                           m_timersHeap.end(),
                           m_comparer);
            return;
        }
    }
}
