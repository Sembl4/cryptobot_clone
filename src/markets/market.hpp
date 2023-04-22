#pragma once

#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace common::market {

struct Currency {
    std::string name;
    size_t index;
};

struct CurrencyPair {
    std::string first;
    std::string second;
    size_t index;
};

struct MarketInfo {
    std::string symbol;
    double step_size;     // Шаг объема на рынке
    double min_notional;  // Минимальный размер ордера (измеряется в второй
                          // валюте рынка, в которой указан price)
    double tick_size;  // Шаг цены на рынке
};

class MarketManager {
  public:
    static MarketManager& Get();

    void InitGlobal();

    const std::string& MarketName(size_t id) const;
    size_t MarketId(const std::string& key) const;
    size_t CurrencyId(const std::string& name) const;
    std::string_view GetName(size_t index) const;
    MarketInfo GetMarketInfo(size_t index) const;
    const std::vector<std::string>& NoFeeMarkets() const;
    size_t MarketsCount() const;

    void PrintAllMarkets() const;

  private:
    MarketManager();

    void ClearGlobal();
    void InitCurrenciesList(std::unordered_set<std::string>& set);
    void InitMarkets(const std::string& file_name);
    void InitNoFeeMarkets();

  private:
    std::vector<std::string> currencies_list_;
    std::unordered_map<std::string, size_t> currencies_map_;

    std::vector<std::string> market_names_list_;
    std::vector<MarketInfo> markets_info_;
    std::unordered_map<std::string, size_t> market_names_map_;

    std::vector<std::string> no_fee_markets_;
};

}  // namespace common::market