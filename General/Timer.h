#ifndef _GENERAL_TIMER_H_
#define _GENERAL_TIMER_H_

#include <atomic>
#include <chrono>
#include <functional>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include "../Computer/Driver/Layer/DataType.h"

class Timer {
    struct TimerInfo {
        size_t ID;
        NumberSequence NumberData;
        std::chrono::milliseconds Interval;
        std::chrono::system_clock::time_point Start;
        std::chrono::system_clock::time_point NextTick;
        std::function<void(size_t, NumberSequence)> Function;
    };

    // Permet de construire le monceau en mettant la plus petite valeur au
    // début. Le prochain Timer est celui dont le prochain tick est le plus
    // petit.
    struct TimerInfoComparer {
        bool operator()(const TimerInfo& first, const TimerInfo& second) const {
            return first.NextTick > second.NextTick;
        }
    };

    size_t m_nextID;
    TimerInfoComparer m_comparer;
    std::vector<TimerInfo> m_timersHeap;

    std::atomic<bool> m_timerRunning;
    std::mutex m_lockMutex;
    std::thread m_timerThread;

    void innerTimer();

public:
    static constexpr size_t InvalidTimerID = 0;

    Timer();
    ~Timer();

    void start();
    void stop();

    size_t addTimer(std::chrono::milliseconds interval,
                    std::function<void(size_t, NumberSequence)> function,
                    NumberSequence numberData);
    bool restartTimer(size_t timerID, NumberSequence numberData);
    void removeTimer(size_t id);
};

#endif //_GENERAL_TIMER_H_
