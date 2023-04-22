#include <stream_reader.hpp>

#include <logger.hpp>

#include <chrono>

using namespace common;
using namespace stream;
using namespace reader;

///////////// IStreamReader /////////////

IStreamReader::IStreamReader(size_t start, size_t end)
    : start_(start), end_(end) {
    url_ = std::make_shared<std::string>();
    InitThread();
    InitParams();
}

void IStreamReader::InitThread() {
    thread_ = std::make_shared<hv::EventLoopThread>();
}

void IStreamReader::InitParams() {
    size_t count = end_ - start_;
    market_names_.clear();
    market_names_.reserve(count);
    reconn_setting_init(&settings_);
    settings_.min_delay = 1000;
    settings_.max_delay = 10000;
    settings_.delay_policy = 1;
}

void IStreamReader::InitWebSocket() {
    websocket_ = std::make_shared<hv::WebSocketClient>(thread_->loop());
    websocket_->onopen = [url = url_]() {
        /*WSOpenFunction(*url);*/
        // ToDo Init LogMaster thread log
    };
    websocket_->onclose = [url = url_]() { /*WSCloseFunction(*url);*/ };
    websocket_->onmessage = [handler = handler_](const std::string& msg) {
        handler->HandleMessage(msg);
    };
    websocket_->setReconnect(&settings_);
}

void IStreamReader::Start() {
    InitMarketNames();
    InitHandler();
    InitURL();
    InitMessage();
    InitWebSocket();
    Log("Start reading stream and updating quotes");
    StartThread();
    StartWebsocket();
}

void IStreamReader::StartThread() { thread_->start(); }

void IStreamReader::StartWebsocket() {
    LogMore("Open websocket with url:", *url_, '\n');
    websocket_->open(url_->c_str());
    if (!message_.empty()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        websocket_->send(message_);
    }
    Log("from", start_, "to", end_, "--quotes-- reading streams are open");
}

void IStreamReader::Join() {
    Log("Joining IStreamReader");
    websocket_->close();
    std::this_thread::sleep_for(std::chrono::milliseconds(3000));
    JoinThread();
    Log("IStreamReader Joined");
}

void IStreamReader::JoinThread() {
    thread_->stop();
    thread_->join();
}
