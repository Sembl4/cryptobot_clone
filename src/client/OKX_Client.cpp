#include <OKX_Client.hpp>
#include <logger.hpp>
#include <openssl/evp.h>
#include <openssl/hmac.h>
#include <hv/requests.h>
#include <iomanip>
#include <config.hpp>

using namespace hv;
using namespace OKX_API;
using namespace nlohmann;
using namespace CryptoAPI;
using namespace std::chrono;
using namespace common::config;

static std::function getTimestamp = utils::getTimestamp<milliseconds>;

static const uint64_t hours_24 = 24 * 60 * 60 * 1000;

static std::string currentISO8601TimeUTC() {
    auto now = std::chrono::system_clock::now();
    auto itt = std::chrono::system_clock::to_time_t(now);
    std::ostringstream ss;
    ss << std::put_time(gmtime(&itt), "%FT%TZ");
    return ss.str();
}


OKX_Client& OKX_Client::getClient() 
{
    static OKX_Client client;
    return client;
}

OKX_Client::OKX_Client():SIDE({"sell","buy"}),TIF({"limit","ioc","fok"})
{
    _api_key = Config::Order().api_key;//"2fbe8a14-5e98-460b-8796-e785456cd94a";
    _passphrase = Config::Order().pass_phrase;
    _secret_key = Config::Order().secret_key;
    platform = OKX;
    working_asset = Config::Market().working_asset;
    commission_asset = std::nullopt;
    sended_reqs = 0;
    answers = 0;
}

OKX_Client::~OKX_Client()
{
    Disconnect();
    Log("OKX client destroyed");
}

void OKX_Client::connectClient()
{
    if(!is_client_connected)
    {
        websocket_client.onmessage = [this](const std::string& msg) {
            this->clientMessage(msg);
        };
        websocket_client.onopen = [this]() {
            Log("OKX client websocket opened");
            this->is_client_connected = true;
        };
        websocket_client.onclose = [this]() {
            Log("OKX client websocket closed");
            this->is_client_connected = false;
        };

        reconn_setting_t settings_;
        reconn_setting_init(&settings_);
        settings_.min_delay = 1000;
        settings_.max_delay = 10000;
        settings_.delay_policy = 2;
        websocket_client.setReconnect(&settings_);

        websocket_client.open(PrivateWebsocket);

        while(!is_client_connected) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        json j;
        j["op"] = "login";
        hv::QueryParams params;
        params["apiKey"] = _api_key;
        params["passphrase"] = _passphrase;
        params["timestamp"] = std::to_string(utils::getTimestamp<seconds>());
        params["sign"] = getSignHMAC(std::string(params["timestamp"] + "GET" + "/users/self/verify").c_str());

        for(auto par : params)
        {
            j["args"][0][par.first] = par.second;
        }

        exchange_info = getInstruments().value();
        all_balances = getBalances().value();

        websocket_client.send(j.dump());
    }
}
void OKX_Client::connectOrders()
{
    if(!is_orders_connected)
    {
        websocket_orders_account.onmessage = [this](const std::string& msg) {
            this->ordersMessage(msg);
        };
        websocket_orders_account.onopen = [this]() {
            Log("OKX orders & balance websocket opened");
            this->is_orders_connected = true;
        };
        websocket_orders_account.onclose = [this]() {
            Log("OKX orders & balance websocket closed");
            this->is_orders_connected = false;
        };

        reconn_setting_t settings_;
        reconn_setting_init(&settings_);
        settings_.min_delay = 1000;
        settings_.max_delay = 10000;
        settings_.delay_policy = 2;
        websocket_orders_account.setReconnect(&settings_);

        websocket_orders_account.open(PrivateWebsocket);

        while(!is_orders_connected) std::this_thread::sleep_for(std::chrono::milliseconds(1));
        json j;
        j["op"] = "login";
        hv::QueryParams params;
        params["apiKey"] = _api_key;
        params["passphrase"] = _passphrase;
        params["timestamp"] = std::to_string(utils::getTimestamp<seconds>());
        params["sign"] = getSignHMAC(std::string(params["timestamp"] + "GET" + "/users/self/verify").c_str());

        for(auto par : params)
        {
            j["args"][0][par.first] = par.second;
        }
        websocket_orders_account.send(j.dump());

        std::this_thread::sleep_for(std::chrono::milliseconds(1000));

        // Подписка на ордеры и баланс
        json sub;
        sub["op"] = "subscribe";
        sub["args"][0]["channel"]= "orders";
        sub["args"][0]["instType"] = "SPOT";
        sub["args"][1]["channel"]= "account";

        websocket_orders_account.send(sub.dump());
    }
}

