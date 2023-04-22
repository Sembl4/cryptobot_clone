#pragma once

#include <log_file.hpp>
#include <spinlock.hpp>

#include <deque>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <thread>
#include <unordered_map>

#define LOG(...)                                                        \
    tools::log::LogMaster::Get().LogMessage(std::this_thread::get_id(), \
                                            __VA_ARGS__)

#define LOG_ERROR(...) tools::log::LogMaster::Get().LogError(__VA_ARGS__)

namespace tools::log {

namespace detail {

struct LogMessage {
    std::thread::id thread_id;
    std::string message;

    LogMessage(std::thread::id id, std::string&& msg);
    LogMessage(const LogMessage& other) = delete;
    LogMessage(LogMessage&& other) = default;
    LogMessage& operator=(const LogMessage& other) = delete;
    LogMessage& operator=(LogMessage&& other) = delete;
};

class LogQueue {
  public:
    LogQueue() = default;

    bool Put(LogMessage msg);
    std::optional<LogMessage> Take();
    bool IsEmpty();
    void Clear();
    void Close();
    bool IsClosed();

  private:
    std::deque<LogMessage> queue_;
    utils::SpinLock spinlock_;
    bool closed_{false};
};

}  // namespace detail

class LogMaster {
  public:
    LogMaster(const LogMaster&) = delete;
    LogMaster(LogMaster&&) = delete;
    LogMaster& operator=(const LogMaster&) = delete;
    LogMaster& operator=(LogMaster&&) = delete;
    ~LogMaster();

    static LogMaster& Get();
    void InitLog(std::string name, std::thread::id);
    void Start();

  private:
    LogMaster();
    void Join();
    void WorkLoop();

  public:
    template <typename... Args>
    void LogMessage(std::thread::id id, Args&&... args) {
        std::string str;
        str.reserve(300);
        LogPrivate(str, std::forward<Args>(args)...);
        detail::LogMessage message(id, std::move(std::move(str)));
        queue_.Put(std::move(message));
    }

    template <typename... Args>
    void LogError(Args&&... args) {
        std::stringstream stream;
        LogPrivate(stream, std::forward<Args>(args)...);
        error_file_->Write(std::move(std::move(stream).str()));
        error_file_->Flush();
    }

  private:
    template <class T>
    void LogPrivate(std::string& stream, T&& item) {
        Concat(stream, std::forward<T>(item));
    }

    template <typename T, typename... Args>
    void LogPrivate(std::string& stream, T&& item, Args&&... args) {
        Concat(stream, std::forward<T>(item));
        LogPrivate(stream, std::forward<Args>(args)...);
    }

    static void Concat(std::string& str, int32_t item);
    static void Concat(std::string& str, uint32_t item);
    static void Concat(std::string& str, int64_t item);
    static void Concat(std::string& str, uint64_t item);
    static void Concat(std::string& str, double item);
    static void Concat(std::string& str, std::string& item);
    static void Concat(std::string& str, std::string&& item);

  private:
    detail::LogQueue queue_;
    std::unordered_map<std::thread::id, size_t> id_map_file_;
    std::unordered_map<std::thread::id, std::string> id_map_name_;
    std::unordered_map<std::string, std::thread::id> name_map_id_;
    std::vector<log::LogFile> files_;
    std::string name_suffix_;
    std::shared_ptr<std::jthread> thread_;
    std::atomic<bool> stop_{true};
    std::shared_ptr<LogFile> error_file_;
};

}  // namespace tools::log