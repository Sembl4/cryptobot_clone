#pragma once

#include <WebSocketClient.h>
#include <quote.hpp>

#include <memory>

namespace binance {

namespace stream {

//class StreamReader {
//  public:
//    StreamReader(size_t start, size_t end);
//
//    void Start();
//
//    void Join();
//
//  private:
//    void InitThread();
//    void InitParams();
//    void InitWebSocket();
//
//    void StartThread();
//    void JoinThread();
//
//  private:
//    size_t start_;
//    size_t end_;
//    std::shared_ptr<hv::EventLoopThread> thread_;
//    std::shared_ptr<hv::WebSocketClient> websocket_;
//    std::shared_ptr<std::string> url_;
//    reconn_setting_t settings_;
//    std::vector<std::string> market_names_;
//};

}  // namespace stream

}  // namespace binance