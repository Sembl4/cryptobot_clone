#include <openssl/hmac.h>
#include <binanceClient.hpp>
#include <config.hpp>
#include <logger.hpp>
#include <sstream>
#include <string>
using namespace hv;
using namespace binanceAPI;
using namespace utils;
using namespace binance::config;

// static uint64_t using_objects = 0;

binanceClient& binanceClient::getClient() 
{
    static binanceClient client;
    return client;
}

void binanceClient::Connect() 
{
    if (!is_connected) {
        // websocket_client = std::make_shared<WebSocketClient>();
        websocket_client.onmessage = [this](const std::string& msg) {
            this->orderAnswer(msg);
        };
        websocket_client.onopen = [this]() {
            Log("binanceClient websocket opened");
            this->is_connected = true;
        };
        websocket_client.onclose = [this]() {
            Log("binanceClient websocket closed");
            this->is_connected = false;
        };

        reconn_setting_t settings_;
        reconn_setting_init(&settings_);
        settings_.min_delay = 1000;
        settings_.max_delay = 10000;
        settings_.delay_policy = 2;
        websocket_client.setReconnect(&settings_);

        websocket_client.open(BASE_ENDPOINT);

        while (!websocket_client.isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }

        while (!is_connected)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Log("Connect:: binanceClient connected");

        exchange_info = getExchangeInfo().value();
        all_balances = getBalance().value();

        connectAccount();
    }
}

