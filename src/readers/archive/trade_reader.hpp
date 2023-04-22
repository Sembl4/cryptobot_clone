#pragma once

#include <WebSocketClient.h>

#include <memory>

namespace binance::readers::trade {

class TradeReader {
  public:
    TradeReader(size_t start, size_t end, size_t threads_n);

    void Start();
    void Join();

  private:
    void InitThreads(size_t n);
    void InitParams();
    void InitWebSockets();

    void StartThreads();
    void JoinThreads();

  private:
    size_t start_;
    size_t end_;
    std::vector<std::shared_ptr<hv::EventLoopThread>> thread_list_;
    std::vector<std::shared_ptr<hv::WebSocketClient>> websockets_;
    reconn_setting_t settings_;
    std::vector<std::string> markets_name_;
};

}  // namespace binance::readers::trade