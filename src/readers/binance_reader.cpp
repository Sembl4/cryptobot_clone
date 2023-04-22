#include <binance_reader.hpp>
#include <logger.hpp>
using namespace common;
using namespace binance;
using namespace market;
using namespace quote;
using namespace binance::stream;
using namespace common::stream::reader;

constexpr const char* b_p = "b";
constexpr const char* b_q = "B";
constexpr const char* a_p = "a";
constexpr const char* a_q = "A";

constexpr const size_t MAX_SIZE = 20;

///////////// Handler /////////////

Handler::Handler()
    : market_manager_(MarketManager::Get()),
      quote_manager_(QuoteManager::Get()) {
    bids_price_.assign(MAX_SIZE, 0);
    bids_volume_.assign(MAX_SIZE, 0);
    asks_price_.assign(MAX_SIZE, 0);
    asks_volume_.assign(MAX_SIZE, 0);
}

void Handler::HandleMessage(const std::string& msg) {
    try
    {
        nlohmann::json ans = nlohmann::json::parse(msg);
        auto updater = quote_manager_.GetUpdater(market_manager_.MarketId(ans["s"]));
        uint64_t last_update_id = ans["u"];

        if (!updater.CheckResetActuality(last_update_id)) {
            return;
        }
        updater.UpdateBothTimestamps();
        bids_price_ = ans[b_p];
        bids_volume_ = ans[b_q];
        updater.UpdateBid(bids_price_, bids_volume_);

        asks_price_ = ans[a_p];
        asks_volume_ = ans[a_q];
        updater.UpdateAsk(asks_price_, asks_volume_);
    }catch(...)
    {
        Log("EXEPTION :: binance_reader.cpp (HandleMessage):",msg);
        LogFlush();
    }
}

///////////// BinanceReader /////////////

BinanceReader::BinanceReader(size_t start, size_t end)
    : IStreamReader(start, end) {}

static std::string GetBinancePairName(std::string_view str) {
    std::string result;
    for (char c : str) {
        if (std::isdigit(c)) {
            result.push_back(static_cast<char>(c));
        } else {
            result.push_back(static_cast<char>(std::tolower(c)));
        }
    }
    return result;
}

void BinanceReader::InitMarketNames() {
    MarketManager& manager = MarketManager::Get();
    for (size_t i = start_; i < end_; ++i) {
        market_names_.emplace_back(GetBinancePairName(manager.MarketName(i)));
    }
}

void BinanceReader::InitURL() {
    std::string url = "wss://stream.binance.com:9443/ws/";
    for (size_t i = start_; i < end_; ++i) {
        size_t j = i - start_;
        url += market_names_[j];  // lowercase
        url += "@bookTicker";
        if (i != end_ - 1) {
            url += "/";
        }
    }
    *url_ = url;
}

void BinanceReader::InitMessage() {
    message_.clear();
}

void BinanceReader::InitHandler() {
    handler_ = std::shared_ptr<handler::IHandler>(new Handler());
}
