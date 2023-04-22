#pragma once

#include <string>

namespace common::config {

namespace detail {

struct MarketConfig {
    std::string no_fee_markets_file_path;
    std::string working_asset;
    std::string trading_platform;
};

struct OrderConfig {
    std::string api_key;
    std::string secret_key;
    std::string pass_phrase;
    uint64_t await_time;
    int32_t repeats_count;
    int32_t pause_between_orders;
    int32_t orders_type;  // 0 or 1
    bool print_answers;
};

struct LimitConfig {
    double order_start_limit;
    double fee_step;
    std::string run_type;
};

struct StreamConfig {
    size_t bids_index;
    size_t asks_index;
};

}  // namespace detail

class Config {
  public:
    static const detail::MarketConfig& Market();
    static const detail::OrderConfig& Order();
    static const detail::LimitConfig& Limits();
    static const detail::StreamConfig& Stream();

  private:
    static const Config& Get();

  private:
    Config();

    void Init();
    void InitMarket();
    void InitOrder();
    void InitLimit();
    void InitStream();

  private:
    detail::MarketConfig market_;
    detail::OrderConfig order_;
    detail::LimitConfig limit_;
    detail::StreamConfig stream_;
};

}  // namespace binance::config
