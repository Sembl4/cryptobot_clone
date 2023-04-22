#include <cryptobot.hpp>

#include <binance_reader.hpp>
#include <config.hpp>
#include <logger.hpp>
#include <okx_read.hpp>

#include <cassert>

using namespace common;
using namespace binance;
using namespace common::stream;
using namespace binance::stream;
using namespace okx::stream;
using namespace cryptobot;
using namespace init;
using namespace market;
using namespace quote;
using namespace triangle;

using common::stream::reader::IStreamReader;
using std::chrono::milliseconds;

static void Panic(std::string message) {
    std::cerr << message << std::endl << std::flush;
    assert(false);
}

CryptoBot::CryptoBot()
    : init_master_(InitMaster::Get()),
      market_manager_(MarketManager::Get()),
      quote_manager_(QuoteManager::Get()),
      triangle_factory_(TriangleFactory::Get()) {}

void CryptoBot::Run() {
    Log("CryptoBot Run, cycles count:", cycles_count_);
    Log("Cycle work time:", cycle_work_t_);
    Log("Cycle warm up time:", warm_up_t_);
    Log("StreamReaders count:", readers_count_);
    Log("TriangleHandlers count:", handlers_count_);
    int64_t cycles = cycles_count_;
    while (cycles-- > 0) {
        int64_t number = cycles_count_ - cycles;
        Log("Start cycle No:", number);
        WorkCycle();
        Log("Cycle No:", number, "successfully finished!");
    }
    Log("CryptoBot successfully finished job");
}

void CryptoBot::WorkCycle() {
    /// Init
    Init();

    StartReaders();
    WarmUp();

    StartHandlers();
    LogFlush();
    Work();
    LogFlush();
    JoinHandlers();
    JoinReaders();
    CoolDown();
    Clear();
}

void CryptoBot::Init() {
    init_master_.Setup();
    market_manager_.InitGlobal();
    quote_manager_.Init();
    triangle_factory_.BuildTrianglePack();
    InitReaders();
    InitHandlers();
}

void CryptoBot::InitReaders() {
    auto platform = config::Config::Market().trading_platform;
    readers_.clear();
    size_t markets_count = market_manager_.MarketsCount();
    size_t batch_size = markets_count / readers_count_;
    size_t start = 0;
    size_t end = batch_size;
    for (size_t i = 0; i < readers_count_; ++i) {
        std::shared_ptr<IStreamReader> reader;
        if (platform == "Binance") {
            reader =
                std::shared_ptr<IStreamReader>(new BinanceReader(start, end));
        } else if (platform == "OKX") {
            reader = std::shared_ptr<IStreamReader>(new OKXReader(start, end));
        }
        readers_.emplace_back(std::move(reader));
        start += batch_size;
        end = i == readers_count_ - 2 ? markets_count : end + batch_size;
    }
    Log("Readers Initialized, count:", readers_count_);
}

void CryptoBot::InitHandlers() {
    handlers_.clear();
    size_t triangles_count = triangle_factory_.TrianglesCount();
    size_t batch_size = triangles_count / handlers_count_;
    size_t start = 0;
    size_t end = batch_size;
    for (size_t i = 0; i < handlers_count_; ++i) {
        handlers_.emplace_back(triangle_factory_.GetHandler(start, end));
        start += batch_size;
        end = (i == (handlers_count_ - 2) ? triangles_count : end + batch_size);
    }
    Log("Handlers Initialized, count:", readers_count_);
    LogFlush();
}

void CryptoBot::StartReaders() {
    for (auto& reader : readers_) {
        reader->Start();
    }
}

void CryptoBot::StartHandlers() {
    for (auto& handler : handlers_) {
        handler.Start();
    }
}

void CryptoBot::JoinReaders() {
    for (auto& reader : readers_) {
        reader->Join();
    }
}

void CryptoBot::JoinHandlers() {
    for (auto& handler : handlers_) {
        handler.Stop();
    }
}

void CryptoBot::SetCyclesCount(int64_t count) {
    if (count <= 0) {
        Panic("Cycles count should be positive");
    }
    cycles_count_ = count;
}

void CryptoBot::SetCycleWorkTime(int64_t ms) {
    if (ms <= 0) {
        Panic("Cycle work time should be positive");
    }
    cycle_work_t_ = ms;
}

void CryptoBot::SetWarmUpTime(int64_t ms) {
    if (ms <= 0) {
        Panic("WarmUp time should be positive");
    }
    warm_up_t_ = ms;
}

void CryptoBot::SetStreamReadersCount(int64_t n) {
    if (n <= 0) {
        Panic("StreamReaders count should be positive");
    }
    readers_count_ = n;
}

void CryptoBot::SetTrianglesHandlersCount(int64_t n) {
    if (n <= 0) {
        Panic("Triangles handlers count should be positive");
    }
    handlers_count_ = n;
}

void CryptoBot::SetCoolDownTime(int64_t ms) {
    if (ms <= 0) {
        Panic("CoolDown time should be positive");
    }
    cool_down_t_ = ms;
}

void CryptoBot::Clear() {
    readers_.clear();
    handlers_.clear();
}

void CryptoBot::WarmUp() const {
    std::this_thread::sleep_for(milliseconds(warm_up_t_));
}

void CryptoBot::Work() const {
    std::this_thread::sleep_for(milliseconds(cycle_work_t_));
}

void CryptoBot::CoolDown() const {
    std::this_thread::sleep_for(milliseconds(cool_down_t_));
}
