#include <logg.hpp>

#include <cassert>

using namespace tools;
using namespace log;
using namespace detail;

using std::chrono::milliseconds;

static const constexpr size_t BUFFER_SIZE = 1000000;
static const constexpr int64_t WORK_PAUSE = 3000;  // ms
static const constexpr int64_t JOIN_PAUSE = 1000;  // ms

///////////// LogMessage /////////////

LogMessage::LogMessage(std::thread::id id, std::string&& msg)
    : thread_id(id), message(std::move(msg)) {}

///////////// LogQueue /////////////

bool LogQueue::Put(LogMessage msg) {
    std::lock_guard lock(spinlock_);
    if (closed_) {
        return false;
    }
    queue_.emplace_back(std::move(msg));
    return true;
}

std::optional<LogMessage> LogQueue::Take() {
    std::unique_lock lock(spinlock_);
    if (queue_.empty()) {
        return std::nullopt;
    }

    LogMessage value = std::move(queue_.front());
    queue_.pop_front();
    return std::move(value);
}

bool LogQueue::IsEmpty() {
    std::unique_lock lock(spinlock_);
    return queue_.empty();
}

void LogQueue::Clear() {
    std::unique_lock lock(spinlock_);
    queue_.clear();
}

void LogQueue::Close() {
    std::unique_lock lock(spinlock_);
    closed_ = true;
}

bool LogQueue::IsClosed() {
    std::unique_lock lock(spinlock_);
    return closed_;
}

///////////// LogMaster /////////////

LogMaster::LogMaster() : name_suffix_("(") {
    name_suffix_ += __TIME__;
    name_suffix_ += ").log.txt";
    error_file_ = std::make_shared<LogFile>("Errors" + name_suffix_, 20000);
}

void LogMaster::InitLog(std::string name, std::thread::id id) {
    name += name_suffix_;
    auto find = name_map_id_.find(name);
    if (find != name_map_id_.end()) {  /// Exist such Log
        auto initial_id = find->second;
        if (!id_map_file_.contains(initial_id)) {
            std::cerr
                << "Couldn't find id in id_map_file, which is impossible\n";
            assert(false);
        }
        auto file_index = id_map_file_[initial_id];
        id_map_file_.insert({id, file_index});
        return;
    }
    /// NewLogFile
    auto file_index = files_.size();
    files_.emplace_back(name, BUFFER_SIZE);
    name_map_id_.insert({name, id});
    id_map_name_.insert({id, name});
    id_map_file_.insert({id, file_index});
}

void LogMaster::WorkLoop() {
    while (!stop_.load()) {
        auto msg_option = std::move(queue_.Take());
        if (msg_option.has_value()) {
            auto msg = std::move(msg_option.value());
            try {
                auto file_index = id_map_file_.at(msg.thread_id);
                files_[file_index].Write(std::move(msg.message));
            } catch (...) {
                std::cerr << "No such file in files_list, LogMaster error\n";
            }
            continue;
        }
        std::this_thread::sleep_for(milliseconds(WORK_PAUSE));
    }
}

void LogMaster::Start() {
    stop_.store(false);
    thread_ = std::make_shared<std::jthread>([this]() { WorkLoop(); });
}

void LogMaster::Join() {
    queue_.Close();
    while (!queue_.IsEmpty()) {
        std::this_thread::sleep_for(milliseconds(JOIN_PAUSE));
    }
    stop_.store(true);
    if (thread_ == nullptr) {
        return;
    }
    thread_->join();
}

LogMaster::~LogMaster() { Join(); }

LogMaster& LogMaster::Get() {
    static LogMaster master;
    return master;
}

void LogMaster::Concat(std::string& str, int32_t item) {
    str += std::to_string(item);
}

void LogMaster::Concat(std::string& str, uint32_t item) {
    str += std::to_string(item);
}

void LogMaster::Concat(std::string& str, int64_t item) {
    str += std::to_string(item);
}

void LogMaster::Concat(std::string& str, uint64_t item) {
    str += std::to_string(item);
}
void LogMaster::Concat(std::string& str, double item) {
    str += std::to_string(item);
}
void LogMaster::Concat(std::string& str, std::string& item) { str += item; }

void LogMaster::Concat(std::string& str, std::string&& item) { str += item; }
