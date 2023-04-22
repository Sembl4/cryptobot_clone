#pragma once

#include <axios.h>
#include <requests.h>
#include <chrono>
#include <iostream>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <sstream>

namespace binanceAPI {
constexpr const char* API_URL = "https://api.binance.com/api";
constexpr const char* API1_URL = "https://api1.binance.com/api";
constexpr const char* API2_URL = "https://api2.binance.com/api";
constexpr const char* API3_URL = "https://api3.binance.com/api";

constexpr const char* PRICE_V3 = "/v3/ticker/price";
constexpr const char* ACCOUNT_V3 = "/v3/account";

//#define TestMode

#ifdef TestMode
const std::string OPENORDER = "/v3/order/test";
#else
constexpr const char* OPENORDER = "/v3/order";
#endif

constexpr const char* BUY = "BUY";
constexpr const char* SELL = "SELL";
};  // namespace binanceAPI


namespace utils {
inline uint64_t getTimestamp() {
    uint64_t timestamp =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    return timestamp;
}
inline std::string to_str(double num)
{
    char buf[100];
    std::sprintf(buf,"%.8f",num);
    return buf;
}
}  // namespace utils


class binanceClient  // Класс для реализации различных запросов, просмотра
                     // баланса и т.п.
{
  private:
    std::shared_ptr<hv::HttpClient> http_client;
    std::string _api_key;
    std::string _secret_key;

    std::string encryptWithHMAC(const char* data);

    std::optional<double> balance;
    uint64_t sended_reqs;
    uint64_t answers;

  public:
    std::map<uint64_t, std::string> answers_body;

  public:
    binanceClient(std::string_view api_key, std::string_view secret_key);
    ~binanceClient();

    uint64_t getBalance(std::string_view currency);

    HttpRequestPtr createLimitOrder(const std::string_view& trading_pair,
                            const std::string_view& order, double quantity,
                            double price);

    HttpRequestPtr createMarketOrder(const std::string_view& trading_pair,
                            const std::string_view& order, double quantity);

    uint64_t openOrder(const HttpRequestPtr& req);

    uint64_t openLimitOrder(const std::string_view& trading_pair,
                            const std::string_view& order, double quantity,
                            double price);

    uint64_t openMarketOrder(const std::string_view& trading_pair,
                             const std::string_view& order, double quantity);

    void waitAnswers() {
        std::cout << "waitAnswers" << std::endl;
        while (answers < sended_reqs)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    void orderAnswer(const HttpResponsePtr& resp, uint64_t i);
};