void OKX_Client::disconnectClient()
{
    if(is_client_connected)
    {
        websocket_client.close();
        while (is_client_connected)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Log("OKX client websoctet disconnected");
        
    }
}
void OKX_Client::disconnectOrders()
{
    if(is_orders_connected)
    {
        websocket_orders_account.close();
        while (is_orders_connected)
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        Log("OKX orders & balance websoctet disconnected");
    }
}

void OKX_Client::Disconnect()
{
    disconnectClient();
    disconnectOrders();
    all_balances.updateTime = 0;
    all_balances.assets.clear();
    exchange_info.updateTime = 0;
    exchange_info.marketsInfo.clear();
}

void OKX_Client::Connect()
{
    connectClient();
    connectOrders();
}

void OKX_Client::Reconnect()
{
    Disconnect();
    Connect();
}

std::optional<CryptoAPI::ExchangeInfo> OKX_Client::getInstruments()
{
    Log("OKX_Client: getInstruments");
    HttpClient cli;
    HttpRequest req;
    req.method = HTTP_GET;
    req.url = std::string(REST_API) + std::string(SPOT_INSTRUMENTS);
    HttpResponse resp;
    int ret = cli.send(&req, &resp);
    if (ret != 0) return std::nullopt;

    ExchangeInfo info;
    try
    {
        json instruments = json::parse(resp.body);
        size_t prec;
        for(json market : instruments["data"])
        {
            MarketInfo mark;
            mark.baseAsset = market["baseCcy"];
            mark.quoteAsset = market["quoteCcy"];
            mark.stepSize = std::atof(market["lotSz"].get<std::string>().c_str());
            mark.tickSize = std::atof(market["tickSz"].get<std::string>().c_str());
            mark.minNotional = std::atof(market["minSz"].get<std::string>().c_str());
            // Вычисляем точность основной валюты
            prec = 0;
            while(mark.stepSize < pow(10,(-1)*(int)prec))++prec;
            mark.basePrecision = prec;
            // Вычисляем точность второй валюты
            prec = 0;
            while(mark.tickSize < pow(10,(-1)*(int)prec))++prec;
            mark.quotePrecision = prec;
            
            info.marketsInfo[market["instId"]] = mark;
        }
        info.updateTime = getTimestamp();
    }catch(...)
    {
        Log("EXEPTION :: OKX_Client.cpp (getInstruments)");
        LogFlush();
    }
    return info;
}

CryptoAPI::ExchangeInfo OKX_Client::exchangeInfo()
{
    if(getTimestamp() - exchange_info.updateTime > hours_24)
    {
        exchange_info = getInstruments().value();
    }
    return exchange_info;
}

std::optional<CryptoAPI::Balances> OKX_Client::getBalances()
{
    Log("OKX_Client: getBalances");
    HttpClient cli;
    HttpRequest req;
    req.method = HTTP_GET;
    req.url = std::string(REST_API) + std::string(ACCOUNT_BALANCES);
    req.headers["OK-ACCESS-KEY"] = _api_key;
    std::chrono::system_clock::now();
    std::string ts = currentISO8601TimeUTC();
    req.headers["OK-ACCESS-SIGN"] = getSignHMAC(std::string(ts + "GET" + ACCOUNT_BALANCES).c_str());
    req.headers["OK-ACCESS-TIMESTAMP"] = ts;
    req.headers["OK-ACCESS-PASSPHRASE"] = _passphrase;    
    HttpResponse resp;
    int ret = cli.send(&req, &resp);
    if (ret != 0) return std::nullopt;

    Balances bal;
    try
    {
        json balances = json::parse(resp.body);

        balances = balances["data"][0];
        bal.updateTime = std::atoll(balances["uTime"].get<std::string>().c_str());
        for(json asset : balances["details"])
        {
            bal.assets[asset["ccy"]] = std::atof(asset["cashBal"].get<std::string>().c_str());
        }
    }catch(...)
    {
        Log("EXEPTION :: OKX_Client.cpp (getBances)");
        LogFlush();
    }
    return bal;
}

CryptoAPI::Balances OKX_Client::balances() 
{
    if(getTimestamp() - all_balances.updateTime > hours_24)
    {
        all_balances = getBalances().value();
    }
    return all_balances;
}

