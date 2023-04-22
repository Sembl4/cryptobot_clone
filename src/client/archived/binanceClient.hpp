#pragma once

#include <hv/axios.h>
#include <hv/requests.h>
#include <chrono>
#include <hv/json.hpp>
#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>

#include <hv/WebSocketClient.h>
namespace binanceAPI {

constexpr const char* client_id = "binance-cryptobot-client-v-0-2";
constexpr const char* BASE_ENDPOINT = "wss://ws-api.binance.com:9443/ws-api/v3";

constexpr const char* OPEN_ORDER = "order.place";
constexpr const char* ACCOUNT = "account.status";

constexpr const char* MARKET = "MARKET";
constexpr const char* LIMIT = "LIMIT";

constexpr const char* BUY = "BUY";
constexpr const char* SELL = "SELL";

constexpr const char* EXCHANGE_INFO_HTTP =
    "https://api.binance.com/api/v3/exchangeInfo?permissions=SPOT";
constexpr const char* ACCOUNT_HTTP = "https://api.binance.com/api/v3/account";
constexpr const char* USER_DATA_HTTP =
    "https://api.binance.com/api/v3/userDataStream";

constexpr const char* EXCHANGE_INFO = "exchangeInfo";

// Надо открыть вебсокет с url = USER_ENDPOINT + listenKey
constexpr const char* USER_ENDPOINT = "wss://stream.binance.com:9443/ws/";
constexpr const char* USER_DATA_STREAM_START = "userDataStream.start";
constexpr const char* USER_DATA_STREAM_STOP = "userDataStream.stop";
constexpr const char* USER_DATA_STREAM_PING = "userDataStream.ping";

struct MarketInfo {
    std::string baseAsset;   // Название первой валюты
    std::string quoteAsset;  // Название второй валюты
    size_t basePrecision;    // Точность первой валюты
    size_t quotePrecision;   // Точность второй валюты
    double minNotional;  // Минимальный размер ордера во второй валюте
    double stepSize;     // Шаг объема в первой валюте
    double tickSize;     // Шаг цены во второй валюте
};
struct ExchangeInfo {
    uint64_t updateTime;  // Время последнего обновления
    std::map<std::string, MarketInfo>
        marketsInfo;  // Информация о рынках : std::string - название рынка,
                      // binanceAPI::MarketInfo - информация
};

// typedef std::map<std::string,double> balance_map;
// using balance_map = std::map<std::string, double>;
struct Balances {
    uint64_t updateTime;
    std::map<std::string, double> assets;
};

};  // namespace binanceAPI

namespace utils {
inline uint64_t getTimestamp() {
    uint64_t timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    return timestamp;
}
constexpr const char* PRECISIONS[9] = {"%.0f", "%.1f", "%.2f", "%.3f", "%.4f",
                                       "%.5f", "%.6f", "%.7f", "%.8f"};
inline std::string to_str(double num, size_t prec) {
    char buf[100];
    std::sprintf(buf, PRECISIONS[prec], num);
    return buf;
}
}  // namespace utils

class binanceClient  // Класс для реализации различных запросов, просмотра
                     // баланса и т.п.
{
  private:
    // std::shared_ptr<hv::WebSocketClient> websocket_client;
    // std::shared_ptr<hv::WebSocketClient> websocket_account;
    hv::WebSocketClient websocket_client;
    hv::WebSocketClient websocket_account;
    std::string _api_key;
    std::string _secret_key;

    std::string encryptWithHMAC(const char* data);

    uint64_t sended_reqs;
    uint64_t answers;
    std::map<uint64_t, std::string> answers_body;

    bool is_connected = false;

    binanceAPI::ExchangeInfo exchange_info;
    binanceAPI::Balances all_balances;

    bool is_account_connected = false;
    std::string listenKey = "";

  private:
    std::optional<binanceAPI::ExchangeInfo> getExchangeInfo();
    std::optional<binanceAPI::Balances> getBalance();

  public:
    ~binanceClient();

    static binanceClient& getClient();

    void Disconnect();
    void Connect();
    void Reconnect();

    binanceAPI::ExchangeInfo exchangeInfo();
    binanceAPI::Balances balances();
    json createLimitOrder(const std::string_view& trading_pair,
                          const std::string_view& order, double quantity,
                          double price);
    json createMarketOrder(const std::string_view& trading_pair,
                           const std::string_view& order, double quantity);
    json createMarketOrderQuote(const std::string_view& trading_pair,
                                const std::string_view& order,
                                double quote_quantity);
    uint64_t openOrder(json order);
    uint64_t openLimitOrder(const std::string_view& trading_pair,
                            const std::string_view& order, double quantity,
                            double price);
    uint64_t openMarketOrder(const std::string_view& trading_pair,
                             const std::string_view& order, double quantity);
    uint64_t openMarketOrderQuote(const std::string_view& trading_pair,
                                  const std::string_view& order,
                                  double quote_quantity);
    std::string getAnswer(
        uint64_t ans_num);  // сюда передается номер ответа, для каждого ответа
                            // использовать один раз! иначе упадем в бесконечное
                            // ожидание этого ответа

    void waitAnswers() {
        while (answers < sended_reqs)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void pingAccount();
    void orderAnswer(const std::string& msg);
  private:
    binanceClient();

    void getListenKey();
    void connectAccount();
    void disconnectAccount();
    void balanceAnswer(const std::string& msg);
};