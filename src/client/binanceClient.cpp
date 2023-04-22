#include <openssl/hmac.h>
#include <binanceClient.hpp>
#include <config.hpp>
#include <logger.hpp>
#include <sstream>
#include <string>
#include <hv/requests.h>
using namespace hv;
using namespace CryptoAPI;
using namespace binanceAPI;
using namespace utils;
using namespace common::config;
using namespace nlohmann;
using namespace std::chrono;

#define getTimestamp utils::getTimestamp<milliseconds>
static const uint64_t hours_24 = 24 * 60 * 60 * 1000;
// static void getTimestamp = utils::getTimestamp<milliseconds>;
// static uint64_t using_objects = 0;

binanceClient& binanceClient::getClient() 
{
    static binanceClient client;
    return client;
}

binanceClient::binanceClient():SIDE({"SELL","BUY"}), TIF({"GTC","IOC","FOK"})
{
    _api_key = Config::Order().api_key;
    _secret_key = Config::Order().secret_key;
    platform = BINANCE;
    working_asset = Config::Market().working_asset;
    commission_asset = "BNB";

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

        for(auto info : exchange_info.marketsInfo)
        {
            // Если комиссионная валюта является первой в рынке а рабочая второй, то запоминаем все что нужно
            if(commission_asset == info.second.baseAsset && working_asset == info.second.quoteAsset)  
                working_market = info.second;
            // Если рабочая валюта является первой в рынке а комиссионная второй, то запоминаем все что нужно
            if(working_asset == info.second.baseAsset && commission_asset == info.second.quoteAsset)
                working_market = info.second;
        }
        Log("Working market:", working_market.value().baseAsset,working_market.value().quoteAsset,working_market.value().minNotional);

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

CryptoAPI::OrderAnswer binanceClient::checkOrderAnswer(const std::string& answer,std::vector<CryptoAPI::ORDER_STATUS> await_statuses)
{
    // bool is_done;
    OrderAnswer ord_answer;
    ord_answer.is_done = false;
    ord_answer.text = answer;
    json parsed_body = json::parse(answer);
    if (parsed_body.contains("error"))
    { 
        ord_answer.is_done = false;
        ord_answer.base_filled_qty = 0.0;
        ord_answer.quote_filled_qty = 0.0;
    }
    else if(parsed_body.contains("result"))
    {
        parsed_body = parsed_body["result"];
        ord_answer.ts = parsed_body["transactTime"];
        // Если ответ содержит status и status равен "FILLED", то запрос
        // считается выполненным Иначе запрос считается не выполненным
        if (parsed_body.contains("status")) {
            // Log("answer contains status ",
            // parsed_body["status"].get<std::string>() == "FILLED");
            for(auto stat : await_statuses)
            {
                if(parsed_body["status"] == order_status.at(stat))
                {
                    Log("Answer is good", parsed_body["status"]);
                    ord_answer.status = stat;
                    ord_answer.is_done = true;
                    break;
                }
            }
            ord_answer.base_filled_qty = std::stod(parsed_body["executedQty"].get<std::string>());
            ord_answer.quote_filled_qty = std::stod(parsed_body["cummulativeQuoteQty"].get<std::string>());
            if(!ord_answer.is_done)
            {
                // Log("Answer is bad", parsed_body["status"]);
                for(auto st : order_status)
                    if(parsed_body["status"] == st.second)
                    {
                        ord_answer.status = st.first;
                        break;
                    }
            }
        } else 
        {
            ord_answer.is_done = false;
            ord_answer.base_filled_qty = 0.0;
            ord_answer.quote_filled_qty = 0.0;
        }
    }
    else if(parsed_body.contains("e"))
    {
        // Log("Answer with e");
        if(parsed_body["e"] == "executionReport")
        {
            ord_answer.ts = parsed_body["T"];
            for(auto stat : await_statuses)
            {
                if(parsed_body["X"] == order_status.at(stat))
                {
                    Log("Answer is good", parsed_body["X"]);
                    ord_answer.status = stat;
                    ord_answer.is_done = true;
                    break;
                }
            }
            ord_answer.base_filled_qty = std::stod(parsed_body["z"].get<std::string>());
            ord_answer.quote_filled_qty = std::stod(parsed_body["Z"].get<std::string>());
            if(!ord_answer.is_done)
            {
                // Log("Answer is bad", parsed_body["X"]);
                for(auto st : order_status)
                    if(parsed_body["X"] == st.second)
                    {
                        ord_answer.status = st.first;
                        break;
                    }
            }
        }
    }
    // return is_done;
    return ord_answer;
}

CryptoAPI::OrderAnswer binanceClient::getAnswer(uint64_t ans_num,uint64_t wait_ms) 
{
    OrderAnswer answer;
    // answer.is_done = false;
    uint64_t start_ts = getTimestamp();
    try
    {
    while(!answer.is_done)
    {
        if(getTimestamp() - start_ts >= wait_ms)
        {
            answer.is_done = false;
            answer.text += "{\"error\":\"The answer didn't come. Skip...\"}";
            return answer;
        }
        if(answers_body.isLocked()) continue;
        // Ждем появления в ответах нужного ответа
        if(!answers_body.contains(ans_num)) continue;
        // Удостоверимся что ответ не пустой
        // if(answers_body[ans_num] == "") continue;
        
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        // тут нас интересуют только ответы которые либо точно выполнены, либо точно не выполнены
        answer = checkOrderAnswer(answers_body.at(ans_num),{FILLED,EXPIRED}); 
    }
    if(answer.status == EXPIRED) answer.is_done = false;

    answers_body.erase(ans_num);
    }catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (getAnswer):",answer.text);
        LogFlush();
        answer.was_exception = true;
    }
    return answer;
}

