#include <config.hpp>

#include <config_loader.hpp>

using namespace common;
using namespace config;
using namespace scl;
using std::string;

static constexpr const char* marketConfigFile = "markets.config";
static constexpr const char* orderConfigFile = "orders.config";
static constexpr const char* limitConfigFile = "limits.config";
static constexpr const char* streamConfigFile = "stream.config";

Config::Config() { Init(); }

void Config::Init() {
    InitMarket();
    InitOrder();
    InitLimit();
    InitStream();
}

void Config::InitMarket() {
    config_file config(marketConfigFile, config_file::READ);

    market_.working_asset = config.get<string>("working_asset");
    market_.no_fee_markets_file_path =
        config.get<string>("no_fee_markets_file");
    market_.trading_platform = config.get<string>("trading_platform");
    config.close();
}

#include <iostream>
void Config::InitOrder() {
    config_file config(orderConfigFile, config_file::READ);

    order_.api_key = config.get<string>("api_key");
    order_.secret_key = config.get<string>("secret_key");
    order_.pass_phrase = config.get<string>("pass_phrase");

    order_.await_time = config.get<uint64_t>("await_time");
    std::cout << "Init Order:: Await Time " << order_.await_time << ' ' << config.get<int32_t>("await_time") << std::endl;
    order_.repeats_count = config.get<int32_t>("count_repeats");
    order_.pause_between_orders = config.get<int32_t>("pause_between_orders");
    order_.orders_type = config.get<int32_t>("orders_type");
    order_.print_answers = config.get<bool>("print_answers");
    config.close();
}

void Config::InitLimit() {
    config_file config(limitConfigFile, config_file::READ);

    limit_.order_start_limit = config.get<double>("start_limit");
    limit_.fee_step = config.get<double>("fee_step");
    limit_.run_type = config.get<string>("run_type");
    config.close();
}

void Config::InitStream() {
    config_file config(streamConfigFile, config_file::READ);

    stream_.bids_index = config.get<size_t>("bids_index");
    stream_.asks_index = config.get<size_t>("asks_index");
    config.close();
}

const Config& Config::Get() {
    static Config config;
    return config;
}

const detail::MarketConfig& Config::Market() { return Get().market_; }

const detail::OrderConfig& Config::Order() { return Get().order_; }

const detail::LimitConfig& Config::Limits() { return Get().limit_; }

const detail::StreamConfig& Config::Stream() { return Get().stream_; }