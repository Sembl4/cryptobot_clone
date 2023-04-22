#include <hv/json.hpp>
#include <logger.hpp>
#include <rollBack.hpp>
#include <market.hpp>

using namespace rollback;
using namespace answers;
using namespace common::request;
using namespace hv;

// static cryptoClient& client = cryptoClient::getClient();
static common::market::MarketManager& market_manager =  common::market::MarketManager::Get();

bool answers::checkAnswers(cryptoClient& client,AnsReq ans) {
    // Изначально заполняем все 3 ответа как не выполненные
    Answers answers{false, false, false};
    // Читаем все ответы на запросы первого ребра
    Log("Answers for Requests ID:",ans.requests.ID);
    for (auto i : ans.awaits.start) {
        // Копируем тело запроса
        auto body = client.getAnswer(i);
        // bool rez = checkAnswer(body);
        LogMore("1st ans ", i, ", result:", body.is_done ,":", body.text, '\n');
        // Если запрос уже считается выполненным, то можно его больше не
        // проверять
        if (!answers.start) {
            // Проверяем ответ
            answers.start = body.is_done;
        }
    }
    // Остальные проверяем тем же способом
    for (auto i : ans.awaits.mid) {
        auto body = client.getAnswer(i);
        LogMore("2st ans ", i, ", result:", body.is_done ,":", body.text, '\n');
        if (!answers.mid) {
            answers.mid = body.is_done;
        }
    }
    for (auto i : ans.awaits.fin) {
        auto body = client.getAnswer(i);
        LogMore("3rd ans ", i, ", result:", body.is_done ,":", body.text ,'\n');
        if (!answers.fin) {
            answers.fin = body.is_done;
        }
    }
    LogMore("START ans - ", answers.start, ",MIDDLE ans - ", answers.mid,",FINAL ans - ", answers.fin,'\n');
    return answers.isOk();
}

template <>
void RollBack::rollBack<BINANCE>(cryptoClient& client);
template <>
void RollBack::rollBack<OKX>(cryptoClient& client);

void RollBack::RollBackBalance(cryptoClient& client)
{
    switch(client.getPlatform())
    {
    case BINANCE:
        rollBack<BINANCE>(client);
        break;
    case OKX:
        rollBack<OKX>(client);
        break;
    default:
        Log("RollBack:: WRONG PLATFORM!");
        std::abort();
        break;
    }
}