nlohmann::json OKX_Client::createLimitOrder(const std::string_view& trading_pair,int side, double quantity, double price, CryptoAPI::timeInForce tm)
{
    json j;
    j["id"] = "OKX_CLIENT_LIMIT_FOK";
    j["op"] = "order";
    j["args"][0]["side"] = SIDE.at(side);
    j["args"][0]["instId"] = trading_pair;
    j["args"][0]["tdMode"] = "cash";
    j["args"][0]["ordType"] = TIF.at(tm);
    j["args"][0]["sz"] = utils::to_str(quantity,exchange_info.marketsInfo[std::string(trading_pair)].basePrecision);
    j["args"][0]["px"] = utils::to_str(price,exchange_info.marketsInfo[std::string(trading_pair)].quotePrecision);

    return j;
}
nlohmann::json OKX_Client::createMarketOrder(const std::string_view& trading_pair,int side, double quantity)
{
    json j;
    j["id"] = "OKX_CLIENT_MARKET";
    j["op"] = "order";
    j["args"][0]["side"] = SIDE.at(side);
    j["args"][0]["instId"] = trading_pair;
    j["args"][0]["tdMode"] = "cash";
    j["args"][0]["ordType"] = "market";
    j["args"][0]["tgtCcy"] = "base_ccy";
    j["args"][0]["sz"] = utils::to_str(quantity,exchange_info.marketsInfo[std::string(trading_pair)].basePrecision);
    return j;
}
nlohmann::json OKX_Client::createMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity)
{
    json j;
    j["id"] = "OKX_CLIENT_MARKET_QUOTE";
    j["op"] = "order";
    j["args"][0]["side"] = SIDE.at(side);
    j["args"][0]["instId"] = trading_pair;
    j["args"][0]["tdMode"] = "cash";
    j["args"][0]["ordType"] = "market";
    j["args"][0]["tgtCcy"] = "quote_ccy";
    j["args"][0]["sz"] = utils::to_str(quote_quantity,exchange_info.marketsInfo[std::string(trading_pair)].quotePrecision);
    return j;
}
uint64_t OKX_Client::sendRequest(nlohmann::json order)
{
    sended_reqs++;
    order["args"][0]["clOrdId"] = std::to_string(sended_reqs);
    order["id"] = order["args"][0]["clOrdId"];
    while (!is_client_connected) std::this_thread::sleep_for(std::chrono::milliseconds(1));
    websocket_client.send(order.dump());
    Log(sended_reqs, order.dump());
    return sended_reqs;
}
uint64_t OKX_Client::openLimitOrder(const std::string_view& trading_pair,int side, double quantity,double price,CryptoAPI::timeInForce tm)
{
    return sendRequest(createLimitOrder(trading_pair, side, quantity, price,tm));
}
uint64_t OKX_Client::openMarketOrder(const std::string_view& trading_pair,int side, double quantity)
{
    return sendRequest(createMarketOrder(trading_pair, side, quantity));
}
uint64_t OKX_Client::openMarketOrderQuote(const std::string_view& trading_pair,int side,double quote_quantity)
{
    return sendRequest(createMarketOrderQuote(trading_pair, side, quote_quantity));
}

void OKX_Client::cancelOrder(const std::string_view& trading_pair,const std::string_view& order_id)
{
    json j;
    j["id"] = order_id;
    j["op"] = "cancel-order";
    j["args"][0]["instId"] = trading_pair;
    j["args"][0]["clOrdId"] = order_id;

    websocket_client.send(j.dump());
    auto ans = getAnswer(std::atoi(std::string(order_id).c_str()));
    Log(ans.is_done,ans.text);
    
}

CryptoAPI::OrderAnswer OKX_Client::checkOrderAnswer(const std::string& answer, std::vector<CryptoAPI::ORDER_STATUS> await_statuses)
{
    bool is_done;
    OrderAnswer ord_answer;
    ord_answer.is_done = false;
    ord_answer.text = answer;

    json parsed_body = json::parse(ord_answer.text);

    if(parsed_body.contains("op"))
    {
        ord_answer.is_done = false;
        ord_answer.base_filled_qty = 0.0;
        ord_answer.quote_filled_qty = 0.0;
    }else
    {
        parsed_body = parsed_body["data"];
        if(parsed_body.contains("state"))
        {
            ord_answer.ts = std::atoi(parsed_body["uTime"].get<std::string>().c_str());
            for(auto stat : await_statuses)
            {
                ord_answer.is_done = parsed_body["state"] == order_status.at(stat);
                
                if(ord_answer.is_done) 
                { 
                    ord_answer.status = stat;
                    break;
                }
            }
            if(!ord_answer.is_done)
                for(auto stat : order_status)
                    if(parsed_body["state"] == stat.second)
                        ord_answer.status = stat.first;

            MarketInfo info = exchange_info.marketsInfo[parsed_body["instId"].get<std::string>()];
            double fillSz = std::stod(parsed_body["accFillSz"].get<std::string>());
            double avgPx = std::stod(parsed_body["avgPx"].get<std::string>());
            double fee = std::stod(parsed_body["fillFee"].get<std::string>());
            if(parsed_body["fillFeeCcy"].get<std::string>() == info.baseAsset)
            {
                ord_answer.base_filled_qty = fillSz + fee;
                ord_answer.quote_filled_qty = fillSz * avgPx + fee * avgPx;
            }else
            {
                ord_answer.base_filled_qty = fillSz + fee / avgPx;
                ord_answer.quote_filled_qty = fillSz * avgPx + fee;
            }
        }else
        {
            ord_answer.is_done = false;
            ord_answer.base_filled_qty = 0.0;
            ord_answer.quote_filled_qty = 0.0;
        }
    }
    return ord_answer;
}

