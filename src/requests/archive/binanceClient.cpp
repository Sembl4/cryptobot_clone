#include <binanceClient.hpp>

#include <openssl/hmac.h>
#include <sstream>
#include <hv/json.hpp>

using namespace hv;
using namespace binanceAPI;
using namespace utils;

binanceClient::binanceClient(std::string_view api_key, std::string_view secret_key) : 
    http_client(new HttpClient())
{
	_api_key = api_key;
	_secret_key = secret_key;

    answers = 0;
    sended_reqs = 0;
}

binanceClient::~binanceClient()
{
    waitAnswers();
	http_client.reset();
}

std::string binanceClient::encryptWithHMAC(const char* data) {
	unsigned char *result;
	static char res_hexstring[64];
	int result_len = 32;
	std::string signature;

	result = HMAC(EVP_sha256(), _secret_key.c_str(), strlen((char *)_secret_key.c_str()), const_cast<unsigned char *>(reinterpret_cast<const unsigned char*>(data)), strlen((char *)data), NULL, NULL);
	for (int i = 0; i < result_len; i++) {
		sprintf(&(res_hexstring[i * 2]), "%02x", result[i]);
	}

	for (int i = 0; i < 64; i++) {
		signature += res_hexstring[i];
	}

	return signature;
}

void binanceClient::orderAnswer(const HttpResponsePtr& resp,uint64_t i)
{
	answers++;
    if (resp == NULL) {
        printf("request failed!\n");
    } else {
        this->answers_body[i] = resp->body;
    }
}

uint64_t binanceClient::getBalance(std::string_view currency)
{
	HttpRequestPtr req(new HttpRequest());
    req->method = HTTP_GET;
    
    req->url = std::string(API_URL) + std::string(ACCOUNT_V3);
	QueryParams params;
	params["timestamp"] = std::to_string(getTimestamp());
	params["recvWindow"] = "5000";
    std::string sign = encryptWithHMAC(dump_query_params(params).c_str());
	params["signature"] = sign;

    req->query_params = params;

    req->headers["X-MBX-APIKEY"] = _api_key;

    req->timeout = 5;
    sended_reqs++;
    http_client -> sendAsync(req,[this,currency](const HttpResponsePtr& resp)
    {   
        answers++;
        this->answers_body[sended_reqs] = resp->body;
        Json js = json::parse(resp->body);
        json arr = js["balances"].get<json>();
        for(const json &it : arr)
        {
            if(it["asset"].get<std::string>() == currency)
            {
                this->balance = std::stod(it["free"].get<std::string>());
            }
        }
    });
    return sended_reqs;
}

HttpRequestPtr binanceClient::createLimitOrder(const std::string_view& trading_pair,
                        const std::string_view& order, double quantity,
                        double price)
{
    HttpRequestPtr req(new HttpRequest());
    req->method = HTTP_POST;
    
    req->url = std::string(API_URL) + std::string(OPENORDER);

    hv::QueryParams params;
    params["symbol"] = trading_pair;
	params["side"] = order;
	params["type"] = "LIMIT";
	params["timeInForce"] = "FOK";
	params["quantity"] = std::to_string(quantity);
	params["price"] = std::to_string(price);
	params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "60000";
	params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    req->query_params = params;

    req->headers["X-MBX-APIKEY"] = _api_key;
    req->timeout = 10;

    return req;
}


HttpRequestPtr binanceClient::createMarketOrder(const std::string_view& trading_pair,
                        const std::string_view& order, double quantity)
{
    HttpRequestPtr req(new HttpRequest());
    req->method = HTTP_POST;
    
    req->url = std::string(API_URL) + std::string(OPENORDER);

    hv::QueryParams params;
    params["symbol"] = trading_pair;
	params["side"] = order;
	params["type"] = "MARKET";
	params["quantity"] = std::to_string(quantity);
	params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "60000";
	params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    req->query_params = params;

    req->headers["X-MBX-APIKEY"] = _api_key;
    req->timeout = 10;

    return req;
}

uint64_t binanceClient::openOrder(const HttpRequestPtr& req)
{
    sended_reqs++;
    uint64_t i = sended_reqs;
    http_client -> sendAsync(req, [this,i](const HttpResponsePtr& resp)
    {
        this->orderAnswer(resp,i);
    });
    return sended_reqs;
}

uint64_t binanceClient::openLimitOrder(const std::string_view &trading_pair, const std::string_view &order, double quantity, double price)
{
    return openOrder(createLimitOrder(trading_pair,order,quantity,price));
    /* старый код
    HttpRequestPtr req(new HttpRequest());
    req->method = HTTP_POST;
    
    req->url = std::string(API_URL) + std::string(OPENORDER);

    hv::QueryParams params;
    params["symbol"] = trading_pair;
	params["side"] = order;
	params["type"] = "LIMIT";
	params["timeInForce"] = "FOK";
	params["quantity"] = std::to_string(quantity);
	params["price"] = std::to_string(price);
	params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "60000";
	params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    req->query_params = params;

    req->headers["X-MBX-APIKEY"] = _api_key;
    req->timeout = 10;
    sended_reqs++;
    uint64_t i = sended_reqs;
    http_client -> sendAsync(req, [this,i](const HttpResponsePtr& resp)
    {
        this->orderAnswer(resp,i);
    });
    return sended_reqs;
    */
}

uint64_t binanceClient::openMarketOrder(const std::string_view &trading_pair, const std::string_view &order, double quantity)
{
    return openOrder(createMarketOrder(trading_pair,order,quantity));
    /* старый код
    HttpRequestPtr req(new HttpRequest());
    req->method = HTTP_POST;
    
    req->url = std::string(API_URL) + std::string(OPENORDER);

    hv::QueryParams params;
    params["symbol"] = trading_pair;
	params["side"] = order;
	params["type"] = "MARKET";
	params["quantity"] = std::to_string(quantity);
	params["timestamp"] = std::to_string(getTimestamp());
    params["recvWindow"] = "60000";
	params["signature"] = encryptWithHMAC(dump_query_params(params).c_str());

    req->query_params = params;

    req->headers["X-MBX-APIKEY"] = _api_key;
    req->timeout = 10;
    sended_reqs++;
    uint64_t i = sended_reqs;
    http_client -> sendAsync(req, [this,i](const HttpResponsePtr& resp)
    {
        this->orderAnswer(resp,i);
    });
    return sended_reqs;
    */
}