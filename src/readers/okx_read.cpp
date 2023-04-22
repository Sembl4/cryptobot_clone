#include <okx_read.hpp>
#include <logger.hpp>
using namespace common;
using namespace common::stream::reader;
using namespace okx::stream;
using namespace market;
using namespace quote;

///////////// Handler /////////////

constexpr const size_t MAX_SIZE = 20;

Handler::Handler()
    : market_manager_(MarketManager::Get()),
      quote_manager_(QuoteManager::Get()) {
    bids_price_.assign(MAX_SIZE, 0);
    bids_volume_.assign(MAX_SIZE, 0);
    asks_price_.assign(MAX_SIZE, 0);
    asks_volume_.assign(MAX_SIZE, 0);
    ts_.assign(MAX_SIZE, 0);
}

void Handler::HandleMessage(const std::string& msg) {
    //    std::cerr << "HandleMessage\n" << msg << '\n';
    if (msg.length() < 80) {
        return;
    }
    try {
        nlohmann::json ans = nlohmann::json::parse(msg);
        if (!ans.contains("arg")) {
            std::cout << "Got wrong message in OKX_Reader_handler: \n" << msg << std::endl << std::flush;
            return;
        }
            auto updater = quote_manager_.GetUpdater(
                    market_manager_.MarketId(ans["arg"]["instId"]));
            ts_ = ans["data"][0]["ts"];
            uint64_t last_update_id = strtoll(ts_.c_str(), nullptr, 10);

            if (!updater.CheckResetActuality(last_update_id)) {
                return;
            }
            updater.UpdateBothTimestamps();

            if (!ans["data"][0]["bids"].empty()) {
                bids_price_ = ans["data"][0]["bids"][0][0];
                bids_volume_ = ans["data"][0]["bids"][0][1];
                updater.UpdateBid(bids_price_, bids_volume_);
            }

            if (!ans["data"][0]["asks"].empty()) {
                asks_price_ = ans["data"][0]["asks"][0][0];
                asks_volume_ = ans["data"][0]["asks"][0][1];
                updater.UpdateAsk(asks_price_, asks_volume_);
            }
    } catch (...) {
        Log("EXEPTION :: oks_reader.cpp (HandleMessage):",msg);
        LogFlush();
    }
}

//////////////// Reader /////////////

OKXReader::OKXReader(size_t start, size_t end) : IStreamReader(start, end) {}

void OKXReader::InitMarketNames() {
    MarketManager& manager = MarketManager::Get();
    for (size_t i = start_; i < end_; ++i) {
        market_names_.emplace_back(manager.MarketName(i));
    }
}

void OKXReader::InitURL() { *url_ = "wss://ws.okx.com:8443/ws/v5/public"; }

void OKXReader::InitMessage() {
    message_ = R"({"op":"subscribe", "args": [)";
    std::string common_str = R"({"channel": "bbo-tbt", "instId": ")";
    for (auto& name : market_names_) {
        message_ += common_str;
        message_ += name;
        message_ += R"("},)";
    }
    message_.pop_back();
    message_ += "]}";
}

void OKXReader::InitHandler() {
    handler_ = std::shared_ptr<handler::IHandler>(new Handler());
}