// Для Binance
// ---------------------------------------------------------------------------------------------------------------------
template <>
void RollBack::rollBack<BINANCE>(cryptoClient& client)
{
    Log("Start RollBack");
    auto exchange_info = client.exchangeInfo();

    auto balance = client.balances();

    std::string working_asset = client.getWorkingAsset();
    std::string ComissionAsset = client.getCommissionAsset();

    struct
    {
		std::string name; 	// Название рынка
		size_t id;			// id рынка
		bool isWorkingBase; // Является ли рабочая валюта первой на этом рынке
		CryptoAPI::MarketInfo info; // Информация рынка
    }working_market;

    working_market.info = client.getWorkingMarket();
    working_market.name = working_market.info.baseAsset + working_market.info.quoteAsset;
    working_market.id = market_manager.MarketId(working_market.name);
    working_market.isWorkingBase = (working_market.info.baseAsset == working_asset);

    for(auto asset : balance.assets)
    {
        // Если валюта не равна основной валюте и комиссионной валюте
        if(asset.first != working_asset && asset.first != ComissionAsset)
        {
            if(asset.second != 0.0){
                for(auto info : exchange_info.marketsInfo)
                {
                    // Если текущая валюта является первой в рынке а рабочая валюта второй
                    if(asset.first == info.second.baseAsset && working_asset == info.second.quoteAsset)
                    {
                        common::request::Order ord;
                        ord.symbols_pair = info.first;
                        ord.market_id = market_manager.MarketId(info.first);
                        ord.op_type = common::request::Operation::SELL;
                        ord.rate.volume = ((int)(asset.second / info.second.stepSize)) * info.second.stepSize;
                        // Если объем валюты на текущий прайс рынка больше или равен минимальному размеру ордера то делаем откат
                        if(ord.rate.volume * ord.ActualPrice() >= info.second.minNotional)
                        {
                            LogMore("Order Market: ", ord.symbols_pair, "SELL" ,ord.rate.volume,'\n');
                            auto ans = client.openMarketOrder(ord.symbols_pair,ord.op_type,ord.rate.volume);
                            auto body = client.getAnswer(ans);
                            LogMore("Rollback answer:", body.text,'\n');
                            if(body.is_done)
                                LogMore("Order done.\n");
                            else
                                LogMore("Order not done.\n");
                        }
                    }
                    // Если рабочая валюта является первой в рынке а текущая второй
                    if( working_asset == info.second.baseAsset && asset.first == info.second.quoteAsset)
                    {
                        // Если объем валюты больше или равен минимальному размеру ордера то делаем откат
                        if(asset.second >= info.second.minNotional)
                        {
                            LogMore("Order Market Quote: ", info.first, "BUY" ,asset.second,'\n');
                            auto ans = client.openMarketOrderQuote(info.first,common::request::Operation::BUY,asset.second);
                            auto body = client.getAnswer(ans);
                            LogMore("Rollback answer:", body.text,'\n');
                            if(body.is_done)
                                LogMore("Order done.\n");
                            else
                                LogMore("Order not done.\n");
                        }
                    }
                }
            }
        }
        // Если валюта является комиссионной
        if(asset.first == ComissionAsset)
        {
            // LogMore("is BNB, ");
            common::request::Order ord;
            ord.symbols_pair = working_market.name;
            ord.market_id = working_market.id;
            // Основная разница в том, какой является рабочая валюта на этом рынке
            // Если рабочая валюта вторая а это в большинстве случаев (BNB/BTC, BNB/USDT)
            // то выполняем следующие действия
            if(!working_market.isWorkingBase)
            {
                // Для отката, нам потребуется операция продажи
                ord.op_type = common::request::Operation::SELL; // Это для просмотра котировки
                // Вычисляем минимальный объем который мы можем продать по текущей цене
                double min_order_volume = working_market.info.minNotional / ord.ActualPrice();
                // Вычитаем из текущего объема минимальный объем
                ord.rate.volume = asset.second - min_order_volume;
                ord.rate.volume = ((int)(ord.rate.volume / working_market.info.stepSize)) * working_market.info.stepSize;
                // Эсли после этого объем остался больше минимального, то оставляем минимальный объем на счету, а остальное продаем
                if(ord.rate.volume > min_order_volume)
                {
                    LogMore("Order Market: ", ord.symbols_pair, "SELL" ,ord.rate.volume,'\n');
                    auto ans = client.openMarketOrder(ord.symbols_pair,ord.op_type,ord.rate.volume);
                    auto body = client.getAnswer(ans);
                    LogMore("Rollback answer:", body.text,'\n');
                    if(body.is_done)
                        LogMore("Order done.\n");
                    else
                        LogMore("Order not done.\n");
                }
            }else
            {
                // Если рабочая валюта идет первой то нам даже не надо смотреть котировки,
                // Потому что комиссионная валюта уже будет валютой минимального размера ордера
                // Вычитаем из текущего объема на балансе объем минимального ордера
                ord.rate.volume = asset.second - working_market.info.minNotional;
                // Если после этого объем остался больше чем минимальный объем ордера, 
                // то откатываем комиссионный объем к рабочей валюте
                if(ord.rate.volume > working_market.info.minNotional)
                {
                    LogMore("Order Market Quote: ", ord.symbols_pair, "BUY" ,ord.rate.volume,'\n');
                    auto ans = client.openMarketOrderQuote(ord.symbols_pair,common::request::Operation::BUY,ord.rate.volume);
                    auto body = client.getAnswer(ans);
                    LogMore("Rollback answer:", body.text,'\n');
                    if(body.is_done)
                        LogMore("Order done.\n");
                    else
                        LogMore("Order not done.\n");
                }
            }
        }
    }

    Log("Total changes in this rollback:");
    for(auto cur : client.balances().assets)
    {
        for(auto asset : balance.assets)
        {
            if(cur.first == asset.first && cur.second != asset.second)
            {
                std::string sign = "";
                if (cur.second > asset.second) sign = "+";
                sign += std::to_string(cur.second - asset.second);
                LogMore(cur.first, "balance changed from ",asset.second, "to", cur.second, ", movement: ",sign,'\n');
            }
        }
    }
    Log("Rollback end.\n");
}  
// ---------------------------------------------------------------------------------------------------------------------
// Для OKX
// ---------------------------------------------------------------------------------------------------------------------
template <>
void RollBack::rollBack<OKX>(cryptoClient& client)
{
    Log("Start RollBack");
    auto exchange_info = client.exchangeInfo();
    auto balance = client.balances();

    std::string working_asset = client.getWorkingAsset();

    for(auto asset : balance.assets)
    {
        if(asset.second != 0.0)
        {
            for(auto info : exchange_info.marketsInfo)
            {
                // Если текущая валюта является первой в рынке а рабочая валюта второй
                if(asset.first == info.second.baseAsset && working_asset == info.second.quoteAsset)
                {
                    common::request::Order ord;
                    ord.symbols_pair = info.first;
                    ord.market_id = market_manager.MarketId(info.first);
                    ord.op_type = common::request::Operation::SELL;
                    ord.rate.volume = ((int)(asset.second / info.second.stepSize)) * info.second.stepSize;
                    // Если объем валюты на текущий прайс рынка больше или равен минимальному размеру ордера то делаем откат
                    if(ord.rate.volume >= info.second.minNotional)
                    {
                        auto ans = client.openMarketOrder(ord.symbols_pair,ord.op_type,ord.rate.volume);
                        auto body = client.getAnswer(ans);
                        LogMore("Order Market: ", ord.symbols_pair, "SELL" ,ord.rate.volume,'\n');
                        LogMore("Rollback answer:", body.text,'\n');
                        if(body.is_done)
                            LogMore("Order done.\n");
                        else
                            LogMore("Order not done.\n");
                    }
                }
                // Если рабочая валюта является первой в рынке а текущая второй
                if(working_asset == info.second.baseAsset && asset.first == info.second.quoteAsset)
                {
                    // Если объем валюты больше или равен минимальному размеру ордера то делаем откат
                    if(asset.second >= info.second.minNotional)
                    {
                        auto ans = client.openMarketOrderQuote(info.first,common::request::Operation::BUY,asset.second);
                        auto body = client.getAnswer(ans);
                        LogMore("Order Market Quote: ", info.first, "BUY" ,asset.second,'\n');
                        LogMore("Rollback answer:", body.text,'\n');
                        if(body.is_done)
                            LogMore("Order done.\n");
                        else
                            LogMore("Order not done.\n");
                    }
                }
            }
        }
    }

    Log("Total changes in this rollback:");
    for(auto cur : client.balances().assets)
    {
        for(auto asset : balance.assets)
        {
            if(cur.first == asset.first && cur.second != asset.second)
            {
                std::string sign = "";
                if (cur.second > asset.second) sign = "+";
                sign += std::to_string(cur.second - asset.second);
                LogMore(cur.first, "balance changed from ",asset.second, "to", cur.second, ", movement: ",sign,'\n');
            }
        }
    }
    Log("Rollback end.\n");
}
// ---------------------------------------------------------------------------------------------------------------------