#include <doOrdersClass.hpp>
#include <algorithms.hpp>
#include <config.hpp>
#include <logger.hpp>
#include <quote.hpp>
using namespace common::market;
using namespace common::config;
using namespace nlohmann;

static MarketManager& market_manager = MarketManager::Get();
// static cryptoClient& client = cryptoClient::getClient();

doOrders::doOrders(cryptoClient& cl) : client(cl)
{
    // Используется на всякий случай, если клиента еще нигде не включили
    client.Connect();

    sndr_type = Config::Order().orders_type;
    count_repeats = Config::Order().repeats_count;
    pause_between_orders = Config::Order().pause_between_orders;
    print_answers = Config::Order().print_answers;
    working_asset = Config::Market().working_asset;
    await_time = Config::Order().await_time;
    switch(client.getPlatform())
    {   
    case BINANCE:
        platform_fee = 0;
        break;
    case OKX:
        platform_fee = 0.001;
    break;
    }

    switch (sndr_type) {
    case SenderType::GTC:
        Log("doOrders: ", "orders_type is GTC");
        triangleImplementation = [this](){ this->doGTC_Orders();};
        break;
    case SenderType::FOK:
        Log("doOrders: ", "orders_type is FOK");
        triangleImplementation = [this](){ this->doFOK_Orders();};
        break;
    case SenderType::IOC:
        Log("doOrders: ", "orders_type is IOC");
        triangleImplementation = [this](){ this->doIOC_Orders();};
        break;
    case SenderType::GTC_part:
        Log("doOrders: ", "orders_type is GTC_part");
        triangleImplementation = [this](){ this->doGTC_part_Orders();};
        break;
    default:
        Log("doOrders: ", "bad orders_type");
        std::abort();
        break;
    }

    Log("Await time",await_time);
    first_request = true;
    state = States::WAIT_ORDERS;

    order_balance = 50;

    Log("doOrders: doOrders created");

    std::thread thrd([this]() { loop(); });
    thrd.detach();
}
doOrders::~doOrders() {
    state = BREAK;
    requests_queue.Close();
    // Развернутый отчет в конце лога
    Log("Changed balances at the end of work:");
    std::string sign = "";
    for (auto start : start_balances.assets) {
        for (auto end : client.balances().assets) {
            if (start.first == end.first && start.second != end.second) {
                sign = "";
                if (end.second > start.second) sign = "+";
                sign += std::to_string(end.second - start.second);
                LogMore(start.first, "balance was changed from ", start.second,
                        " to ", end.second, "; movement:", sign, '\n');
            }
        }
    }
}

void doOrders::addOrder(common::request::RequestData& request) {
    // if(add_new)
    // {
    //     add_new = false;
    // }
    if (requests_queue.empty()) requests_queue.Put(request);
        requests_queue.Put(request);
}

void doOrders::clearQueue() { requests_queue.Clear(); }
void doOrders::stop() { state = BREAK; }

