#include <trade_reader.hpp>

#include <logger.hpp>
#include <quote.hpp>

using namespace binance;
using namespace readers;
using namespace trade;
using namespace market;
using namespace hv;
using namespace std::chrono;
using namespace nlohmann;

using std::string;

static auto& market_manager = MarketManager::GetManager();
static auto& quote_manager = QuoteManager::GetManager();

static thread_local string bidsPrice = "bidsTradePriceString__";
static thread_local string bidsVolume = "bidsTradeVolumeString__";
static thread_local string asksPrice = "asksTradePriceString__";
static thread_local string asksVolume = "asksTradeVolumeString__";

static void WSOpenFunction(std::string_view str) {
//    std::cerr << "Thread id:  " << std::this_thread::get_id() << std::endl;
    Log("Opened websocket for %s\n", str.data());
}

static void WSCloseFunction(std::string_view str) {
//    std::cerr << "Thread id:  " << std::this_thread::get_id() << std::endl;
    Log("Close websocket for %s\n", str.data());
}

static void WSProcessData(size_t index, const std::string& msg) {
    auto updater = quote_manager.GetUpdater(index);
    json ans = json::parse(msg);
    if (ans["m"]) {
        bidsPrice = ans["p"];
        bidsVolume = ans["q"];
        updater.UpdateBidTrade(bidsPrice, bidsVolume);
    } else {
        asksPrice = ans["p"];
        asksVolume = ans["q"];
        updater.UpdateAskTrade(asksPrice, asksVolume);
    }
}

TradeReader::TradeReader(size_t start, size_t end, size_t threads_n)
    : start_(start), end_(end) {
    Log("Init TradeReader\n");
    InitThreads(threads_n);
    InitParams();
    InitWebSockets();
    Log("TradeReader initialization finished\n");
}

void TradeReader::InitThreads(size_t n) {
    if (n == 0) {
        Log("Threads count in TradeReader can't be zero!\n");
        std::abort();
    }
    for (size_t i = 0; i < n; ++i) {
        thread_list_.emplace_back(std::make_shared<EventLoopThread>());
    }
}

void TradeReader::InitParams() {
    size_t count = end_ - start_;
    websockets_.reserve(count);
    markets_name_.reserve(count);
    reconn_setting_init(&settings_);
    settings_.min_delay = 1000;
    settings_.max_delay = 10000;
    settings_.delay_policy = 2;
}

static std::string CorrectName(std::string_view str);

void TradeReader::InitWebSockets() {
    size_t threads_count = thread_list_.size();
    for (size_t i = start_; i < end_; ++i) {
        markets_name_.emplace_back(CorrectName(market_manager.GetPairName(i)));
        size_t thread_index = i % threads_count;
        auto ws = std::make_shared<WebSocketClient>(thread_list_[thread_index]->loop());
        ws->onopen = [i]() { WSOpenFunction(market_manager.GetPairName(i)); };
        ws->onclose = [i]() { WSCloseFunction(market_manager.GetPairName(i)); };
        ws->onmessage = [i](const std::string& msg) {
//            auto start_t = duration_cast<nanoseconds>(
//                               system_clock::now().time_since_epoch())
//                               .count();
            WSProcessData(i, msg);
//            auto finish_t = duration_cast<nanoseconds>(
//                                system_clock::now().time_since_epoch())
//                                .count();

//            std::cerr << finish_t - start_t << std::endl;
        };
        ws->setReconnect(&settings_);
        websockets_.emplace_back(ws);
    }
}

void TradeReader::Start() {
    Log("Start reading --trades-- stream and updating quotes\n");

    StartThreads();
    size_t count = end_ - start_;
    for (size_t i = 0; i < count; ++i) {
        std::string url = "wss://stream.binance.com:9443/ws/";
        url += markets_name_[i];  // lowercase
        url += "@trade";

        websockets_[i]->open(url.c_str());

        LogMore("Open websocket with url:", url, '\n');
    }

    Log("All", count, "--trades-- reading websockets are open\n");
}

void TradeReader::StartThreads() {
    for (auto& thread : thread_list_) {
        thread->start();
    }
}

void TradeReader::Join() {
    Log("Joining TradeReader\n");

    for (auto& ws : websockets_) {
        ws->close();
    }
    JoinThreads();

    Log("TradeReader Joined\n");
}

void TradeReader::JoinThreads() {
    for (const auto& thread : thread_list_) {
        thread->stop();
        thread->join();
    }
}

static std::string CorrectName(std::string_view str) {
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