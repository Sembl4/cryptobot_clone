#pragma once

#include <WebSocketClient.h>
#include <quote.hpp>

#include <memory>

namespace binance {

class StreamReader {
  public:
    StreamReader(size_t start, size_t end);

    void Start();
    void Join();

  private:
    void Init();

  private:
    size_t start_;
    size_t end_;
    hv::EventLoopThread thread_;
    std::vector<std::shared_ptr<hv::WebSocketClient>> websockets_;
    std::vector<reconn_setting_t> settings_;
    std::vector<std::string> currency_pair_name_;
};

}  // namespace binance