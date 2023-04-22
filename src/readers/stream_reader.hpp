#pragma once

#include <WebSocketClient.h>

#include <concepts>
#include <memory>

namespace common::stream::reader {

template <typename T>
concept WSHandler = requires(T h, std::string b) {
    h.HandleMessage(b);
};

namespace handler {
class IHandler {
public:
    virtual void HandleMessage(const std::string& msg) = 0;
};
}  // namespace handler

class IStreamReader {
  public:
    IStreamReader(size_t start, size_t end);

    void Start();

    void Join();

  protected:
    void InitThread();
    void InitParams();
    void InitWebSocket();
    virtual void InitMarketNames() = 0;
    virtual void InitURL() = 0;
    virtual void InitMessage() = 0;
    virtual void InitHandler() = 0;

    void StartThread();
    void StartWebsocket();
    void JoinThread();

  protected:
    size_t start_;
    size_t end_;
    std::shared_ptr<hv::EventLoopThread> thread_;
    std::shared_ptr<hv::WebSocketClient> websocket_;
    std::shared_ptr<handler::IHandler> handler_;
    std::shared_ptr<std::string> url_;
    reconn_setting_t settings_;
    std::vector<std::string> market_names_;
    std::string message_;
};

}  // namespace stream::reader