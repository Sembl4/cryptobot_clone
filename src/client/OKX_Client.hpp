#pragma once

#include <cryptoClient.hpp>
#include <optional>
#include <hv/WebSocketClient.h>
#include <tsmap.hpp>
// #include <request_data.hpp>

namespace OKX_API
{
    constexpr const char* REST_API = "https://www.okx.com";
    constexpr const char* SPOT_INSTRUMENTS = "/api/v5/public/instruments?instType=SPOT";
    constexpr const char* ACCOUNT_BALANCES = "/api/v5/account/balance";
    constexpr const char* PrivateWebsocket = "wss://ws.okx.com:8443/ws/v5/private";
    constexpr const char* PublicWebsocket = "wss://ws.okx.com:8443/ws/v5/public";

    const std::map<CryptoAPI::ORDER_STATUS,std::string> order_status =
    {
        {CryptoAPI::ORDER_STATUS::NEW,"live"},
        {CryptoAPI::ORDER_STATUS::CANCELED,"canceled"},
        {CryptoAPI::ORDER_STATUS::EXPIRED,"canceled"},
        {CryptoAPI::ORDER_STATUS::EXPIRED_IN_MATCH,"canceled"},
        {CryptoAPI::ORDER_STATUS::FILLED,"filled"},
        {CryptoAPI::ORDER_STATUS::PARTIALLY_FILLED,"partially_filled"},
        {CryptoAPI::ORDER_STATUS::REJECTED,"canceled"}
    };

    const std::map<int,std::string> cancelCodes = 
    {
        {0 ,"Order canceled by system"},
        {1 ,"Order canceled by user"},
        {2 ,"Order canceled: Pre reduce-only order canceled, due to insufficient margin in user position"},
        {3 ,"Order canceled: Risk cancelation was triggered. Pending order was canceled due to insufficient margin ratio and forced-liquidation risk."},
        {4 ,"Order canceled: Borrowings of crypto reached hard cap, order was canceled by system."},
        {6 ,"Order canceled: ADL order cancelation was triggered. Pending order was canceled due to a low margin ratio and forced-liquidation risk."},
        {9 ,"Order canceled: Insufficient balance after funding fees deducted."},
        {13,"Order canceled: FOK order was canceled due to incompletely filled."},
        {14,"Order canceled: IOC order was partially canceled due to incompletely filled."},
        {17,"Order canceled: Close order was canceled, due to the position was already closed at market price."},
        {20,"Cancel all after triggered"},
        {21,"Order canceled: The TP/SL order was canceled because the position had been closed"},
        {22,"Order canceled: Reduce-only orders only allow reducing your current position. System has already canceled this order."},
        {23,"Order canceled: Reduce-only orders only allow reducing your current position. System has already canceled this order."}
    };
};

class OKX_Client: public cryptoClient
{
private:
    OKX_Client();
    const std::vector<std::string> SIDE; 
    const std::vector<std::string> TIF; 
    // Информация о рынках биржи
    CryptoAPI::ExchangeInfo exchange_info;
    // Балансы аккаунта
    CryptoAPI::Balances     all_balances;
    // Вебсокет для совершения ордеров
    hv::WebSocketClient websocket_client;
    // Вебсокет для чтение ответов ордеров и баланса аккаунта
    hv::WebSocketClient websocket_orders_account;

    std::string _api_key;
    std::string _secret_key;
    std::string _passphrase;
    // Отправленные запросы
    uint64_t sended_reqs;
    // Полученные ответы (только ответы запросов)
    uint64_t answers;
    // Сами ответы от запросов
    // std::map<uint64_t, std::string> answers_body;
    tsmap<uint64_t, std::string> answers_body;

    // Флаги о соединении с вебсокетами
    bool is_client_connected = false;
    bool is_orders_connected = false;
public:
    ~OKX_Client();
    static OKX_Client& getClient();
    // Ниже идут одинаковые методы для клиентов всех бирж
    void Disconnect() override;
    void Connect() override;
    void Reconnect() override;

    CryptoAPI::ExchangeInfo exchangeInfo() override;

    CryptoAPI::Balances balances() override;

    nlohmann::json createLimitOrder(const std::string_view& trading_pair,int side, double quantity,double price, CryptoAPI::timeInForce tm = CryptoAPI::FOK)override;

    nlohmann::json createMarketOrder(const std::string_view& trading_pair,int side, double quantity) override;

    nlohmann::json createMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity) override;

    uint64_t sendRequest(nlohmann::json order) override;

    uint64_t openLimitOrder(const std::string_view& trading_pair,int side, double quantity,double price, CryptoAPI::timeInForce tm = CryptoAPI::FOK) override;

    uint64_t openMarketOrder(const std::string_view& trading_pair,int side, double quantity) override;

    uint64_t openMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity) override;

    void cancelOrder(const std::string_view& trading_pair,const std::string_view& order_id)override;

    // CryptoAPI::OrderAnswer getAnswer(uint64_t ans_num, uint64_t wait_ms = 600'000) override;

    CryptoAPI::OrderAnswer getAnswer(uint64_t ans_num, uint64_t wait_ms = 10'000) override;  // сюда передается номер ответа
																							 // И время в мс сколько можно ждать ответа
    CryptoAPI::OrderAnswer getAnswerSt(uint64_t ans_num, std::vector<CryptoAPI::ORDER_STATUS> await_statuses, uint64_t wait_ms = 10'000) override;
																							// Сюда передается еще ожидаемый статус ответа

    std::string SIDE_(int side) override;  

    double getBalanceInCur(const std::string& cur) override;

private:
    void connectClient();
    void connectOrders();

    void disconnectClient();
    void disconnectOrders();

    std::optional<CryptoAPI::ExchangeInfo> getInstruments();
    std::optional<CryptoAPI::Balances> getBalances();

    std::string getSignHMAC(const char* data);

    CryptoAPI::OrderAnswer checkOrderAnswer(const std::string& answer, std::vector<CryptoAPI::ORDER_STATUS> await_statuses);

    void clientMessage(const std::string& msg);
    void ordersMessage(const std::string& msg);
    char *base64(const unsigned char *input, int length);
};