void doOrders::loop() {
    Log("doOrders: ", "start orders loop\n");


    start_balances = client.balances();
    current_balances = start_balances;

    Log("doOrders: Start balance = ", start_balances.assets[working_asset],working_asset);
    Log("doOrders: ", ComissionAsset," balance = ", start_balances.assets[ComissionAsset], ComissionAsset);
    LogFlush();
    while (true) 
    {
        if (state == States::BREAK) {
            Log("doOrders: ", "break loop");
            break;
        }
        switch (state) {
            case States::DO_ORDERS: 
            {
                if (!requests_queue.empty()) 
                {
                    auto req_opt = requests_queue.Take();
                    if (req_opt.has_value()) 
                    {
                        current_request = req_opt.value();
                    } else 
                    {
                        Log("Optional was empty");
                        // state = States::BREAK;
                        continue;
                    }

                    if (first_request) 
                    {
                        first_request = false;
                        Log("doOrders: ", "FIRST REQUEST");
                        start_time = std::chrono::system_clock::now();
                    }

                    triangleImplementation();
                    // add_new = true;
                } else 
                {
                    Log("doOrders: ", "go to WAIT_ORDERS");
                    state = WAIT_ORDERS;
                }
            } break;
            case States::WAIT_ORDERS: 
            {
                if (!requests_queue.empty()) 
                {
                    first_request = true;
                    Log("doOrders: ", "go to DO_ORDERS");
                    state = DO_ORDERS;
                }
                if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_time).count() >= 10 &&order_balance < 50) 
                {
                    Log("doOrders: ", "count of orders was ", order_balance,
                        " updated to 50");
                    order_balance = 50;
                }
                if(client.getPlatform() == BINANCE)
                {
                    autoBuyComission();
                    pingAccount();
                }
            } break;
            case States::WAIT_BALANCE: {
                if (std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start_time).count() >= 10) 
                {
                    Log("doOrders: ", "go to DO_ORDERS");
                    first_request = true;
                    Log("doOrders: ", "count of orders was ", order_balance,
                        " updated to 50");
                    order_balance = 50;
                    state = DO_ORDERS;
                }
            } break;
            default:
                break;
        }
    }
    Log("doOrders: ", "end loop");
}