CryptoAPI::OrderAnswer OKX_Client::getAnswer(uint64_t ans_num,uint64_t wait_ms)
{
    OrderAnswer answer;
    answer.is_done = false;
    uint64_t start_ts = getTimestamp();
    try
    {
        while(!answer.is_done)
        {
            if(getTimestamp() - start_ts >= wait_ms)
            {
                answer.is_done = false;
                answer.text = "{\"error\":\"The answer didn't come. Skip...\"}";
                return answer;
            }


            // Ждем появления в ответах нужного ответа
            // if(!answers_body.contains(ans_num)) co            
            // Удостоверимся что ответ не пустой
            // if(answers_body[ans_num] == "") continue;
            
            if(answers_body.isLocked()) continue;
            if(!answers_body.contains(ans_num)) continue;

            answer = checkOrderAnswer(answers_body.at(ans_num),{FILLED,CANCELED});
        }
        if(answer.status == CANCELED || 
            answer.status == EXPIRED || 
            answer.status == EXPIRED_IN_MATCH || 
            answer.status == REJECTED) answer.is_done = false;

        answers_body.erase(ans_num);
    }catch(...)
    {
        Log("EXEPTION :: OKX_Client.cpp (getAnswer):",answer.text);
        LogFlush();
    }
    return answer;
}

