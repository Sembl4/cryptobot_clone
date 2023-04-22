#include <cryptoClient.hpp>
#include <logger.hpp>
#include <binanceClient.hpp>
#include <OKX_Client.hpp>
#include <config.hpp>
using namespace common::config;
cryptoClient& cryptoClient::getClient(int platform)
{   
    Log("getClient",platform);
    if(platform == BINANCE)
        return binanceClient::getClient();
    
    if(platform == OKX)
        return OKX_Client::getClient();

    Log("cryptoClient <getClient> :: WRONG CLIENT PLATFORM!");
    LogFlush();
    std::abort();
}

cryptoClient& cryptoClient::getClient()
{
    if(Config::Market().trading_platform == "Binance")
        return binanceClient::getClient();

    if(Config::Market().trading_platform == "OKX")
        return OKX_Client::getClient();

    Log("cryptoClient <getClient> :: WRONG CLIENT PLATFORM! Change 'trading_platform' in config");
    LogFlush();
    std::abort();
}

int cryptoClient::getPlatform()
{
    return platform;
}

std::string cryptoClient::getWorkingAsset()
{
    if(working_asset.has_value())
    {
        return working_asset.value();
    }
    return "";
}
std::string cryptoClient::getCommissionAsset()
{
    if(commission_asset.has_value())
    {
        return commission_asset.value();
    }
    return "";
}

CryptoAPI::MarketInfo cryptoClient::getWorkingMarket()
{
    if(working_market.has_value())
    {
        return working_market.value();
    }
    return {};
}