CryptoAPI::OrderAnswer binanceClient::getAnswerSt(uint64_t ans_num, std::vector<CryptoAPI::ORDER_STATUS> await_statuses, uint64_t wait_ms)
{
    OrderAnswer answer;
    answer.is_done = false;
    uint64_t start_ts = getTimestamp();
    try
    {
        // uint64_t counter = 0;
        while(true)
        {
            if(getTimestamp() - start_ts >= wait_ms)
            {
                answer.is_done = false;
                answer.status = NEW;
                answer.text += {"{\"error\":\"ANS didn't come\"}"};
                return answer;
            }
            if(answers_body.isLocked()) continue;
            // Ждем появления в ответах нужного ответа
            if(!answers_body.contains(ans_num)) continue;
            // Удостоверимся что ответ не пустой
            // if(answers_body[ans_num] == "") continue;

            OrderAnswer temp_answer = checkOrderAnswer(answers_body.at(ans_num),await_statuses);
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
            if(temp_answer.is_done) // Если пришел хороший ответ
            {
                if(temp_answer.status == await_statuses.front()) // Если статус ответа равен первому элементу
                {
                    answer = temp_answer; // то сразу выводим его и бежим дальше
                    Log("All good, break");
                    break;
                }else // Если не равен первому элементу, то надо подумать
                {
                    if(temp_answer != answer) // Если текущий ответ равен предыдущему, значит мы сюда еще не заходили
                    {
                        answer = temp_answer; // Сохраняем этот ответ
                        Log("Wait 100 ms, if second answer == current, it will done.");
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    }else // тут мы появляемся если мы уже подождали 100 мс и нам ничего нового не пришло
                    {
                        // тогда мы просто выводим этот ответ (что-то выполнилось)
                        Log("Something done, break!");
                        break;
                    }
                }
            }
        }
        answers_body.erase(ans_num); // ну и удалем этот ответ из мапы ответов
    }catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (getAnswer with statuses)");
        LogFlush();
        answer.was_exception = true;
    }
    return answer;
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
    // Log(msg);
    try
    {
        json j = json::parse(msg);
        if(j.contains("result"))
        {
            if(j["result"].contains("clientOrderId"))
            {
                // if(j["result"]["status"] != "NEW") 
                while(answers_body.isLocked()){}
                this->answers_body.insert(std::atoi(j["result"]["clientOrderId"].get<std::string>().c_str()),msg);
            }
            else 
                Log("ERROR Message :~!",msg,"!~");
        }else
        {
            if(j.contains("id"))
            {
                while(answers_body.isLocked()){}
                this->answers_body.insert(std::atoi(j["id"].get<std::string>().c_str()),msg);
            }
            else Log("ERROR Message :~!",msg,"!~");
        }
    }catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (orderAnswer):",msg);
        LogFlush();
    }
}

