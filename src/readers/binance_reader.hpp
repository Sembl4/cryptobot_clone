#pragma once

#include <quote.hpp>
#include <stream_reader.hpp>

namespace binance::stream {

class Handler : public common::stream::reader::handler::IHandler {
  public:
    Handler();

    void HandleMessage(const std::string& msg) final;

  private:
    common::market::MarketManager& market_manager_;
    common::quote::QuoteManager& quote_manager_;
    std::string bids_price_;
    std::string bids_volume_;
    std::string asks_price_;
    std::string asks_volume_;
};

class BinanceReader : public common::stream::reader::IStreamReader {
  public:
    BinanceReader(size_t start, size_t end);
  protected:
    void InitMarketNames() override;
    virtual void InitURL() override;
    virtual void InitMessage() override;
    virtual void InitHandler() override;
};
}  // namespace binance::stream