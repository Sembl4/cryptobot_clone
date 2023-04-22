#pragma once
#include <chrono>
#include <iostream>
#include <string>
#include <hv/json.hpp>
#include <optional>
// #include <request_data.hpp>

enum CRYPTO_PLATFORMS
{
    BINANCE,
    OKX
};


namespace utils {
template<typename Type>
inline uint64_t getTimestamp() {
    return std::chrono::duration_cast<Type>(std::chrono::system_clock::now().time_since_epoch()).count();
}

inline const char* precision(size_t prec)
{
    return std::string("%." + std::to_string(prec) + "f").c_str();
}

inline std::string to_str(double num, size_t prec) {
    char buf[100];
    std::sprintf(buf, precision(prec), num);
    return buf;
}
}  // namespace utils

namespace CryptoAPI
{

enum ORDER_STATUS // Возможные статусы ордеров, на разных биржах может не быть каких-то ответов
{
    NEW,                // Размещен
    PARTIALLY_FILLED,   // Частично заполнен
    FILLED,             // Полностью заполнен
    CANCELED,           // Отменен пользователем
    REJECTED,           // Ордер не подтвержден системой (проблемы с входными значениями)
    EXPIRED,            // Отменен в соответствии с правилами ордера
    EXPIRED_IN_MATCH    // Отменен биржей из-за STP-триггера
};

enum timeInForce
{
    GTC,
    IOC,
    FOK
};

struct MarketInfo {
    std::string baseAsset;  // Название первой валюты
    std::string quoteAsset; // Название второй валюты
    size_t basePrecision;   // Точность первой валюты
    size_t quotePrecision;  // Точность второй валюты
    double minNotional;     // Минимальный размер ордера во второй валюте
    double stepSize;        // Шаг объема в первой валюте
    double tickSize;        // Шаг цены во второй валюте
};

struct ExchangeInfo {
    uint64_t updateTime;  // Время последнего обновления
    std::map<std::string, MarketInfo> marketsInfo;  // Информация о рынках : std::string - название рынка,
                                                    // MarketInfo - информация
};

struct Balances {
    uint64_t updateTime;
    std::map<std::string, double> assets;
};

struct OrderAnswer
{
    bool was_exception = false;
    bool is_done = false; // Соответствует ли ответ нужным статусам
    std::string text;
    uint64_t ts; // Время выполнения ордера
    ORDER_STATUS status; // Статус ордера
    double base_filled_qty; // Сколько мы продали/купили чистыми в основной валюте
    double quote_filled_qty;// Сколько мы продали/купили чистыми во второй валюте

    bool operator==(OrderAnswer a)
    {
        return  is_done == a.is_done &&
                ts == a.ts &&
                status == a.status &&
                base_filled_qty == a.base_filled_qty &&
                quote_filled_qty == a.quote_filled_qty;
    }
};

}

class cryptoClient
{
protected:
    int platform = -1;
    std::optional<std::string> working_asset = std::nullopt; // Название рабочей валюты
    std::optional<std::string> commission_asset = std::nullopt; // Название комиссионной валюты
    std::optional<CryptoAPI::MarketInfo> working_market = std::nullopt; // Рынок рабочей и комиссионной валюты
public:

    static cryptoClient& getClient(int platform);
    static cryptoClient& getClient();
    int getPlatform();
    std::string getWorkingAsset();
    std::string getCommissionAsset();
    
    CryptoAPI::MarketInfo getWorkingMarket();
    
    virtual void Disconnect() = 0;

    virtual void Connect() = 0;

    virtual void Reconnect() = 0;

    virtual CryptoAPI::ExchangeInfo exchangeInfo() = 0;

    virtual CryptoAPI::Balances balances() = 0;

    virtual nlohmann::json createLimitOrder(const std::string_view& trading_pair,int side, double quantity,double price, CryptoAPI::timeInForce tm = CryptoAPI::FOK) = 0;

    virtual nlohmann::json createMarketOrder(const std::string_view& trading_pair,int side, double quantity) = 0;

    virtual nlohmann::json createMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity) = 0;

    virtual uint64_t sendRequest(nlohmann::json order) = 0;

    virtual uint64_t openLimitOrder(const std::string_view& trading_pair,int side, double quantity,double price,CryptoAPI::timeInForce tm = CryptoAPI::FOK) = 0;

    virtual uint64_t openMarketOrder(const std::string_view& trading_pair,int side, double quantity) = 0;

    virtual uint64_t openMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity) = 0;

    // virtual std::string getAnswer(uint64_t ans_num) = 0;
    virtual void cancelOrder(const std::string_view& trading_pair,const std::string_view& order_id) = 0;

    virtual CryptoAPI::OrderAnswer getAnswer(uint64_t ans_num, uint64_t wait_ms = 5'000) = 0; 
    virtual CryptoAPI::OrderAnswer getAnswerSt(uint64_t ans_num, std::vector<CryptoAPI::ORDER_STATUS> await_statuses, uint64_t wait_ms = 10'000) = 0;

    virtual double getBalanceInCur(const std::string& cur) = 0;

    virtual std::string SIDE_(int side) = 0;
};