void doOrders::doIOC_Orders()
{
    Log("new triangle, IOC");
    if (order_balance < 3) {
        Log("doOrders: ", "go to WAIT_BALANCE");
        state = WAIT_BALANCE;
        return;
    }
    if (!current_request.CompareAndDecrease()) {
        Log("doOrders: Triangle not actual now!\n");
        state = WAIT_BALANCE;
    }else
    {
        std::vector<common::request::Order> orders(3);
        // orders.push_back(current_request.start);
        // orders.push_back(current_request.middle);
        // orders.push_back(current_request.fin);
        orders[0] = current_request.start;
        orders[1] = current_request.middle;
        orders[2] = current_request.fin;

        std::vector<CryptoAPI::OrderAnswer> answers(3);
        
        bool triangle_done = true;
        for(int i = 0; i < 3; ++i)
        {
            auto ans_num = client.openLimitOrder(orders[i].symbols_pair, orders[i].op_type ,orders[i].rate.volume, orders[i].rate.price, CryptoAPI::IOC);
            Log("Order", ans_num, "sended");
            answers[i] = client.getAnswerSt(ans_num,{CryptoAPI::EXPIRED,CryptoAPI::FILLED},10'000);
            Log("The answer is",answers[i].text);
            if(answers[i].base_filled_qty >= orders[i].rate.volume)
            {
                // Ордер полностью выполнен
                Log(i+1, "order fully filled");
            }else if(answers[1].base_filled_qty > 0.0)
            {
                Log(i+1, "order partically filled");
                // Ордер частично выполнен
                // записываем в ордер выполненную часть 
                orders[i].rate.volume = answers[i].base_filled_qty;
                current_request.start = orders[0];
                current_request.middle = orders[1];
                current_request.fin = orders[2];
                // Пересчитываем с новыми объемами и идем на следующую итерацию
                common::algo::Algorithm::CountRequestVolumes(current_request);
                common::algo::Algorithm::CountRequestFilters(current_request,platform_fee);
            }else
            {
                // Order is failed
                triangle_done = false;
                break;
            }
        }

        LogFlush();
        for(int i = 0; i < 3; ++i)
            LogMore(i+1,"ans(",answers[i].is_done,") : ", answers[i].text,'\n');

        LogMore("START ans - ", answers[0].is_done,"MIDDLE ans - ", answers[1].is_done,"FINAL ans - ", answers[2].is_done,'\n');

        if(!triangle_done)
        {
            Log("Triangle failed! Requests ID", current_request.ID);
            rollback::RollBack::RollBackBalance(client);
        }else
        {
            Log("Triangle done! Requests ID", current_request.ID);
        }
    }
    LogMore("Current ", working_asset, "balance : ", client.balances().assets[working_asset],'\n');

    LogFlush();


}

void doOrders::doGTC_part_Orders()
{
    Log("new triangle, GTC_part");
    if (order_balance < 3) {
        Log("doOrders: ", "go to WAIT_BALANCE");
        state = WAIT_BALANCE;
        return;
    }

    if (!current_request.CompareAndDecrease()) {
        Log("doOrders: Triangle not actual now!\n");
        state = WAIT_BALANCE;
    }else
    {
        std::vector<common::request::Order> orders(3);
        orders[0] = current_request.start;
        orders[1] = current_request.middle;
        orders[2] = current_request.fin;

        std::vector<CryptoAPI::OrderAnswer> answers(3);
        std::vector<CryptoAPI::ORDER_STATUS> await_statuses = {CryptoAPI::FILLED, CryptoAPI::PARTIALLY_FILLED};
        bool triangle_done = true;
        bool is_partially = false; 
        for(size_t i = 0; i < 3; ++i)
        {
            order_balance--;
            uint64_t ans = client.openLimitOrder(orders[i].symbols_pair, orders[i].op_type ,orders[i].rate.volume, orders[i].rate.price, CryptoAPI::GTC);
            Log("Order", ans, "sended");
            answers[i] = client.getAnswerSt(ans,await_statuses,await_time);
            Log("The answer is",answers[i].text);
            switch (answers[i].status)
            {
            case CryptoAPI::FILLED:
                // Ордер полностью выполнился, все круто, выводим сообщение, и бежим выполнять следующий
                answers[i].is_done = true;
                Log(i+1, "order fully filled");
                if(answers[i].was_exception)
                {
                    Log("It was exeption, cancel this order just in case");
                    client.cancelOrder(orders[i].symbols_pair,std::to_string(ans));
                }

                break;
            case CryptoAPI::PARTIALLY_FILLED:
            {   
                // Ордер выполнился частично, тоже нормально, займемся пересчетом
                Log(i+1, "order partially filled");
                if(answers[i].base_filled_qty > orders[i].rate.volume / 2)
                {
                    answers[i].is_done = true;
                    orders[i].rate.volume = answers[i].base_filled_qty;
                    current_request.start = orders[0];
                    current_request.middle = orders[1];
                    current_request.fin = orders[2];
                    // Пересчитываем с новыми объемами и идем на следующую итерацию
                    common::algo::Algorithm::CountRequestVolumes(current_request);
                    common::algo::Algorithm::CountRequestFilters(current_request,platform_fee);
                    // Отменяем не до конца выполненный ордер
                    is_partially = true;
                }else
                {
                    answers[i].is_done = false;
                    triangle_done = false;
                }
                client.cancelOrder(orders[i].symbols_pair,std::to_string(ans));
                break;
            }
            default:
                client.cancelOrder(orders[i].symbols_pair,std::to_string(ans));
                answers[i].is_done = false;
                triangle_done = false;
                Log(i+1, "order failed");
                break;
            }
            if(!triangle_done) break;
        }

        LogFlush();
        for(int i = 0; i < 3; ++i)
            LogMore(i+1,"ans(",answers[i].is_done,") : ", answers[i].text,'\n');
        LogMore("START ans - ", answers[0].is_done,"MIDDLE ans - ", answers[1].is_done,"FINAL ans - ", answers[2].is_done,'\n');

        if(!triangle_done){
            Log("Triangle failed! Requests ID", current_request.ID);
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            rollback::RollBack::RollBackBalance(client);
        }else
        {
            Log("Triangle done! Requests ID", current_request.ID);
            if(is_partially)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                rollback::RollBack::RollBackBalance(client);
            }
        }    
    }
    // LogMore("Current ", working_asset, "balance : ", client.balances().assets[working_asset],'\n');

    LogFlush();
    static uint64_t prev_ts = 0;
    static double prev_cur_bal = 0;
    if(utils::getTimestamp<std::chrono::seconds>() - prev_ts >= (30 * 60)) // Каждые полчаса
    {
        prev_ts = utils::getTimestamp<std::chrono::seconds>();
        double cur_bal = client.getBalanceInCur(working_asset);
        std::string s = "";
        if(cur_bal - prev_cur_bal > 0) s = "+";
        Log("Current converted to",working_asset,"balance:", cur_bal,"change:",s,cur_bal - prev_cur_bal);
        prev_cur_bal = cur_bal;
    }
    requests_queue.Clear();
}

