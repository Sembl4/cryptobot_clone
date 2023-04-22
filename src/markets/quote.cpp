#include <quote.hpp>

#include <charconv>
#include <chrono>
#include <iostream>

using namespace common;
using namespace quote;
using std::chrono::milliseconds;
using std::chrono::nanoseconds;

///////////// Rate //////////////
///////////// Quote /////////////

Rate Quote::GetBid() const {
    return Rate{bid_quote_.price.load(), bid_quote_.volume.load()};
}

Rate Quote::GetAsk() const {
    return Rate{ask_quote_.price.load(), ask_quote_.volume.load()};
}

void Quote::SharedRate::Update(double p, double q) {
    price.store(p);
    volume.store(q);
    watched.store(Unchecked);
}

void Quote::SharedRate::UpdateTimestamp() {
    update_t = std::chrono::duration_cast<nanoseconds>(
                   std::chrono::system_clock::now().time_since_epoch())
                   .count();
}

void Quote::UpdateAsk(double price, double volume) {
    ask_quote_.Update(price, volume);
}

void Quote::UpdateBid(double price, double volume) {
    bid_quote_.Update(price, volume);
}

void Quote::UpdateAskTrade(double price, double volume) {
    if (ask_quote_.price.load() == price) {
        ask_quote_.volume.fetch_sub(volume);
        ask_quote_.watched.store(Unchecked);
        ask_quote_.UpdateTimestamp();
    }
}

void Quote::UpdateBidTrade(double price, double volume) {
    if (bid_quote_.price.load() == price) {
        bid_quote_.volume.fetch_sub(volume);
        bid_quote_.watched.store(Unchecked);
        bid_quote_.UpdateTimestamp();
    }
}

bool Quote::CompareAndDecreaseAsk(double price, double volume) {
    if (ask_quote_.price.load() == price) {
        ask_quote_.volume.fetch_sub(volume);
        ask_quote_.watched.store(quote::Unchecked);
        ask_quote_.UpdateTimestamp();
        return true;
    }
    return false;
}

bool Quote::CompareAndDecreaseBid(double price, double volume) {
    if (bid_quote_.price.load() == price) {
        bid_quote_.volume.fetch_sub(volume);
        bid_quote_.watched.store(quote::Unchecked);
        bid_quote_.UpdateTimestamp();
        return true;
    }
    return false;
}

bool Quote::IsAskChecked() const {
    return ask_quote_.watched.load() == Checked;
}

bool Quote::IsBidChecked() const {
    return bid_quote_.watched.load() == Checked;
}

void Quote::SetAskChecked() { ask_quote_.watched.store(Checked); }

void Quote::SetBidChecked() { bid_quote_.watched.store(Checked); }

uint64_t Quote::GetActualityId() const { return last_update_id_; }

void Quote::SetActualityId(uint64_t id) { last_update_id_ = id; }

uint64_t Quote::GetBidTimestamp() const { return bid_quote_.update_t; }

uint64_t Quote::GetAskTimestamp() const { return ask_quote_.update_t; }

void Quote::UpdateBidTimestamp() { bid_quote_.UpdateTimestamp(); }

void Quote::UpdateAskTimestamp() { ask_quote_.UpdateTimestamp(); }

bool Quote::MatchAskPrice(double price) const {
    return ask_quote_.price.load() == price;
}

bool Quote::MatchBidPrice(double price) const {
    return bid_quote_.price.load() == price;
}

void Quote::UpdateTimestamps() {
    UpdateBidTimestamp();
    UpdateAskTimestamp();
}

///////////// QuoteManager /////////////

QuoteManager& QuoteManager::Get() {
    static QuoteManager manager;
    return manager;
}

QuoteManager::QuoteManager() {}

void QuoteManager::Init() {
    quotes_list_.clear();
    quotes_list_ = std::move(
        std::vector<Quote>(market::MarketManager::Get().MarketsCount()));
}

QuoteUpdater QuoteManager::GetUpdater(size_t pair) {
    return QuoteUpdater(quotes_list_[pair]);
}

QuoteReader QuoteManager::GetReader(size_t pair) {
    return QuoteReader(quotes_list_[pair]);
}

Rate QuoteManager::GetRate(size_t market_id, size_t operation) {
    switch (operation) {
        case Bid:
            return quotes_list_[market_id].GetBid();
        case Ask:
            return quotes_list_[market_id].GetAsk();
        default:
            std::abort();
    }
}

//////////// QuoteBase /////////////

QuoteBase::QuoteBase(Quote& quote) : quote_(quote) {}

///////////// QuoteUpdater /////////////

QuoteUpdater::QuoteUpdater(Quote& quote) : QuoteBase(quote) {}