std::optional<ExchangeInfo> binanceClient::getExchangeInfo() 
{
    HttpClient cli;
    HttpRequest req;
    req.method = HTTP_GET;
    req.url = EXCHANGE_INFO_HTTP;
    HttpResponse resp;
    int ret = cli.send(&req, &resp);
    if (ret != 0) return std::nullopt;

    ExchangeInfo info;
    try
    {
        json answer = json::parse(resp.body);

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
                        market_info.tickSize = std::atof(filt["tickSize"].get<std::string>().c_str());
                    }
                    if (filt["filterType"] == "LOT_SIZE") {
                        market_info.stepSize = std::atof(filt["stepSize"].get<std::string>().c_str());
                    }
                    if (filt["filterType"] == "MIN_NOTIONAL") {
                        market_info.minNotional = std::atof(filt["minNotional"].get<std::string>().c_str());
                    }
                }
                info.marketsInfo[symbol["symbol"]] = market_info;
            }
        }
    }catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (getExchangeInfo)");
        LogFlush();
    }
    return info;
}

ExchangeInfo binanceClient::exchangeInfo() 
{
    if (getTimestamp() - exchange_info.updateTime >= hours_24) {

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

std::optional<Balances> binanceClient::getBalance() 
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

    Balances bals;
    try
    {
        json answer = json::parse(resp.body);

        bals.updateTime = getTimestamp();
        for (json bal : answer["balances"]) {
            bals.assets[bal["asset"].get<std::string>()] =
                std::atof(bal["free"].get<std::string>().c_str());
        }
    }
    catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (getBalance)");
        LogFlush();
    }
    return bals;
}

Balances binanceClient::balances() 
{
    if (getTimestamp() - all_balances.updateTime >= hours_24) {
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

json binanceClient::createLimitOrder(const std::string_view& trading_pair,int side,double quantity, double price, timeInForce tm) 
{
    sended_reqs++;
    hv::QueryParams params;
    params["symbol"] = trading_pair;
    params["side"] = SIDE.at(side);
    params["newClientOrderId"] = std::to_string(sended_reqs);
    params["type"] = LIMIT;
    params["timeInForce"] = TIF.at(tm);
    params["quantity"] = to_str(quantity,exchange_info.marketsInfo[std::string(trading_pair)].basePrecision);
    params["price"] = to_str(price,exchange_info.marketsInfo[std::string(trading_pair)].quotePrecision);
    params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "10000";
    params["apiKey"] = _api_key;
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    // static int i = 0;
    json j;
    j["id"] = std::to_string(sended_reqs);
    j["method"] = OPEN_ORDER;
    for (auto pair : params) {
        j["params"][pair.first] = pair.second;
    }
    return j;
}

json binanceClient::createMarketOrder(const std::string_view& trading_pair,int side,double quantity) 
{
    sended_reqs++;
    hv::QueryParams params;
    params["symbol"] = trading_pair;
    params["side"] = SIDE.at(side);
    params["newClientOrderId"] = std::to_string(sended_reqs);
    params["type"] = MARKET;
    params["quantity"] = to_str(quantity,exchange_info.marketsInfo[std::string(trading_pair)].basePrecision);
    params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "100";
    params["apiKey"] = _api_key;
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    json j;
    j["id"] = std::to_string(sended_reqs);
    j["method"] = OPEN_ORDER;
    for (auto pair : params) {
        j["params"][pair.first] = pair.second;
    }

    return j;
}

json binanceClient::createMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity) 
{
    sended_reqs++;
    hv::QueryParams params;
    params["symbol"] = trading_pair;
    params["side"] = SIDE.at(side);
    params["newClientOrderId"] = std::to_string(sended_reqs);
    params["type"] = MARKET;
    params["quoteOrderQty"] = to_str(quote_quantity,exchange_info.marketsInfo[std::string(trading_pair)].quotePrecision);
    params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "100";
    params["apiKey"] = _api_key;
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    json j;
    j["id"] = std::to_string(sended_reqs);
    j["method"] = OPEN_ORDER;
    for (auto pair : params) {
        j["params"][pair.first] = pair.second;
    }

    return j;
}

uint64_t binanceClient::sendRequest(json order) 
{
    // sended_reqs++;
    // order["id"]= std::to_string(sended_reqs);
    // hv::QueryParams pars = order["params"];
    // order["params"]["newClientOrderId"] = std::to_string(sended_reqs);
    // Log("check connection");
    while (!is_connected)
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    // Log("send");
    websocket_client.send(order.dump());
    // Log("sended", sended_reqs);
    return std::atoi(order["id"].get<std::string>().c_str());
}

uint64_t binanceClient::openLimitOrder(const std::string_view& trading_pair,int side,double quantity, double price,CryptoAPI::timeInForce tm) 
{
    return sendRequest(createLimitOrder(trading_pair, side, quantity, price,tm));
}

uint64_t binanceClient::openMarketOrder(const std::string_view& trading_pair,int side,double quantity) 
{
    return sendRequest(createMarketOrder(trading_pair, side, quantity));
}

uint64_t binanceClient::openMarketOrderQuote(const std::string_view& trading_pair, int side,double quote_quantity) 
{
    return sendRequest(createMarketOrderQuote(trading_pair, side, quote_quantity));
}

void binanceClient::cancelOrder(const std::string_view& trading_pair,const std::string_view& order_id)
{
    Log("Cancel Order", order_id);
    sended_reqs++;
    hv::QueryParams params;
    params["symbol"] = trading_pair;
    params["origClientOrderId"] = order_id;
    params["newClientOrderId"] = std::to_string(sended_reqs);
    params["timestamp"] = std::to_string(getTimestamp());
    params["apiKey"] = _api_key;
    params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    json j;
    j["id"] = std::to_string(sended_reqs);
    j["method"] = CANCEL_ORDER;
    for (auto pair : params) {
        j["params"][pair.first] = pair.second;
    }

    auto ans = getAnswer(sendRequest(j));

    Log(ans.is_done,ans.text);
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

    try
    {
        json answer = json::parse(resp.body);

        listenKey = answer["listenKey"];
        Log("binanceClient:: get ListenKey", listenKey);
    }catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (getListenKey):",resp.body);
        LogFlush();
    }
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
    try
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
        if(answer["e"] == "executionReport")
        {
            Log("new order answer",msg);
            while(answers_body.isLocked()){}
            answers_body.insert(std::atoi(answer["c"].get<std::string>().c_str()),msg);
            // Log("new order answer",answers_body[std::atoi(answer["c"].get<std::string>().c_str())]);
        }
    }catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (balanceAnswer):", msg);
        LogFlush();
    }
}

