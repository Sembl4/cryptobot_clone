#include <config.hpp>
#include <json.hpp>
#include <logger.hpp>
#include <stream_reader.hpp>

#include <cctype>

using namespace binance;
using namespace config;
using namespace market;
using namespace quote;
using namespace stream;
using namespace hv;
using namespace std::chrono;

using std::string;
using std::string_view;
using std::vector;

static auto& market_manager = MarketManager::Get();
static auto& quote_manager = QuoteManager::Get();
// constexpr const char* BIDS = "bids";
// constexpr const char* ASKS = "asks";
constexpr const char* BIDS = "b";
constexpr const char* ASKS = "a";

constexpr const char* b_p = "b";
constexpr const char* b_q = "B";
constexpr const char* a_p = "a";
constexpr const char* a_q = "A";

static thread_local string symbols = "marketSymbols__";
static thread_local string bidsPrice = "bidsQuotePriceString";
static thread_local string bidsVolume = "bidsQuoteVolumeString";
static thread_local string asksPrice = "asksQuotePriceString";
static thread_local string asksVolume = "asksQuoteVolumeString";

static thread_local size_t bidsIndex = 0;  // in [0, 5)
static thread_local size_t asksIndex = 0;  // in [0, 5)

static void WSOpenFunction(const std::string& url) {
    Log("onopen: Opened websocket\n", url);
}

static void WSCloseFunction(const std::string& url) {
    Log("onclose: Close websocket\n", url);
}

static void WSMessageFunction(std::string_view str) {
    Log("\nMarket info for %s\n", str.data());
}

static void WSProcessMarketDataJson(const std::string& msg) {
    nlohmann::json ans = nlohmann::json::parse(msg);
    std::string symbol = ans["s"];
    auto updater = quote_manager.GetUpdater(market_manager.MarketId(symbol));
    uint64_t last_update_id = ans["u"];

    if (!updater.CheckResetActuality(last_update_id)) {
        return;
    }
    updater.UpdateBothTimestamps();
    // Log(msg);
    bidsPrice = ans[b_p];
    bidsVolume = ans[b_q];
    updater.UpdateBid(bidsPrice, bidsVolume);

    // LogMore( "bids update", bidsPrice, bidsVolume,'\n');

    asksPrice = ans[a_p];
    asksVolume = ans[a_q];
    updater.UpdateAsk(asksPrice, asksVolume);
}

std::string GetBinancePairName(std::string_view str) {
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

StreamReader::StreamReader(size_t start, size_t end)
    : start_(start), end_(end) {
    Log("Init StreamBookTickerReader\n");
    InitThread();
    InitParams();
    InitWebSocket();
    Log("TradeReader initialization finished\n");
}
void StreamReader::InitThread() {
    thread_ = std::make_shared<EventLoopThread>();
}
void StreamReader::InitParams() {
    size_t count = end_ - start_;
    market_names_.clear();
    market_names_.reserve(count);
    reconn_setting_init(&settings_);
    settings_.min_delay = 1000;
    settings_.max_delay = 10000;
    settings_.delay_policy = 1;

    for (size_t i = start_; i < end_; ++i) {
        market_names_.emplace_back(
            GetBinancePairName(market_manager.MarketName(i)));
    }
}

void StreamReader::InitWebSocket() {
    url_ = std::make_shared<string>();
    websocket_ = std::make_shared<WebSocketClient>(thread_->loop());
    websocket_->onopen = [url = url_]() { /*WSOpenFunction(*url);*/ };
    websocket_->onclose = [url = url_]() { /*WSCloseFunction(*url);*/ };
    websocket_->onmessage = [](const std::string& msg) {
        WSProcessMarketDataJson(msg);
    };
    websocket_->setReconnect(&settings_);
}

void StreamReader::Start() {
    Log("Start reading --bookTicker-- stream and updating quotes");
    ////
    StartThread();
    std::string url = "wss://stream.binance.com:9443/ws/";
    for (size_t i = start_; i < end_; ++i) {
        size_t j = i - start_;
        url += market_names_[j];  // lowercase
        url += "@bookTicker";
        if (i != end_ - 1) {
            url += "/";
        }
    }
    LogMore("Open websocket with url:", url, '\n');
    *url_ = url;
    ////
    websocket_->open(url.c_str());
    Log("from", start_, "to", end_, "--quotes-- reading streams are open");
}

void StreamReader::StartThread() { thread_->start(); }

void StreamReader::Join() {
    Log("Joining IStreamReader");
    websocket_->close();
    std::this_thread::sleep_for(milliseconds(3000));
    JoinThread();
    Log("IStreamReader Joined");
}

void StreamReader::JoinThread() {
    thread_->stop();
    thread_->join();
}