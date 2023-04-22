#include <session_readers.hpp>

#include <json.hpp>

using namespace session;
using namespace binance;
using namespace detail;
using namespace market;

static thread_local auto& market_manager = MarketManager::GetManager();
static thread_local auto& quote_manager = QuoteManager::GetManager();

static thread_local std::string symbols = "marketSymbols_____________";
static thread_local std::string bidsPrice = "bidsQuotePriceString____";
static thread_local std::string bidsVolume = "bidsQuoteVolumeString__";
static thread_local std::string asksPrice = "asksQuotePriceString____";
static thread_local std::string asksVolume = "asksQuoteVolumeString__";

///////////// ThreadProperties /////////////

ThreadProperties::ThreadProperties()
    : io_context_(std::make_shared<net::io_context>()),
      ssl_context_(std::make_shared<boost::asio::ssl::context>(
          boost::asio::ssl::context::tlsv12_client)) {}

void ThreadProperties::Start() {
    thread_ = std::make_shared<std::jthread>(
        [io_ctx = io_context_]() { io_ctx->run(); });
}

void ThreadProperties::Join() {
    io_context_->stop();
    thread_->join();
}

net::io_context& ThreadProperties::IOContext() { return *io_context_; }

net::ssl::context& ThreadProperties::SSLContext() { return *ssl_context_; }

void ThreadProperties::ThreadFunc() { io_context_->run(); }

std::string GetBinancePairName(std::string_view str) {
    std::string result;
    for (char c : str) {
        if (!std::isdigit(c)) {
            c = static_cast<char>(std::tolower(c));
        }
        result.push_back(c);
    }
    return result;
}

///////////// StreamHandler /////////////

StreamSession::StreamHandler::StreamHandler(const std::string& market_name)
    : market_id(market_manager.GetMarketIdFromLowercase(market_name)),
      market_info(market_manager.GetMarketInfo(market_id)),
      updater(quote_manager.GetUpdater(market_id)) {}

void StreamSession::StreamHandler::Process(std::string msg) {
    std::cout << '\n' << market_info.symbol << '\n' << msg << std::endl;
}

///////////// StreamSession /////////////

StreamSession::StreamSession(size_t n_threads, size_t start, size_t end)
    : n_threads_(n_threads), start_(start), end_(end) {
    InitThreads();
    InitParams();
    InitWebsockets();
}

void StreamSession::InitThreads() {
    if (n_threads_ == 0) {
        std::cerr << "threads count must be positive\n";
        std::abort();
    }
    for (size_t i = 0; i < n_threads_; ++i) {
        threads_.emplace_back(std::make_shared<ThreadProperties>());
    }
}

void StreamSession::InitParams() {
    size_t count = end_ - start_;
    for (size_t i = start_; i < end_; ++i) {
        market_names_.emplace_back(
            GetBinancePairName(market_manager.GetPairName(i)));
    }
}

void StreamSession::InitWebsockets() {
    for (size_t i = 0; i < market_names_.size(); ++i) {
        size_t j = i % threads_.size();
        websockets_.emplace_back(std::make_shared<Session<StreamHandler>>(
            threads_[j]->IOContext(), threads_[j]->SSLContext(),
            StreamHandler(market_names_[i])));
    }
}

void StreamSession::RunThreads() {
    for (auto& thread : threads_) {
        thread->Start();
    }
}

void StreamSession::JoinThreads() {
    for (auto& thread : threads_) {
        thread->Join();
    }
}

void StreamSession::Start() {
    if (websockets_.size() != market_names_.size() ||
        websockets_.size() != (end_ - start_)) {
        std::cerr << "Wrong size in StreamSession\n" << std::flush;
        std::abort();
    }
    auto const host = "stream.binance.com";
    auto const port = "443";
    auto const path = "/ws";
    // R"({"method": "SUBSCRIBE", "params": ["btcusdt@bookTicker"], "id": 0})";
    const std::string prefix = R"({"method": "SUBSCRIBE", "params": [")";
    const std::string suffix = R"(@depth5@100ms"], "id": 0})";
    for (size_t i = 0; i < websockets_.size(); ++i) {
        std::string text = prefix + market_names_[i] + suffix;
        websockets_[i]->Run(host, port, path, text);
    }
    RunThreads();
}

void StreamSession::Join() {
    for (auto& session : websockets_) {
        session->Close();
    }
    JoinThreads();
}