std::string binanceClient::SIDE_(int side)
{
    if(side >= SIDE.size() && side < 0) return "";
    return SIDE.at(side);
}
double binanceClient::getBalanceInCur(const std::string& cur)
{
    Log("getBalanceInCur", cur);
    auto balances = getBalance().value();
    double cur_bal = balances.assets[cur];
    for(auto asset : balances.assets)
    {
        if(asset.first != cur && asset.second > 0.0)
            for(auto market : exchange_info.marketsInfo)
            {
                if(market.second.baseAsset == cur && market.second.quoteAsset == asset.first)
                {
                    HttpClient cli;
                    HttpRequest req;
                    req.method = HTTP_GET;
                    req.url = SYMBOL_PRICE_TICKER;
                    req.query_params["symbol"] = market.first;
                    HttpResponse resp;
                    int ret = cli.send(&req, &resp);
                    if (ret == 0)
                    {
                        try
                        {
                            json answer = json::parse(resp.body);
                            cur_bal += asset.second / std::atof(answer["price"].get<std::string>().c_str());
                        }catch(...)
                        {
                            Log("EXEPTION in getBalanceInCur", cur);
                        }   
                    }
                }
                if(market.second.baseAsset == asset.first && market.second.quoteAsset == cur)
                {
                    HttpClient cli;
                    HttpRequest req;
                    req.method = HTTP_GET;
                    req.url = SYMBOL_PRICE_TICKER;
                    req.query_params["symbol"] = market.first;
                    HttpResponse resp;
                    int ret = cli.send(&req, &resp);
                    if (ret == 0)
                    {
                        try
                        {
                            json answer = json::parse(resp.body);
                            cur_bal += asset.second * std::atof(answer["price"].get<std::string>().c_str());
                        }catch(...)
                        {
                            Log("EXEPTION in getBalanceInCur", cur);
                        }   
                    }
                }
            }
    }
    return cur_bal;
}