#pragma once

#include <binanceClient.hpp>
#include <blocking_queue.hpp>
#include <request_data.hpp>
#include <rollBack.hpp>

enum States { DO_ORDERS, WAIT_ORDERS, WAIT_BALANCE, BREAK };
enum SenderType { GTC, FOK, IOC, GTC_part };

constexpr const char* ComissionAsset = "BNB";

class doOrders 
{
private:
    States state;	// Текущее состояние
    int sndr_type;	// Тип отправки

    int order_balance; // Баланс ордеров

    uint64_t pause_between_orders; // Пауза между отправкой ордеров
    uint64_t await_time; // Время на ожидание выполнения ордера
    int count_repeats; // Количество повторов
    bool print_answers; // Печатать ли ответы

    double  platform_fee; // Комиссия на платформе
    
    std::function<void()> triangleImplementation;

    BlockingQueue<common::request::RequestData> requests_queue;// Очередь треугольников на реализацию
	
    common::request::RequestData current_request;// Текущий треугольник

    std::chrono::time_point<std::chrono::system_clock> start_time;// Время отправки первой реализации треугольника
	
    bool first_request;// Первый ли это треугольник

    bool add_new = true;
	
    std::string working_asset;// Рабочая валюта
    // структура для рынка комиссионной валюты и рабочей валюты
    struct
    {
		std::string name; 	// Название рынка
		size_t id;			// id рынка
		bool isWorkingBase; // Является ли рабочая валюта первой на этом рынке
		CryptoAPI::MarketInfo info; // Информация рынка
    }working_market;

    CryptoAPI::Balances start_balances;
    CryptoAPI::Balances current_balances;

    // binanceClient* client;
    cryptoClient& client;

public:
    doOrders(cryptoClient& client);

    ~doOrders();
    // Добавить ордер
    void addOrder(common::request::RequestData& requests);
    // Очистить очередь запросов
    void clearQueue();
    // Остановить конечный автомат
    void stop();

    States currentState() const { return state; }

private:
    std::string_view orderType(common::request::Operation op)
	{
        switch (op) 
		{
            case common::request::Operation::BUY:
                return std::string_view("BUY");
                break;
            case common::request::Operation::SELL:
                return std::string_view("SELL");
                break;
        }
        return std::string_view("");
    }
    void loop();			// Рабочий цикл 
    void doGTC_Orders(); 		// Метод отправки ордеров последовательно 1, 2, 3
    void doFOK_Orders(); 		// Метод отправки ордеров не дожидаясь ответов 1, 2, 3, 2, 3, 2, 3......
    void doIOC_Orders();
    void doGTC_part_Orders();

	void autoBuyComission();	// Автопокупка комиссионной валюты
    void pingAccount();
};