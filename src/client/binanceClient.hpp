#pragma once

#include <cryptoClient.hpp>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <thread>
#include <tsmap.hpp>

#include <hv/WebSocketClient.h>
// #include <request_data.hpp>
namespace binanceAPI {

constexpr const char* client_id = "binance-cryptobot-client-v-0-2";
constexpr const char* BASE_ENDPOINT = "wss://ws-api.binance.com:9443/ws-api/v3";

constexpr const char* OPEN_ORDER = "order.place";
constexpr const char* CANCEL_ORDER = "order.cancel";
constexpr const char* ACCOUNT = "account.status";

constexpr const char* MARKET = "MARKET";
constexpr const char* LIMIT = "LIMIT";

constexpr const char* BUY = "BUY";
constexpr const char* SELL = "SELL";

constexpr const char* EXCHANGE_INFO_HTTP = "https://api.binance.com/api/v3/exchangeInfo?permissions=SPOT";
constexpr const char* ACCOUNT_HTTP = "https://api.binance.com/api/v3/account";
constexpr const char* USER_DATA_HTTP = "https://api.binance.com/api/v3/userDataStream";
constexpr const char* SYMBOL_PRICE_TICKER = "https://api.binance.com/api/v3/ticker/price";

constexpr const char* EXCHANGE_INFO = "exchangeInfo";

// Надо открыть вебсокет с url = USER_ENDPOINT + listenKey
constexpr const char* USER_ENDPOINT = "wss://stream.binance.com:9443/ws/";
constexpr const char* USER_DATA_STREAM_START = "userDataStream.start";
constexpr const char* USER_DATA_STREAM_STOP = "userDataStream.stop";
constexpr const char* USER_DATA_STREAM_PING = "userDataStream.ping";


const std::map<CryptoAPI::ORDER_STATUS,std::string> order_status =
{
	{CryptoAPI::ORDER_STATUS::NEW,"NEW"},
	{CryptoAPI::ORDER_STATUS::CANCELED,"CANCELED"},
	{CryptoAPI::ORDER_STATUS::EXPIRED,"EXPIRED"},
	{CryptoAPI::ORDER_STATUS::EXPIRED_IN_MATCH,"EXPIRED_IN_MATCH"},
	{CryptoAPI::ORDER_STATUS::FILLED,"FILLED"},
	{CryptoAPI::ORDER_STATUS::PARTIALLY_FILLED,"PARTIALLY_FILLED"},
	{CryptoAPI::ORDER_STATUS::REJECTED,"REJECTED"}
};

};  // namespace binanceAPI

class binanceClient : public cryptoClient   // Класс для реализации различных запросов, просмотра
                                            // баланса и т.п.
{
  private:

    const std::vector<std::string> SIDE;
    const std::vector<std::string> TIF; 
    hv::WebSocketClient websocket_client;
    hv::WebSocketClient websocket_account;
    std::string _api_key;
    std::string _secret_key;


    uint64_t sended_reqs;
    uint64_t answers;
    
    tsmap<uint64_t, std::string> answers_body;

    bool is_connected = false;

    CryptoAPI::ExchangeInfo exchange_info;
    CryptoAPI::Balances all_balances;

    bool is_account_connected = false;
    std::string listenKey = "";

  private:
    std::string encryptWithHMAC(const char* data);
    std::optional<CryptoAPI::ExchangeInfo> getExchangeInfo();
    std::optional<CryptoAPI::Balances> getBalance();
    binanceClient();
    void orderAnswer(const std::string& msg);
    void waitAnswers() { while (answers < sended_reqs)std::this_thread::sleep_for(std::chrono::milliseconds(10));}
    void getListenKey();
    void connectAccount();
    void disconnectAccount();
    void balanceAnswer(const std::string& msg);

	  CryptoAPI::OrderAnswer checkOrderAnswer(const std::string& answer, std::vector<CryptoAPI::ORDER_STATUS> await_statuses);
  public:
    ~binanceClient();

    static binanceClient& getClient();

    void Disconnect() override;
    void Connect() override;
    void Reconnect() override;

    CryptoAPI::ExchangeInfo exchangeInfo() override;

    virtual CryptoAPI::Balances balances() override;

    nlohmann::json createLimitOrder(const std::string_view& trading_pair,int side, double quantity,double price, 
															CryptoAPI::timeInForce tm = CryptoAPI::FOK) override;

    nlohmann::json createMarketOrder(const std::string_view& trading_pair,int side, double quantity) override;

    nlohmann::json createMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity) override;

    uint64_t sendRequest(nlohmann::json order) override;

    uint64_t openLimitOrder(const std::string_view& trading_pair,int side, double quantity,double price, 
    														CryptoAPI::timeInForce tm = CryptoAPI::FOK) override;

    uint64_t openMarketOrder(const std::string_view& trading_pair,int side, double quantity) override;

    uint64_t openMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity) override;

    void cancelOrder(const std::string_view& trading_pair,const std::string_view& order_id) override;

    CryptoAPI::OrderAnswer getAnswer(uint64_t ans_num, uint64_t wait_ms = 5'000) override;  // сюда передается номер ответа
																							 // И время в мс сколько можно ждать ответа
    CryptoAPI::OrderAnswer getAnswerSt(uint64_t ans_num, std::vector<CryptoAPI::ORDER_STATUS> await_statuses, uint64_t wait_ms = 10'000) override;
																							// Сюда передается еще ожидаемый статус ответа

    double getBalanceInCur(const std::string& cur) override;
    std::string SIDE_(int side) override;
    void pingAccount();
};