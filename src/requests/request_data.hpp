#pragma once

#include <quote.hpp>

#include <string_view>

namespace common::request {

struct Volume {
    double start;
    double middle;
    double fin;
};

enum Operation {
    SELL = 0,
    BUY = 1,
};

struct FilterBase {
    double stepSize;
    double minNotional;
    double tickSize;
};

struct Order {
    size_t market_id;
    std::string_view symbols_pair;
    Rate rate;
    Operation op_type;

    bool isQuoteVolume = false;
    Rate ActualRate() const;
    double ActualPrice() const;
    double ActualVolume() const;
    [[nodiscard]] bool MatchPrice() const;
    bool Decrease() const;
    FilterBase filters;
};

struct RequestData {
    Order start;
    Order middle;
    Order fin;

    uint64_t ID;

    bool CompareAndDecrease() const;
    void PrintMarketsOps() const;
};

}  // namespace binance::request
