#pragma once

#include <init.hpp>
#include <market.hpp>
#include <quote.hpp>
#include <stream_reader.hpp>
#include <triangle.hpp>

namespace cryptobot {
class CryptoBot {
  public:
    CryptoBot();

    void Run();

    void SetCyclesCount(int64_t count);
    void SetCycleWorkTime(int64_t ms);
    void SetWarmUpTime(int64_t ms);
    void SetCoolDownTime(int64_t ms);
    void SetStreamReadersCount(int64_t n);
    void SetTrianglesHandlersCount(int64_t n);

  private:
    void WorkCycle();

    void Init();
    void InitReaders();
    void InitHandlers();

    void StartReaders();
    void StartHandlers();

    void JoinReaders();
    void JoinHandlers();

    void WarmUp() const;
    void Work() const;
    void CoolDown() const;

    void Clear();

  private:
    init::InitMaster& init_master_;
    common::market::MarketManager& market_manager_;
    common::quote::QuoteManager& quote_manager_;
    binance::triangle::TriangleFactory& triangle_factory_;

    int64_t cycles_count_{0};
    int64_t cycle_work_t_{0};  // in milliseconds
    int64_t warm_up_t_{0};
    int64_t readers_count_{0};
    int64_t handlers_count_{0};
    int64_t cool_down_t_{10000};

    std::vector<std::shared_ptr<common::stream::reader::IStreamReader>>
        readers_;
    std::vector<binance::triangle::TrianglePack> handlers_;
};
}  // namespace cryptobot
