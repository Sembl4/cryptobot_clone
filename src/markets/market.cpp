#include <market.hpp>

#include <binanceClient.hpp>
#include <config.hpp>
#include <logger.hpp>

using namespace common;
using namespace config;
using namespace market;
using std::string;
using std::string_view;
using std::unordered_map;
using std::vector;

MarketManager::MarketManager() {}

MarketManager& MarketManager::Get() {
    static MarketManager manager;
    return manager;
}

void MarketManager::InitGlobal() {
    ClearGlobal();
    auto& client = cryptoClient::getClient();
    auto exchange_info = client.exchangeInfo().marketsInfo;
    std::unordered_set<string> currencies;
    std::unordered_set<string> markets;
    for (const auto& key_val : exchange_info) {
        string market_name = key_val.first;
        if (market_name !=
            key_val.second.baseAsset + key_val.second.quoteAsset) {
            std::cerr << "Wrong data in ExchangeInfo\n";
            return;
        }
        currencies.insert(key_val.second.baseAsset);
        currencies.insert(key_val.second.quoteAsset);
    }

    InitCurrenciesList(currencies);
    InitMarkets("pairs_" + Config::Market().working_asset + "_" +
                Config::Market().trading_platform + ".txt");
    InitNoFeeMarkets();

    Log("Markets successfully built, count: ", MarketsCount());
}

void MarketManager::InitMarkets(const std::string& file_name) {
    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Couldn't open file " << file_name
                  << " in MarketManager::InitMarkets\n";
        return;
    }
    while (!file.eof()) {
        MarketInfo info;
        file >> info.symbol;
        if (info.symbol.length() <= 3) {
            break;
        }
        file >> info.step_size >> info.min_notional >> info.tick_size;
        markets_info_.emplace_back(info);
        market_names_list_.emplace_back(info.symbol);
    }
    file.close();
    for (size_t i = 0; i < market_names_list_.size(); ++i) {
        market_names_map_.insert({market_names_list_[i], i});
    }
}

void MarketManager::InitCurrenciesList(std::unordered_set<std::string>& set) {
    for (const auto& symbol : set) {
        size_t index = currencies_list_.size();
        currencies_list_.emplace_back(symbol);
        currencies_map_.insert({symbol, index});
    }
}

void MarketManager::InitNoFeeMarkets() {
    const string path = Config::Market().no_fee_markets_file_path;
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Couldn't open file in MarketManager::InitNoFeeMarkets\n";
        return;
    }
    while (!file.eof()) {
        std::string market_name;
        file >> market_name;
        if (market_name.empty()) {
            break;
        }
        if (market_name == "and") {
            continue;
        }
        auto iter = market_name.begin();
        for (; iter != market_name.end(); ++iter) {
            if (*iter == '/') {
                break;
            }
        }
        market_name.erase(iter);
        if (market_name.back() == ',') {
            market_name.pop_back();
        }
        no_fee_markets_.emplace_back(market_name);
    }
    file.close();
}

void MarketManager::ClearGlobal() {
    currencies_list_.clear();
    currencies_map_.clear();
    markets_info_.clear();
    market_names_list_.clear();
    market_names_map_.clear();
    no_fee_markets_.clear();
}

const string& MarketManager::MarketName(size_t id) const {
    if (id < market_names_list_.size()) {
        return market_names_list_[id];
    }
    std::cerr << "Index out of range in GetMarketName\n";
    assert(false);
}

size_t MarketManager::MarketId(const string& key) const {
    auto find = market_names_map_.find(key);

    if (find != market_names_map_.end()) {
        return find->second;
    }

    std::cerr << "Unreachable code in MarketId\n";
    assert(false);
}

size_t MarketManager::CurrencyId(const string& name) const {
    auto find = currencies_map_.find(name);
    if (find != currencies_map_.end()) {
        return find->second;
    }

    std::cerr << "Unreachable code in CurrencyId\n";
    assert(false);
}

string_view MarketManager::GetName(size_t index) const {
    if (index < currencies_list_.size()) {
        return currencies_list_[index];
    }

    std::cerr << "Index out of memory bound in GetName\n";
    assert(false);
}

MarketInfo MarketManager::GetMarketInfo(size_t index) const {
    if (index < markets_info_.size()) {
        return markets_info_[index];
    }

    std::cerr << "Index out of memory bound in GeTMarketInfo\n";
    assert(false);
}

const std::vector<std::string>& MarketManager::NoFeeMarkets() const {
    return no_fee_markets_;
}

void MarketManager::PrintAllMarkets() const {
    Log("Markets", markets_info_.size());
    for (const auto& market : markets_info_) {
        LogMore(market.symbol, market.step_size, market.min_notional, '\n');
    }
}

size_t MarketManager::MarketsCount() const { return markets_info_.size(); }
