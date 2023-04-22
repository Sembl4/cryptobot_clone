#pragma once

#include <spinlock.hpp>

#include <chrono>
#include <fstream>
#include <iostream>

#ifdef NO_LOG
#define LOG(...)
#define LOG_DEBUG(...)

#elif DEBUG
#define Log(...) LogDebug(__VA_ARGS__)
#define LogDebug(...)                                                       \
    Logger::GetLogger().log_debug(Logger::FileName(__FILE__), __FUNCTION__, \
                                  __LINE__, ":", __VA_ARGS__)

#else
#define Log(...) Logger::GetLogger().log(__VA_ARGS__)
#define LogDebug(...)
#endif

#ifdef CTIME
#define TIMING CTime()

#elif TIMESTAMP
#define TIMING TimeStamp()

#else
#define TIMING TimeStamp()
#endif

#define LogMore(...) Logger::GetLogger().logMore(__VA_ARGS__)
#define LogFlush() Logger::GetLogger().flush()

template <typename T>
int64_t TTimestamp() {
    return std::chrono::duration_cast<T>(std::chrono::system_clock::now().time_since_epoch()).count();
}

struct Logger {
    static Logger& GetLogger() {
        static Logger logger;
        return logger;
    }

  private:
    Logger() { Init(); }
    void Init() {
        // ToDo: add different files
    }

  private:
    utils::SpinLock lock;

  public:
    template <typename... Args>
    void log(const Args&... args) {
        auto guard = lock.Guard();
        std::cout << "LOG:" << TIMING;
        LogPrivate(args...);
        std::cout << '\n';
    }

    template <typename... Args>
    void logMore(const Args&... args) {
        auto guard = lock.Guard();
        LogPrivate(args...);
    }

    template <typename... Args>
    void log_debug(const Args&... args) {
        auto guard = lock.Guard();
        std::cout << "DEBUG:" << TIMING;
        LogPrivate(args...);
        std::cout << '\n';
    }

    void flush() {
        auto guard = lock.Guard();
        std::cout << std::flush;
    }

  private:
    template <typename T>
    static void LogPrivate(const T& msg) {
        std::cout << ' ' << msg;
    }

    template <typename T, typename... Args>
    static void LogPrivate(const T& first, const Args&... args) {
        LogPrivate(first);
        LogPrivate(args...);
    }

    static std::string CTime() {
        std::string result = __TIME__;
        result += ":";
        return result;
    }

    static std::string TimeStamp() {
        auto timestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock().now().time_since_epoch())
                .count();
        std::string result = std::to_string(timestamp) + ":";
        return result;
    }

  public:
    static std::string FileName(const char* name) {
        std::string full_path = name;
        size_t i;
        for (i = full_path.size() - 1; i > 0; --i) {
            if (full_path[i] == '/') {
                break;
            }
        }
        return full_path.substr(i);
    }
};
