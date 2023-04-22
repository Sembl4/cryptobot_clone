#pragma once

#include <market.hpp>

#include <atomic>
#include <vector>

namespace common {

struct Rate {
    double price;
    double volume;
};

///////////// Quote /////////////

namespace quote {

enum QuoteState {
    Checked,
    Unchecked,
};

class Quote {
  private:
    struct SharedRate {
        std::atomic<double> price{0.0};
        std::atomic<double> volume{0.0};
        std::atomic<quote::QuoteState> watched{quote::Checked};
        std::atomic<uint64_t> update_t{0};  // in nanoseconds

        void Update(double p, double q);

        void UpdateTimestamp();
    };

  public:
    Quote() = default;

    ~Quote() = default;

    Quote(const Quote& other) = default;

    Quote(Quote&& other) = delete;

    Quote& operator=(const Quote& other) = delete;

    Quote& operator=(Quote&& other) = delete;

    [[nodiscard]] Rate GetBid() const;

    [[nodiscard]] Rate GetAsk() const;

    void UpdateBid(double price, double volume);

    void UpdateAsk(double price, double volume);

    void UpdateBidTrade(double price, double volume);

    void UpdateAskTrade(double price, double volume);

    bool CompareAndDecreaseBid(double price, double volume);

    bool CompareAndDecreaseAsk(double price, double volume);

    bool IsBidChecked() const;

    bool IsAskChecked() const;

    void SetBidChecked();

    void SetAskChecked();

    uint64_t GetActualityId() const;

    void SetActualityId(uint64_t id);

    uint64_t GetBidTimestamp() const;

    uint64_t GetAskTimestamp() const;

    void UpdateBidTimestamp();

    void UpdateAskTimestamp();

    void UpdateTimestamps();

    bool MatchBidPrice(double price) const;

    bool MatchAskPrice(double price) const;

  private:
    SharedRate bid_quote_;
    SharedRate ask_quote_;
    uint64_t last_update_id_{0};
};

///////////// QuoteManager /////////////

class QuoteUpdater;

class QuoteReader;

class QuoteManager {
  private:
    enum OrderType {
        Bid = 0,
        Ask = 1,
    };

  public:
    ~QuoteManager() = default;

    QuoteManager(const QuoteManager& other) = delete;

    QuoteManager& operator=(const QuoteManager& other) = delete;

    static QuoteManager& Get();

    // Initialize quote list
    void Init();

    // Get updater/reader for currency pair quote
    QuoteUpdater GetUpdater(size_t pair);

    QuoteReader GetReader(size_t pair);

    Rate GetRate(size_t market_id, size_t operation);

  private:
    QuoteManager();

  private:
    std::vector<Quote> quotes_list_;
    std::vector<market::CurrencyPair> currency_pairs_;
};

///////////// QuoteBase /////////////

class QuoteBase {
  protected:
    enum OrderType {
        Bid = 0,
        Ask = 1,
    };

  public:
    QuoteBase() = delete;

    explicit QuoteBase(Quote& quote);

  protected:
    Quote& quote_;
};

///////////// QuoteUpdater /////////////

class QuoteUpdater : private QuoteBase {
  public:
    QuoteUpdater() = delete;

    explicit QuoteUpdater(Quote& quote);

    void UpdateBid(std::string_view price, std::string_view volume);
    void UpdateAsk(std::string_view price, std::string_view volume);
    void UpdateBidTrade(std::string_view price, std::string_view volume);
    void UpdateAskTrade(std::string_view price, std::string_view volume);
    bool CompareAndDecrease(int32_t op_type, double price, double volume);
    bool CheckResetActuality(uint64_t id);
    void UpdateAskTimestamp();
    void UpdateBidTimestamp();
    void UpdateBothTimestamps();

  private:
    void Update(OrderType type, std::string_view price,
                std::string_view volume);
};

///////////// QuoteReader /////////////

class QuoteReader : private QuoteBase {
  public:
    QuoteReader() = delete;

    explicit QuoteReader(Quote& quote);

    [[nodiscard]] Rate GetBid() const;
    [[nodiscard]] Rate GetAsk() const;
    bool IsChecked(size_t type) const;
    void SetChecked(size_t type);
    uint64_t GetBidUpdateTimestamp() const;
    uint64_t GetAskUpdateTimestamp() const;
    uint64_t GetTimestamp(size_t type) const;
    bool MatchPrice(size_t type, double price) const;
    [[nodiscard]] Rate Get(size_t type) const;

  private:
    Rate GetRate(OrderType type) const;
    bool IsQuoteChecked(OrderType type) const;
    void SetQuoteChecked(OrderType type);
};

}  // namespace quote

}  // namespace common