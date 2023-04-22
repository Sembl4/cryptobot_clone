#include <trade_reader.hpp>

#include <logger.hpp>
#include <quote.hpp>

using namespace binance;
using namespace readers;
using namespace trade;
using namespace market;
using namespace quote;
using namespace hv;
using namespace std::chrono;
using namespace nlohmann;

using std::string;

static auto& market_manager = MarketManager::Get();
static auto& quote_manager = QuoteManager::Get();

static std::vector<string> trade_urls;

static thread_local string symbols = "marketSymbols__";
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


static void WSProcessData(const std::string& msg) {
    json ans = json::parse(msg);
    symbols = ans["s"];
    auto updater = quote_manager.GetUpdater(market_manager.MarketId(symbols));
    // Log(symbols,msg);
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

static std::string CorrectName(std::string_view str);

void TradeReader::InitParams() {
    size_t count = end_ - start_;
    websockets_.reserve(count);
    markets_name_lower_.reserve(count);
    reconn_setting_init(&settings_);
    settings_.min_delay = 1000;
    settings_.max_delay = 10000;
    settings_.delay_policy = 2;

    for(size_t i = start_; i < end_; ++i)
    {
        markets_name_lower_.emplace_back(CorrectName(market_manager.MarketName(i)));
    }
}


void TradeReader::InitWebSockets() {
    size_t threads_count = thread_list_.size();
    for (size_t i = 0; i < threads_count; ++i) {
        // size_t thread_index = i % threads_count;
        auto ws = std::make_shared<WebSocketClient>(thread_list_[i]->loop());
        
        ws->onopen = [i]() { WSOpenFunction(trade_urls[i]); };
        ws->onclose = [i]() { WSCloseFunction(trade_urls[i]); };
        ws->onmessage = [](const std::string& msg) {
            WSProcessData(msg);
        };
        ws->setReconnect(&settings_);
        websockets_.emplace_back(ws);
    }
}

void TradeReader::Start() {
    Log("Start reading --trades-- stream and updating quotes\n");

    StartThreads();
    size_t count = end_ - start_;
    size_t threads_count = thread_list_.size();
    size_t piece = count / threads_count;
    for(size_t j = 0; j < threads_count; ++j)
    {
        size_t local_end = piece * (j+1);
        if(j == threads_count - 1)
            local_end = end_;
        
        std::string url = "wss://stream.binance.com:9443/ws/";
        for (size_t i = piece * j + start_; i < local_end; ++i) {
            url += markets_name_lower_[i];  // lowercase
            url += "@trade";
            if(i != local_end - 1)
                url += "/";
        }
        LogMore("Open websocket with url:", url, '\n');
        trade_urls.push_back(url);
        websockets_[j]->open(url.c_str());
    }

    Log("All", count, "--trades-- reading streams are open, with ", threads_count ," threads & websockets\n");
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
    trade_urls.clear();
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