CryptoAPI::OrderAnswer OKX_Client::getAnswerSt(uint64_t ans_num, std::vector<CryptoAPI::ORDER_STATUS> await_statuses, uint64_t wait_ms)
{
    OrderAnswer answer;
    answer.is_done = false;
    uint64_t start_ts = getTimestamp();
    try
    {
        while(!answer.is_done)
        {
            if(getTimestamp() - start_ts >= wait_ms)
            {
                answer.is_done = false;
                answer.text = "{\"error\":\"The answer didn't come. Skip...\"}";
                return answer;
            }
            if(answers_body.isLocked()) continue;
            if(!answers_body.contains(ans_num)) continue;
            // Ждем появления в ответах нужного ответа
            // if(!answers_body.contains(ans_num)) continue;
            // Удостоверимся что ответ не пустой
            // if(answers_body[ans_num] == "") continue;
            
            OrderAnswer temp_answer = checkOrderAnswer(answers_body.at(ans_num),await_statuses);
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
        // answer = checkOrderAnswer(answers_body[ans_num],{FILLED}); // Если ордер выполнен то ништяк
        answers_body.erase(ans_num);
    }catch(...)
    {
        Log("EXEPTION :: binanceClient.cpp (getAnswer):",answer.text);
        LogFlush();
    }
    return answer;
}


void OKX_Client::clientMessage(const std::string& msg)
{
    try
    {
        json message = json::parse(msg);

        if(message.contains("event"))
        {
            if(message["event"] == "login")
            {
                if(message["code"] == "0")
                {
                    Log("Client successfully logged in!");
                }
            }else
            {
                Log("Unknown event: ",msg);
            }
        }
        if(message.contains("op"))
        {
            if(message["op"] == "order")
            {
                if(message["data"][0]["sCode"] == "0")
                {
                    Log("Order",message["data"][0]["clOrdId"]," successfully placed!");
                }else
                {
                    Log("Order",message["data"][0]["clOrdId"],"placement failed: ",message["data"][0]["sMsg"], msg);
                    answers_body.insert(std::atoll(message["data"][0]["clOrdId"].get<std::string>().c_str()), msg); 
                }
            }else
            {
                Log("Unknown operation: ",msg);
            }
        }
    }catch(...)
    {
        Log("EXEPTION :: OKX_Client.cpp (clientMessage):",msg);
        LogFlush();
    }
}

void OKX_Client::ordersMessage(const std::string& msg)
{
    try
    {
        json message = json::parse(msg);
        if(message.contains("event"))
        {
            if(message["event"] == "subscribe")
            {
                Log("Channel",message["arg"]["channel"],"successfully subscribed");
            }else if(message["event"] == "login")
            {
                Log("Orders successfully logged in!");
            }else
            {
                Log("Unknown event: ",msg);
            }
        }else
        { 
            if(message["arg"]["channel"] == "account")
            {
                Balances prev_bal = all_balances;
                all_balances.updateTime = getTimestamp();
                all_balances.assets["totalInUSD"] = std::atof(message["data"][0]["totalEq"].get<std::string>().c_str());
                bool is_changed = false;
                double bal;
                for(json assets : message["data"][0]["details"])
                {
                    bal = std::atof(assets["cashBal"].get<std::string>().c_str());
                    if(all_balances.assets.contains(assets["ccy"]))
                    {
                        if(all_balances.assets[assets["ccy"]] != bal) is_changed = true;
                    }else
                    {
                        is_changed = true;
                    }
                    all_balances.assets[assets["ccy"]] = bal;
                }
                if(is_changed)
                {
                    LogMore("balances changed: ", all_balances.updateTime);
                    for(auto asset : all_balances.assets)
                    {
                        LogMore(asset.first,asset.second,';');
                    }
                    LogMore('\n');  
                }

            }
            else if(message["arg"]["channel"] == "orders")
            {
                for(json order : message["data"])
                {
                    // if(order["state"] == "filled" || order["state"] == "canceled")
                    answers_body.insert(std::atoll(message["data"][0]["clOrdId"].get<std::string>().c_str()), order.dump());
                }
            }
        }
    }catch(...)
    {
        Log("EXEPTION :: OKX_Client.cpp (ordersMessage):",msg);
        LogFlush();
    }
}

double OKX_Client::getBalanceInCur(const std::string& cur)
{
    Log("getBalanceInCur",cur);
    auto temp = getBalances();
    if(!temp.has_value())
        return 0.0;
    auto bal = temp.value();
    double cur_bal = bal.assets[cur];
    // std::map<std::string, bool> markets;
    struct Mark
    {
        std::string mark;
        bool isBase;
        double amount;
    };
    std::vector<Mark> markets;

    for(auto asset : bal.assets)
        if(asset.second > 0.0 && asset.first != cur)
            for(auto info : exchange_info.marketsInfo)
            {
                if(info.second.baseAsset == cur && info.second.quoteAsset == asset.first)
                    markets.push_back({info.first,true,asset.second});
                if(info.second.baseAsset == asset.first && info.second.quoteAsset == cur)
                    markets.push_back({info.first,false,asset.second});
            }

    HttpClient cli;
    HttpRequest req;
    req.method = HTTP_GET;
    req.url = "https://www.okx.com/api/v5/market/tickers";
    req.query_params["instType"] = "SPOT";
    HttpResponse resp;
    int ret = cli.send(&req, &resp);
    if (ret == 0)
    {
        try
        {
            // std::cout << resp.body << std::endl;
            json answer = json::parse(resp.body);
            answer = answer["data"];
            for(json inst : answer)
                for(auto mark : markets)
                    if(inst["instId"] == mark.mark)
                        if(mark.isBase)
                            cur_bal += mark.amount / std::atof(inst["askPx"].get<std::string>().c_str());
                        else
                            cur_bal += mark.amount * std::atof(inst["bidPx"].get<std::string>().c_str());
        }catch(...)
        {
            Log("EXEPTION in getBalanceInCur", cur);
        }   
    }
    return cur_bal;
}

std::string OKX_Client::getSignHMAC(const char* data)
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
    return base64(result,result_len);
}

std::string OKX_Client::SIDE_(int side)
{
    if(side >= SIDE.size() && side < 0) return "";
    return SIDE.at(side);
}

char* OKX_Client::base64(const unsigned char *input, int length) 
{
    const auto pl = 4*((length+2)/3);
    auto output = reinterpret_cast<char *>(calloc(pl+1, 1)); //+1 for the terminating null that EVP_EncodeBlock adds on
    const auto ol = EVP_EncodeBlock(reinterpret_cast<unsigned char *>(output), input, length);
    if (pl != ol) { std::cerr << "Whoops, encode predicted " << pl << " but we got " << ol << "\n"; }
    return output;
}