///////////// TickerBookHandler /////////////

TickerBookSession::TickerBookHandler::TickerBookHandler() {
    market_name.reserve(100);
}

void TickerBookSession::TickerBookHandler::Process(std::string msg) {
//   std::cerr << '\n' << msg << '\n' << std::flush;

    if (msg.size() <= 30) {
        return;
    }

    auto parsed_data = nlohmann::json::parse(msg);
    market_name = parsed_data["s"];
    auto updater = quote_manager.GetUpdater(market_manager.GetPairId(market_name));
    uint64_t x = parsed_data["u"];

    if (!updater.CheckResetActuality(x)) {
        return;
    }

    updater.UpdateBothTimestamps();

    bidsPrice = parsed_data["b"];
    bidsVolume = parsed_data["B"];
    updater.UpdateBid(bidsPrice, bidsVolume);

    asksPrice = parsed_data["a"];
    asksVolume = parsed_data["A"];
    updater.UpdateAsk(asksPrice, asksVolume);

//    std::cerr << '\n' << parsed_data["u"] << '\n';
//    std::cerr << '\n' << parsed_data["s"] << '\n';
//    std::cerr << '\n' << parsed_data["b"] << '\n';
//    std::cerr << '\n' << parsed_data["B"] << '\n';
//    std::cerr << '\n' << parsed_data["a"] << '\n';
//    std::cerr << '\n' << parsed_data["A"] << '\n';


}

///////////// TickerBookSession /////////////

TickerBookSession::TickerBookSession(size_t n_threads, size_t n_websockets,
                                     size_t start, size_t end)
    : n_threads_(n_threads),
      n_websockets_(n_websockets),
      start_(start),
      end_(end) {
    if (n_websockets < n_threads) {
        std::cerr << "Websocket's count can't be less than threads count\n" << std::flush;
        std::abort();
    }
    InitThreads();
    InitParams();
    InitWebsockets();
}

void TickerBookSession::InitThreads() {
    if (n_threads_ == 0) {
        std::cerr << "threads count must be positive\n";
        std::abort();
    }
    for (size_t i = 0; i < n_threads_; ++i) {
        threads_.emplace_back(std::make_shared<ThreadProperties>());
    }
}

void TickerBookSession::InitParams() {
    size_t count = end_ - start_;
    for (size_t i = start_; i < end_; ++i) {
        market_names_.emplace_back(
            GetBinancePairName(market_manager.GetPairName(i)));
    }
}

void TickerBookSession::InitWebsockets() {
    for (size_t i = 0; i < n_websockets_; ++i) {
        size_t j = i % threads_.size();
        websockets_.emplace_back(std::make_shared<Session<TickerBookHandler>>(
            threads_[j]->IOContext(), threads_[j]->SSLContext(),
            TickerBookHandler()));
    }
}

void TickerBookSession::RunThreads() {
    for (auto& thread : threads_) {
        thread->Start();
    }
}

void TickerBookSession::JoinThreads() {
    for (auto& thread : threads_) {
        thread->Join();
    }
}

void TickerBookSession::Start() {
    auto const host = "stream.binance.com";
    auto const port = "443";
    auto const path = "/ws";
    // R"({"method": "SUBSCRIBE", "params": ["btcusdt@bookTicker@ethusdt@bookTicker@umabtc@bookTicker@sysbtc@bookTicker"], "id": 0})";
    auto const prefix = R"({"method": "SUBSCRIBE", "params": [")";
    auto const delimeter = R"(",")";
    auto const suffix = R"(], "id": 0})";
    std::vector<std::string> text_array(n_websockets_, prefix);
    for (size_t i = 0; i < market_names_.size(); ++i) {
        size_t websocket_index = i % n_websockets_;
        text_array[websocket_index] += market_names_[i];
        text_array[websocket_index] += "@bookTicker";
        text_array[websocket_index] += delimeter;
    }

    for (size_t i = 0; i < n_websockets_; ++i) {
        text_array[i].pop_back();
        text_array[i].pop_back();
        text_array[i] += suffix;
        std::cerr << text_array[i] << '\n' << std::flush;
        websockets_[i]->Run(host, port, path, text_array[i]);
    }
    RunThreads();

}

void TickerBookSession::Join() {
    for (auto& session : websockets_) {
        session->Close();
    }
    JoinThreads();
}