void doOrders::doGTC_Orders() {
    Log("new triangle, GTC");
    if (order_balance < 3) {
        Log("doOrders: ", "go to WAIT_BALANCE");
        state = WAIT_BALANCE;
        return;
    }

    if (!current_request.CompareAndDecrease()) {
        Log("doOrders: Triangle not actual now!\n");
        state = WAIT_BALANCE;
    }else
    {    
        std::vector<common::request::Order> orders;
        orders.push_back(current_request.start);
        orders.push_back(current_request.middle);
        orders.push_back(current_request.fin);

        std::vector<CryptoAPI::OrderAnswer> answers;
        answers.resize(3);
        
        bool triangle_done = true;
        for(int i = 0; i < 3; ++i)
        {
            order_balance--;
            uint64_t ans = client.openLimitOrder(orders[i].symbols_pair, orders[i].op_type ,orders[i].rate.volume, orders[i].rate.price, CryptoAPI::GTC);
            answers[i] = client.getAnswer(ans,await_time);
            Log("doOrders: request ", orders[i].symbols_pair, " ",orderType(orders[i].op_type), " ", orders[i].rate.volume, " ",orders[i].rate.price, " result:",answers[i].is_done);

            if(!answers[i].is_done)
            {
                triangle_done = false;
                client.cancelOrder(orders[i].symbols_pair,std::to_string(ans));
                break;
            }
        }
        LogFlush();
        LogMore("1st ans(",answers[0].is_done,") : ", answers[0].text,'\n');
        LogMore("2nd ans(",answers[1].is_done,") : ", answers[1].text,'\n');
        LogMore("3rd ans(",answers[2].is_done,") : ", answers[2].text,'\n');

        LogMore("START ans - ", answers[0].is_done,"MIDDLE ans - ", answers[1].is_done,"FINAL ans - ", answers[2].is_done,'\n');

        if(!triangle_done)
        {
            Log("Triangle failed! Requests ID", current_request.ID);
            rollback::RollBack::RollBackBalance(client);
        }else
        {
            Log("Triangle done! Requests ID", current_request.ID);
        }
    }
    LogMore("Current ", working_asset, "balance : ", client.balances().assets[working_asset],'\n');

    LogFlush();
}

void doOrders::doFOK_Orders() {
    // std::cout << (1 + 2 * count_repeats) << std::endl;
    Log("new triangle,FOK");
    if (order_balance < (1 + 2 * count_repeats)) {
        Log("doOrders: ", "go to WAIT_BALANCE");
        state = WAIT_BALANCE;
        return;
    }
    if (!current_request.CompareAndDecrease()) {
        Log("doOrders: Triangle not actual now!\n");
        state = WAIT_BALANCE;
    }else
    {
        // Ответы для откатов
        answers::AnsReq ans;
        ans.requests = current_request;

        ans.awaits.start.push_back(client.openLimitOrder(current_request.start.symbols_pair,current_request.start.op_type,current_request.start.rate.volume, current_request.start.rate.price));
        order_balance--;
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
        for (int i = 0; i < count_repeats; ++i) {
            ans.awaits.mid.push_back(client.openLimitOrder(current_request.middle.symbols_pair,current_request.middle.op_type,current_request.middle.rate.volume,current_request.middle.rate.price));
            order_balance--;
            
            ans.awaits.fin.push_back(client.openLimitOrder(current_request.fin.symbols_pair,current_request.fin.op_type,current_request.fin.rate.volume, current_request.fin.rate.price));
            order_balance--;
            std::this_thread::sleep_for(std::chrono::milliseconds(1));
        }
        if(answers::checkAnswers(client,ans))
        {
            Log("Triangle done!");
            return; 
        }
    }

    if(requests_queue.empty())
        rollback::RollBack::RollBackBalance(client);
}

