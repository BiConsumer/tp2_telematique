#ifndef _GENERAL_LOGGER_H_
#define _GENERAL_LOGGER_H_

#include <ostream>
#include <sstream>
#include <mutex>

class Logger {
    std::stringstream ss;
    std::ostream& stream;
	std::mutex m_mutex;

public:
    Logger(std::ostream& out) : stream(out) {}
    ~Logger() {
        stream << ss.str();
        stream.flush();
    }

    template <typename T>
    Logger& operator<<(const T& val) {
		std::lock_guard<std::mutex> lock(m_mutex);
        ss << val;
        return *this;
    }

    // Pour le support de std::endl et cie
    Logger& operator<<(std::ostream& (*f)(std::ostream&)) {
		std::lock_guard<std::mutex> lock(m_mutex);
        ss << f;
        return *this;
    }
};

#endif //_GENERAL_LOGGER_H_