void QuoteUpdater::Update(OrderType type, std::string_view price_str,
                          std::string_view volume_str) {
    double price = 0.0;
    std::from_chars(price_str.data(), price_str.data() + price_str.size(),
                    price);
    double volume = 0.0;
    std::from_chars(volume_str.data(), volume_str.data() + volume_str.size(),
                    volume);
    switch (type) {
        case Bid:
            quote_.UpdateBid(price, volume);
            break;
        case Ask:
            quote_.UpdateAsk(price, volume);
            break;
    }
}

void QuoteUpdater::UpdateBid(std::string_view price, std::string_view volume) {
    Update(Bid, price, volume);
}

void QuoteUpdater::UpdateAsk(std::string_view price, std::string_view volume) {
    Update(Ask, price, volume);
}

void QuoteUpdater::UpdateBidTrade(std::string_view price,
                                  std::string_view volume) {
    double price_d = 0.0;
    std::from_chars(price.data(), price.data() + price.size(), price_d);
    double volume_d = 0.0;
    std::from_chars(volume.data(), volume.data() + volume.size(), volume_d);
    quote_.UpdateBidTrade(price_d, volume_d);
}

void QuoteUpdater::UpdateAskTrade(std::string_view price,
                                  std::string_view volume) {
    double price_d = 0.0;
    std::from_chars(price.data(), price.data() + price.size(), price_d);
    double volume_d = 0.0;
    std::from_chars(volume.data(), volume.data() + volume.size(), volume_d);
    quote_.UpdateAskTrade(price_d, volume_d);
}

bool QuoteUpdater::CompareAndDecrease(int32_t op_type, double price,
                                      double volume) {
    switch (OrderType(op_type)) {
        case Bid:
            return quote_.CompareAndDecreaseBid(price, volume);
        case Ask:
            return quote_.CompareAndDecreaseAsk(price, volume);
        default:
            std::cerr << "Wrong operation type in CompareAndDecrease\n";
            std::abort();
    }
}

bool QuoteUpdater::CheckResetActuality(uint64_t id) {
    if (quote_.GetActualityId() >= id) {
        return false;
    }
    quote_.SetActualityId(id);
    return true;
}

void QuoteUpdater::UpdateBidTimestamp() { quote_.UpdateBidTimestamp(); }

void QuoteUpdater::UpdateAskTimestamp() { quote_.UpdateAskTimestamp(); }

void QuoteUpdater::UpdateBothTimestamps() {
    UpdateAskTimestamp();
    UpdateBidTimestamp();
}

///////////// QuoteReader /////////////

QuoteReader::QuoteReader(Quote& quote) : QuoteBase(quote) {}

Rate QuoteReader::GetBid() const { return GetRate(Bid); }

Rate QuoteReader::GetAsk() const { return GetRate(Ask); }

Rate QuoteReader::Get(size_t type) const { return GetRate(OrderType(type)); }

bool QuoteReader::IsChecked(size_t type) const {
    return IsQuoteChecked(OrderType(type));
}

void QuoteReader::SetChecked(size_t type) { SetQuoteChecked(OrderType(type)); }

Rate QuoteReader::GetRate(OrderType type) const {
    switch (type) {
        case Bid:
            return quote_.GetBid();
        case Ask:
            return quote_.GetAsk();
    }
    std::abort();
}

bool QuoteReader::IsQuoteChecked(OrderType type) const {
    switch (type) {
        case Bid:
            return quote_.IsBidChecked();
        case Ask:
            return quote_.IsAskChecked();
    }
    std::abort();
}

void QuoteReader::SetQuoteChecked(OrderType type) {
    switch (type) {
        case Bid:
            return quote_.SetBidChecked();
        case Ask:
            return quote_.SetAskChecked();
    }
    std::abort();
}

uint64_t QuoteReader::GetTimestamp(size_t type) const {
    switch (type) {
        case Bid:
            return quote_.GetBidTimestamp();
        case Ask:
            return quote_.GetAskTimestamp();
    }
    std::abort();
}

uint64_t QuoteReader::GetBidUpdateTimestamp() const {
    return quote_.GetBidTimestamp();
}

uint64_t QuoteReader::GetAskUpdateTimestamp() const {
    return quote_.GetAskTimestamp();
}

bool QuoteReader::MatchPrice(size_t type, double price) const {
    switch (type) {
        case Bid:
            return quote_.MatchBidPrice(price);
        case Ask:
            return quote_.MatchAskPrice(price);
        default:
            std::cerr << "Wrong type in QuoteReader::MatchPrice, type: " << type
                      << std::endl;
            std::abort();
    }
}