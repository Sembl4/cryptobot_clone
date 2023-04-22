#include <config.hpp>
#include <json.hpp>
#include <logger.hpp>
#include <stream_reader.hpp>

#include <cctype>

using namespace binance;
using namespace config;
using namespace market;
using namespace hv;
using namespace std::chrono;

using std::string;
using std::string_view;
using std::vector;

constexpr const char* BIDS = "bids";
constexpr const char* ASKS = "asks";

static thread_local string bidsPrice = "bidsQuotePriceString";
static thread_local string bidsVolume = "bidsQuoteVolumeString";
static thread_local string asksPrice = "asksQuotePriceString";
static thread_local string asksVolume = "asksQuoteVolumeString";

static size_t bidsIndex = 0;  // in [0, 5)
static size_t asksIndex = 0;  // in [0, 5)

static void WSOpenFunction(std::string_view str) {
    //    Log("Opened websocket for %s\n", str.data());
}

static void WSCloseFunction(std::string_view str) {
    //    Log("Close websocket for %s\n", str.data());
}

static void WSMessageFunction(std::string_view str) {
    Log("\nMarket info for %s\n", str.data());
}

static auto& market_manager = MarketManager::GetManager();
static auto& quote_manager = QuoteManager::GetManager();

static uint64_t ExtractLastUpdateId(const std::string& msg) {
    char id_str[25];
    size_t begin = 16;  // 15 is the index of ':'
    size_t end = 0;
    for (size_t i = begin; i < msg.size(); ++i) {
        if (msg[i] == ',') {
            end = i;
            break;
        }
        id_str[i - begin] = msg[i];
    }
    id_str[end] = '\0';
    return strtoull(id_str, nullptr, 10);
}

static void WSProcessMarketDataJson(size_t pair_index, const std::string& msg) {
    //    std::cerr << msg << std::endl;

    auto updater = quote_manager.GetUpdater(pair_index);
    nlohmann::json ans = nlohmann::json::parse(msg);
    uint64_t last_update_id = ans["lastUpdateId"];
    if (!updater.CheckResetActuality(last_update_id)) {
        return;
    }
    updater.UpdateBothTimestamps();

    bidsPrice = ans[BIDS][bidsIndex][0];
    bidsVolume = ans[BIDS][bidsIndex][1];
    updater.UpdateBid(bidsPrice, bidsVolume);

    asksPrice = ans[ASKS][asksIndex][0];
    asksVolume = ans[ASKS][asksIndex][1];
    updater.UpdateAsk(asksPrice, asksVolume);
}

static void WSProcessMarketData(size_t pair_index, const std::string& msg) {
    // ToDo Remove
    auto updater = quote_manager.GetUpdater(pair_index);
    uint64_t last_update_id = ExtractLastUpdateId(msg);
    if (!updater.CheckResetActuality(last_update_id)) {
        return;
    }
    updater.UpdateBothTimestamps();
    for (size_t i = 0; i < msg.size(); ++i) {
        if (msg[i] == 'b' && msg[i + 1] == 'i' && msg[i + 2] == 'd' &&
            msg[i + 3] == 's') {
            size_t price_start = i + 9;
            size_t count = 0;
            size_t j = price_start;
            for (;; ++j) {
                if (msg[j] == '\"') {
                    break;
                }
                ++count;
            }
            std::string_view bids_price(msg.c_str() + price_start, count);

            //            std::cout << "bids price: "<< bids_price <<" bids\n";

            size_t volume_start = j + 3;
            j = volume_start;
            count = 0;
            for (;; ++j) {
                if (msg[j] == '\"') {
                    break;
                }
                ++count;
            }
            std::string_view bids_volume(msg.c_str() + volume_start, count);

            //            std::cout << "bids volume: " << bids_volume << "
            //            bids\n";

            updater.UpdateBid(bids_price, bids_volume);
            i += 90;
            continue;
        }
        if (msg[i] == 'a' && msg[i + 1] == 's' && msg[i + 2] == 'k' &&
            msg[i + 3] == 's') {
            size_t price_start = i + 9;
            size_t count = 0;
            size_t j = price_start;
            for (;; ++j) {
                if (msg[j] == '\"') {
                    break;
                }
                ++count;
            }
            std::string_view asks_price(msg.c_str() + price_start, count);

            //            std::cout << "asks price: " << asks_price << "
            //            asks\n";

            size_t volume_start = j + 3;
            j = volume_start;
            count = 0;
            for (;; ++j) {
                if (msg[j] == '\"') {
                    break;
                }
                ++count;
            }
            std::string_view asks_volume(msg.c_str() + volume_start, count);

            //            std::cout << "asks volume: " << asks_volume << "
            //            asks\n";

            updater.UpdateAsk(asks_price, asks_volume);
            break;
        }
    }
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
    : start_(start), end_(end), thread_(EventLoopThread()) {
    size_t count = end_ - start_;
    websockets_.reserve(count);
    currency_pair_name_.reserve(count);
    settings_.resize(count);
    bidsIndex = Config::Stream().bids_index;
    asksIndex = Config::Stream().asks_index;
    Init();
}

void StreamReader::Init() {
    Log("Init StreamReader\n");

    for (size_t i = start_; i < end_; ++i) {
        size_t j = i - start_;
        currency_pair_name_.emplace_back(
            GetBinancePairName(market_manager.GetPairName(i)));
        auto ws = std::make_shared<WebSocketClient>(thread_.loop());
        websockets_.emplace_back(ws);
        ws->onopen = [i]() { WSOpenFunction(market_manager.GetPairName(i)); };
        ws->onclose = [i]() { WSCloseFunction(market_manager.GetPairName(i)); };
        ws->onmessage = [i](const std::string& msg) {
            //            auto start_t = duration_cast<nanoseconds>(
            //                               system_clock::now().time_since_epoch())
            //                               .count();
            WSProcessMarketDataJson(i, msg);
            //            auto finish_t = duration_cast<nanoseconds>(
            //                                system_clock::now().time_since_epoch())
            //                                .count();

            //            std::cerr << start_t - finish_t << std::endl;
        };
        reconn_setting_init(&settings_[j]);
        settings_[j].min_delay = 1000;
        settings_[j].max_delay = 10000;
        settings_[j].delay_policy = 2;
        ws->setReconnect(&settings_[j]);
    }

    Log("StreamReader initialization finished\n");
}

void StreamReader::Start() {
    Log("Start reading --quotes-- stream and updating quotes\n");

    thread_.start();
    size_t count = end_ - start_;
    for (size_t i = 0; i < count; ++i) {
        std::string url = "wss://stream.binance.com:9443/ws/";
        url += currency_pair_name_[i];
        url += "@depth5@100ms";

        websockets_[i]->open(url.c_str());

        //        Log("Open websocket with url: ");
        //        Log(url);
        //        Log("\n");
    }

    Log("All", count, "--quotes-- reading websockets are open\n");
}

void StreamReader::Join() {
    Log("Joining StreamReader\n");

    for (auto& ws : websockets_) {
        ws->close();
    }
    thread_.stop();
    thread_.join();

    Log("StreamReader Joined\n");
}