void doOrders::autoBuyComission()
{
    double comm_bal = client.balances().assets[ComissionAsset];

    working_market.info = client.getWorkingMarket();
    working_market.name = working_market.info.baseAsset + working_market.info.quoteAsset;
    // Log("doOrdersClass.cpp",working_market.name,"name");
    // LogFlush();
    // std::cerr << "autoBuyComission" << std::endl;
    working_market.id = market_manager.MarketId(working_market.name);
    working_market.isWorkingBase = (working_market.info.baseAsset == working_asset);

    common::request::Order ord;
    ord.symbols_pair = working_market.name;
    ord.market_id = working_market.id;

    if(!working_market.isWorkingBase)
    {
        // Если рабочая валюта вторая на рынке, то нам нужно совершить операцию покупки
        ord.op_type = common::request::Operation::BUY;
        // Если баланс комиссионной валюты на актуальную цену больше 1/10 минимального размера ордера, то ничего не делаем
        if(comm_bal * ord.ActualPrice() > working_market.info.minNotional / 10.0) return;
    }else
    {
        ord.op_type = common::request::Operation::SELL;
        // Если рабочая валюта первая то минимальный размер ордера измеряется в комиссионной валюте
        // Если этой валюты больше чем 1/10 минимального размера ордера, то ничего не делаем
        if(comm_bal > working_market.info.minNotional / 10.0) return;
    }
    uint64_t last_time = client.balances().updateTime;

    // Если условия не отработали, то докупаем валюты
    if(!working_market.isWorkingBase)
    {
        LogMore("Buy", ComissionAsset ,"for comission\n");
        auto ans = client.openMarketOrderQuote(ord.symbols_pair,ord.op_type,working_market.info.minNotional);
        auto body = client.getAnswer(ans);
        order_balance--;
        LogMore("Order Market Quote: ", ord.symbols_pair, "BUY" ,working_market.info.minNotional,'\n');
        LogMore("Autobuy comission answer:", body.text,'\n');
        if(body.is_done)
            LogMore("Order done.\n");
        else
            LogMore("Order not done.\n");
    }else
    {
        LogMore("Buy", ComissionAsset ,"for comission\n");
        auto ans = client.openMarketOrderQuote(ord.symbols_pair,ord.op_type,working_market.info.minNotional);
        auto body = client.getAnswer(ans);
        order_balance--;
        LogMore("Order Market Quote: ", ord.symbols_pair, "SELL" ,working_market.info.minNotional,'\n');
        LogMore("Autobuy comission answer:", body.text,'\n');
        if(body.is_done)
            LogMore("Order done.\n");
        else
            LogMore("Order not done.\n");
    }
    while(last_time == client.balances().updateTime);
    Log(ComissionAsset," balance changed from", comm_bal, "to", client.balances().assets[ComissionAsset]);
    current_balances = client.balances();
    clearQueue();
}

// Это исключительно для бинанса, его нужно пинговать каждые 30 минут
void doOrders::pingAccount()
{
    static uint64_t ping_time = utils::getTimestamp<std::chrono::milliseconds>();
    if(utils::getTimestamp<std::chrono::milliseconds>() - ping_time >= 30 * 60 * 1000)
    {
        // client.pingAccount();
        binanceClient::getClient().pingAccount();
        ping_time = utils::getTimestamp<std::chrono::milliseconds>();
    }
}