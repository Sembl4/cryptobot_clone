#include <cryptobot.hpp>

#include <cryptoClient.hpp>
// #include <binanceClient.hpp>
int main() {
    cryptobot::CryptoBot trading_bot;
    
    trading_bot.SetCyclesCount(1);
    trading_bot.SetCycleWorkTime(24 * 60 * 60 * 1000); // ms
    trading_bot.SetWarmUpTime(60*1000); // ms
    trading_bot.SetCoolDownTime(60*1000); // ms
    trading_bot.SetTrianglesHandlersCount(2);
    trading_bot.SetStreamReadersCount(20);
    trading_bot.Run();

    return 0;
}
    // static cryptoClient& client = cryptoClient::getClient();
    // client.Connect();

    // double price = 1;
    // double volume = 1;
    // int i = 1;
    // // common::request::Operation::BUY;
    // while(price != 0)
    // {
    //     std::cin >> i;
    //     std::cin >> price;
    //     std::cin >> volume;

    //     std::cout << client.getAnswer(client.openLimitOrder("USDTRUB",i,volume,price,CryptoAPI::IOC)).text << std::endl;
    // }

//     static cryptoClient& client = cryptoClient::getClient(BINANCE);
//     client.Connect();


//     Log(client.balances().assets["USDT"]);
//     uint64_t ans_num = client.openLimitOrder("BTCUSDT",1,0.0006,24200.29,CryptoAPI::GTC);
//     auto ans = client.getAnswer(ans_num,10000);
    
//     Log("The answer is:",ans.is_done,ans.text);
//     client.cancelOrder("BTCUSDT",std::to_string(ans_num));

//     ans_num = client.openLimitOrder("BTCUSDT",1,0.0006,24200.29,CryptoAPI::GTC);
//     ans = client.getAnswer(ans_num,10000);
    
//     Log("The answer is:",ans.is_done,ans.text);
//     client.cancelOrder("BTCUSDT",std::to_string(ans_num));

//     ans_num = client.openLimitOrder("BTCUSDT",1,0.0006,24200.29,CryptoAPI::GTC);
//     ans = client.getAnswer(ans_num,10000);
    
//     Log("The answer is:",ans.is_done,ans.text);
//     client.cancelOrder("BTCUSDT",std::to_string(ans_num));
    // std::this_thread::sleep_for(std::chrono::seconds(60));

    // client.Connect();
    // std::cout << "connect done" << std::endl;
    // auto all_info = client.exchangeInfo();
    // std::cout << "info get" << std::endl;
    // for(auto info : all_info.marketsInfo)
    // {
    //     std::cout << info.first << ' ' <<  info.second.baseAsset  << ' ' << info.second.basePrecision 
    //             << ' ' << info.second.quoteAsset << ' ' <<  info.second.quotePrecision  << std::endl;
    // }
    // double num = 0.000000000577;
    // // std::cin >> num;
    // int prec = 0;
    // while(0.000000000001 < pow(10,(-1)*prec))++prec;
    // std::cout << prec << std::endl;
    // std::cout << utils::to_str(num,prec) << std::endl;