void binanceClient::Disconnect() 
{
    if (is_connected) {
        disconnectAccount();
        websocket_client.close();
        while (websocket_client.isConnected()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        while (is_connected)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        all_balances.updateTime = 0;
        all_balances.assets.clear();
        exchange_info.updateTime = 0;
        exchange_info.marketsInfo.clear();

        Log("Disconnect:: binanceClient disconnected");
    }
}

void binanceClient::Reconnect() 
{
    Log("binanceClient reconnect");
    Disconnect();

    Connect();
}

std::string binanceClient::getAnswer(uint64_t ans_num) 
{
    static uint64_t wait_time = 0;
    while (!answers_body.contains(ans_num)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        wait_time++;
        if(wait_time >= 10*1000)
        {
            wait_time = 0;
            return "{\"error\":\"NULL\"}";
        }
    }
    // std::this_thread::sleep_for(std::chrono::milliseconds(100));
    while (answers_body[ans_num] == "") {
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    std::string body = answers_body[ans_num];
    answers_body.erase(ans_num);
    return body;
}

binanceClient::binanceClient()
{
    _api_key = Config::Order().api_key;
    _secret_key = Config::Order().secret_key;

    answers = 0;
    sended_reqs = 0;

    is_connected = false;

    exchange_info.updateTime = 0;

    exchange_info.updateTime = 0;
}

binanceClient::~binanceClient() 
{
    waitAnswers();
    Disconnect();
    Log("binanceClient destroyed");
}

std::string binanceClient::encryptWithHMAC(const char* data) 
{
    unsigned char* result;
    static char res_hexstring[64];
    int result_len = 32;
    std::string signature;

    result = HMAC(EVP_sha256(), _secret_key.c_str(),
                  strlen((char*)_secret_key.c_str()),
                  const_cast<unsigned char*>(
                      reinterpret_cast<const unsigned char*>(data)),
                  strlen((char*)data), NULL, NULL);
    for (int i = 0; i < result_len; i++) {
        sprintf(&(res_hexstring[i * 2]), "%02x", result[i]);
    }

    for (int i = 0; i < 64; i++) {
        signature += res_hexstring[i];
    }

    return signature;
}

void binanceClient::orderAnswer(const std::string& msg) 
{
    answers++;// Log(answers);
    json j = json::parse(msg);
    this->answers_body[std::atoi(j["id"].get<std::string>().c_str())] = msg;
}

std::optional<binanceAPI::ExchangeInfo> binanceClient::getExchangeInfo() 
{
    HttpClient cli;
    HttpRequest req;
    req.method = HTTP_GET;
    req.url = EXCHANGE_INFO_HTTP;
    HttpResponse resp;
    int ret = cli.send(&req, &resp);
    if (ret != 0) return std::nullopt;

    json answer = json::parse(resp.body);

    ExchangeInfo info;
    // answer = answer["result"];
    info.updateTime = answer["serverTime"];
    for (json symbol : answer["symbols"]) {
        if (symbol["status"] == "TRADING") {
            MarketInfo market_info;

            market_info.baseAsset = symbol["baseAsset"];
            market_info.quoteAsset = symbol["quoteAsset"];
            market_info.basePrecision = symbol["baseAssetPrecision"];
            market_info.quotePrecision = symbol["quoteAssetPrecision"];
            for (const json& filt : symbol["filters"]) {
                if (filt["filterType"] == "PRICE_FILTER") {
                    market_info.tickSize =
                        std::atof(filt["tickSize"].get<std::string>().c_str());
                }
                if (filt["filterType"] == "LOT_SIZE") {
                    market_info.stepSize =
                        std::atof(filt["stepSize"].get<std::string>().c_str());
                }
                if (filt["filterType"] == "MIN_NOTIONAL") {
                    market_info.minNotional = std::atof(
                        filt["minNotional"].get<std::string>().c_str());
                }
            }
            info.marketsInfo[symbol["symbol"]] = market_info;
        }
    }
    return info;
}

binanceAPI::ExchangeInfo binanceClient::exchangeInfo() 
{
    if (getTimestamp() - exchange_info.updateTime >= 24 * 60 * 60 * 1000) {

        auto info = getExchangeInfo();
        if (info.has_value()) {
            exchange_info = info.value();
        } else {
            Log("binanceClient : exchangeInfo empty!");
            std::cout.flush();
            std::abort();
        }
        Log("binanceClient : exchangeInfo updated");
    }

    return exchange_info;
}

std::optional<binanceAPI::Balances> binanceClient::getBalance() 
{
    // Log("getBalance");
    HttpClient cli;
    HttpRequest req;
    req.method = HTTP_GET;
    req.url = ACCOUNT_HTTP;
    hv::QueryParams params;
    params["timestamp"] = std::to_string(getTimestamp());
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    req.query_params = params;
    req.headers["X-MBX-APIKEY"] = _api_key;
    HttpResponse resp;
    int ret = cli.send(&req, &resp);
    if (ret != 0) return std::nullopt;

    json answer = json::parse(resp.body);

    Balances bals;
    bals.updateTime = answer["updateTime"];
    for (json bal : answer["balances"]) {
        bals.assets[bal["asset"].get<std::string>()] =
            std::atof(bal["free"].get<std::string>().c_str());
    }
    return bals;
}

binanceAPI::Balances binanceClient::balances() 
{
    if (getTimestamp() - all_balances.updateTime >= (24 * 60 * 60 * 1000)) {
        // Log(all_balances.updateTime);
        auto bals = getBalance();
        if (bals.has_value()) {
            all_balances = bals.value();
        } else {
            Log("binanceClient : balances empty!");
            std::cout.flush();
            std::abort();
        }
        Log("binanceClient : balances updated");
    }
    return all_balances;
}

json binanceClient::createLimitOrder(const std::string_view& trading_pair,
                                     const std::string_view& order,
                                     double quantity, double price) 
{
    hv::QueryParams params;
    params["symbol"] = trading_pair;
    params["side"] = order;
    params["type"] = LIMIT;
    params["timeInForce"] = "FOK";
    params["quantity"] = to_str(quantity,exchange_info.marketsInfo[std::string(trading_pair)].basePrecision);
    params["price"] = to_str(price,exchange_info.marketsInfo[std::string(trading_pair)].quotePrecision);
    params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "100";
    params["apiKey"] = _api_key;
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    // static int i = 0;
    json j;
    j["id"] = client_id;
    j["method"] = OPEN_ORDER;
    for (auto pair : params) {
        j["params"][pair.first] = pair.second;
    }
    return j;
}

json binanceClient::createMarketOrder(const std::string_view& trading_pair,
                                      const std::string_view& order,
                                      double quantity) 
{
    hv::QueryParams params;
    params["symbol"] = trading_pair;
    params["side"] = order;
    params["type"] = MARKET;
    params["quantity"] = to_str(quantity,exchange_info.marketsInfo[std::string(trading_pair)].basePrecision);
    params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "100";
    params["apiKey"] = _api_key;
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    json j;
    j["id"] = client_id;
    j["method"] = OPEN_ORDER;
    for (auto pair : params) {
        j["params"][pair.first] = pair.second;
    }

    return j;
}

json binanceClient::createMarketOrderQuote(const std::string_view& trading_pair,
                                           const std::string_view& order,
                                           double quote_quantity) 
{
    hv::QueryParams params;
    params["symbol"] = trading_pair;
    params["side"] = order;
    params["type"] = MARKET;
    params["quoteOrderQty"] = to_str(quote_quantity,exchange_info.marketsInfo[std::string(trading_pair)].quotePrecision);
    params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "100";
    params["apiKey"] = _api_key;
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    json j;
    j["id"] = client_id;
    j["method"] = OPEN_ORDER;
    for (auto pair : params) {
        j["params"][pair.first] = pair.second;
    }

    return j;
}

uint64_t binanceClient::openOrder(json order) 
{
    sended_reqs++;
    order["id"] = std::to_string(sended_reqs);
    // Log("check connection");
    while (!is_connected)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // Log("send");
    websocket_client.send(order.dump());
    // Log("sended", sended_reqs);
    return sended_reqs;
}

uint64_t binanceClient::openLimitOrder(const std::string_view& trading_pair,
                                       const std::string_view& order,
                                       double quantity, double price) 
{
    return openOrder(createLimitOrder(trading_pair, order, quantity, price));
}

uint64_t binanceClient::openMarketOrder(const std::string_view& trading_pair,
                                        const std::string_view& order,
                                        double quantity) 
{
    return openOrder(createMarketOrder(trading_pair, order, quantity));
}

uint64_t binanceClient::openMarketOrderQuote(
    const std::string_view& trading_pair, const std::string_view& order,
    double quote_quantity) 
{
    return openOrder(createMarketOrderQuote(trading_pair, order, quote_quantity));
}

void binanceClient::getListenKey() 
{
    HttpClient cli;
    HttpRequest req;
    req.method = HTTP_POST;
    req.url = USER_DATA_HTTP;
    req.headers["X-MBX-APIKEY"] = _api_key;
    HttpResponse resp;
    int ret = cli.send(&req, &resp);
    if (ret != 0) return;

    json answer = json::parse(resp.body);

    listenKey = answer["listenKey"];
    Log("binanceClient:: get ListenKey", listenKey);
}

void binanceClient::connectAccount() 
{
    if (!is_account_connected) {
        // websocket_account = std::make_shared<WebSocketClient>();
        while (listenKey == "") getListenKey();

        websocket_account.onmessage = [this](const std::string& msg) {
            this->balanceAnswer(msg);
        };
        websocket_account.onopen = [this]() {
            Log("accountStream websocket opened");
            this->is_account_connected = true;
        };
        websocket_account.onclose = [this]() {
            Log("accountStream websocket closed");
            this->is_account_connected = false;
        };

        reconn_setting_t settings_;
        reconn_setting_init(&settings_);
        settings_.min_delay = 1000;
        settings_.max_delay = 10000;
        settings_.delay_policy = 2;
        websocket_account.setReconnect(&settings_);

        std::string url = std::string(USER_ENDPOINT) + listenKey;
        websocket_account.open(url.c_str());
        while (!websocket_account.isConnected())
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

        while (!is_account_connected)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Log("connectAccount:: accountStream connected");
    }
}

void binanceClient::disconnectAccount() 
{
    if (is_account_connected) {
        HttpClient cli;
        HttpRequest req;
        req.method = HTTP_DELETE;
        req.url = USER_DATA_HTTP;
        req.query_params["listenKey"] = listenKey;
        req.headers["X-MBX-APIKEY"] = _api_key;
        HttpResponse resp;
        int ret = cli.send(&req, &resp);
        if (ret != 0) return;
        std::cout << resp.body << std::endl;

        websocket_account.close();
        while (is_account_connected)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Log("disconnectAccount:: accountStream disconnect");
    }
}

void binanceClient::pingAccount() 
{
    if (is_account_connected) {
        HttpClient cli;
        HttpRequest req;
        req.method = HTTP_PUT;
        req.url = USER_DATA_HTTP;
        req.query_params["listenKey"] = listenKey;
        req.headers["X-MBX-APIKEY"] = _api_key;
        HttpResponse resp;
        int ret = cli.send(&req, &resp);
        if (ret != 0) return;
        std::cout << resp.body << std::endl;

        Log("pingAccount:: ping accountStream");
    }
}

void binanceClient::balanceAnswer(const std::string& msg) 
{
    json answer = json::parse(msg);
    if(answer["e"] == "outboundAccountPosition")
    {
        all_balances.updateTime = answer["E"];
        for(json bal : answer["B"])
        {
            all_balances.assets[bal["a"].get<std::string>()] = std::stod(bal["f"].get<std::string>());
        }
    }
}