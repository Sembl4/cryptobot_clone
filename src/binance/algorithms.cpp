#include <algorithms.hpp>
#include <functional>
#include <logger.hpp>

using namespace common;
using namespace common::request;
using namespace algo;
using namespace detail;

static uint64_t count_id = 0;

/*
 * 1 BID BID BID --- SELL SELL SELL
 * 2 ASK BID BID --- BUY  SELL SELL
 * 3 BID ASK BID --- SELL BUY  SELL
 * 4 ASK ASK BID --- BUY  BUY  SELL
 * 5 BID BID ASK --- SELL SELL BUY
 * 6 ASK BID ASK --- BUY  SELL BUY
 * 7 BID ASK ASK --- SELL BUY  BUYl
 * 8 ASK ASK ASK --- BUY  BUY  BUY
 * */

template <>
/// 1 BTC/BUSD SELL BUSD/DAI SELL DAI/BTC SELL
void Algorithm::CountVolumes<SELL, SELL, SELL>(RequestData& req,
                                               Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In DAI
    volume.middle = 0.0;               /// In BUSD
    volume.start = 0.0;                /// In BTC

    if (req.middle.rate.volume < (volume.fin / req.middle.rate.price)) {
        volume.middle = req.middle.rate.volume;
        volume.fin = volume.middle * req.middle.rate.price;
    } else {
        volume.middle = volume.fin / req.middle.rate.price;
    }

    if (req.start.rate.volume < (volume.middle / req.start.rate.price)) {
        volume.start = req.start.rate.volume;
        volume.middle = volume.start * req.start.rate.price;
        volume.fin = volume.middle * req.middle.rate.price;
    } else {
        volume.start = volume.middle / req.start.rate.price;
    }
}

template <>
/// 2 FIS/USDT BUY FIS/BTC SELL BTC/USDT SELL
void Algorithm::CountVolumes<BUY, SELL, SELL>(RequestData& req,
                                              Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In BTC
    volume.middle = 0.0;               /// In FIS
    volume.start = 0.0;                /// In FIS

    if (req.middle.rate.volume <
        (volume.fin /
         req.middle.rate.price)) {  /// 2_FIS_V < (3_BTC_V / 2_FIS_BTC_P)
        volume.middle = req.middle.rate.volume;  /// V_3_BTC >
                                                 /// V_2_RESULT_BTC
        volume.fin = volume.middle * req.middle.rate.price;
    } else {
        volume.middle = volume.fin / req.middle.rate.volume;
    }

    if (volume.middle > req.start.rate.volume) {  /// 2_FIS_V > 1_FIS_V
        volume.middle = volume.start = req.start.rate.volume;
        volume.fin = volume.middle * req.middle.rate.price;
    } else {
        volume.start = volume.middle;
    }
}

template <>
/// 3 BTC/BIDR SELL ZIL/BIDR BUY ZIL/BTC SELL
void Algorithm::CountVolumes<SELL, BUY, SELL>(RequestData& req,
                                              Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In ZIL
    volume.middle = 0.0;               /// In ZIL
    volume.start = 0.0;                /// In BTC

    if (req.middle.rate.volume < volume.fin) {
        volume.fin = volume.middle = req.middle.rate.volume;
    } else {
        volume.middle = volume.fin;
    }

    if (req.start.rate.volume <
        ((volume.middle * req.middle.rate.price) / req.start.rate.price)) {
        volume.start = req.start.rate.volume;
        volume.middle =
            (volume.start * req.start.rate.price) / req.middle.rate.price;
        volume.fin = volume.middle;
    } else {
        volume.start =
            (volume.middle * req.middle.rate.price) / req.start.rate.price;
    }
}

template <>
/// 4 BTC/USDT BUY XZC/BTC BUY XZC/USDT SELL
void Algorithm::CountVolumes<BUY, BUY, SELL>(request::RequestData& req,
                                             Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In XZC
    volume.middle = 0.0;               /// In XZC
    volume.start = 0.0;                /// In BTC

    if (req.middle.rate.volume < volume.fin) {  /// 2_XZC_V < 3_XZC_V
        volume.fin = volume.middle = req.middle.rate.volume;
    } else {
        volume.middle = volume.fin;
    }

    if (req.start.rate.volume < (volume.middle * req.middle.rate.price)) {
        volume.start = req.start.rate.volume;
        volume.fin = volume.middle = (volume.start / req.middle.rate.price);
    } else {
        // Была ошибка: volume.start =
        // volume.middle *
        // req.middle.rate.volume;
        volume.start = volume.middle * req.middle.rate.price;
    }
}

template <>
/// 5 BTC/USDT SELL USDT/RUB SELL BTC/RUB BUY
void Algorithm::CountVolumes<SELL, SELL, BUY>(request::RequestData& req,
                                              Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In BTC
    volume.middle = 0.0;               /// In USDT
    volume.start = 0.0;                /// In BTC

    if (req.middle.rate.volume <
        ((volume.fin * req.fin.rate.price) / req.middle.rate.price)) {
        volume.middle = req.middle.rate.volume;
        volume.fin =
            (volume.middle * req.middle.rate.price) / req.fin.rate.price;
    } else {
        volume.middle =
            (volume.fin * req.fin.rate.price) / req.middle.rate.price;
    }

    if ((volume.middle / req.start.rate.price) < req.start.rate.volume) {
        volume.start = volume.middle / req.start.rate.price;
    } else {
        volume.start = req.start.rate.volume;
        volume.middle = volume.start * req.start.rate.price;
        volume.fin =
            (volume.middle * req.middle.rate.price) / req.fin.rate.price;
    }
}

template <>
/// 6 REEF/USDT BUY REEF/TRY SELL USDT/TRY BUY
void Algorithm::CountVolumes<BUY, SELL, BUY>(request::RequestData& req,
                                             Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In USDT
    volume.middle = 0.0;               /// In REEF
    volume.start = 0.0;                /// In REEF

    if (((volume.fin * req.fin.rate.price) / req.middle.rate.price) <
        req.middle.rate.volume) {
        volume.middle =
            (volume.fin * req.fin.rate.price) / req.middle.rate.price;
    } else {
        volume.middle = req.middle.rate.volume;
        volume.fin =
            (volume.middle * req.middle.rate.price) / req.fin.rate.price;
    }

    if (volume.middle < req.start.rate.volume) {
        volume.start = volume.middle;
    } else {
        volume.start = req.start.rate.volume;
        volume.middle = volume.start;
        volume.fin =
            (volume.middle * req.middle.rate.price) / req.fin.rate.price;
    }
}

template <>
/// 7 BTC/TUSD SELL USDS/TUSD BUY BTC/USDS BUY
void Algorithm::CountVolumes<SELL, BUY, BUY>(request::RequestData& req,
                                             Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In BTC
    volume.middle = 0.0;               /// In USDS
    volume.start = 0.0;                /// In BTC

    if (req.middle.rate.volume < (volume.fin * req.fin.rate.price)) {
        volume.middle = req.middle.rate.volume;
        volume.fin = volume.middle / req.fin.rate.price;
    } else {
        volume.middle = volume.fin * req.fin.rate.price;
    }

    if (req.start.rate.volume <
        ((volume.middle * req.middle.rate.price) / req.start.rate.price)) {
        volume.start = req.start.rate.volume;
        volume.middle =
            (volume.start * req.start.rate.price) / req.middle.rate.price;
        volume.fin = volume.middle / req.fin.rate.price;
    } else {
        volume.start =
            (volume.middle * req.middle.rate.price) / req.start.rate.price;
    }
}

template <>
/// 8 ETH/BTC BUY PAX/ETH BUY BTC/PAX BUY
void Algorithm::CountVolumes<BUY, BUY, BUY>(request::RequestData& req,
                                            Volume& volume) {
    volume.fin = req.fin.rate.volume;  /// In BTC
    volume.middle = 0.0;               /// In PAX
    volume.start = 0.0;                /// In ETH

    if ((volume.fin * req.fin.rate.price) < req.middle.rate.volume) {
        volume.middle = volume.fin * req.fin.rate.price;
    } else {
        volume.middle = req.middle.rate.volume;
        volume.fin = volume.middle / req.fin.rate.price;
    }

    if ((volume.middle * req.middle.rate.price) < req.start.rate.volume) {
        volume.start = volume.middle * req.middle.rate.price;
    } else {
        volume.start = req.start.rate.volume;
        volume.middle = volume.start / req.middle.rate.price;
        volume.fin = volume.middle / req.fin.rate.price;
    }
}

void Algorithm::CountRequestVolumes(request::RequestData& req) {
    Volume volumes;

    if (req.start.op_type == SELL && req.middle.op_type == SELL &&
        req.fin.op_type == SELL) {
        CountVolumes<SELL, SELL, SELL>(req, volumes);
    } else if (req.start.op_type == BUY && req.middle.op_type == SELL &&
               req.fin.op_type == SELL) {
        CountVolumes<BUY, SELL, SELL>(req, volumes);
    } else if (req.start.op_type == SELL && req.middle.op_type == BUY &&
               req.fin.op_type == SELL) {
        CountVolumes<SELL, BUY, SELL>(req, volumes);
    } else if (req.start.op_type == BUY && req.middle.op_type == BUY &&
               req.fin.op_type == SELL) {
        CountVolumes<BUY, BUY, SELL>(req, volumes);
    } else if (req.start.op_type == SELL && req.middle.op_type == SELL &&
               req.fin.op_type == BUY) {
        CountVolumes<SELL, SELL, BUY>(req, volumes);
    } else if (req.start.op_type == BUY && req.middle.op_type == SELL &&
               req.fin.op_type == BUY) {
        CountVolumes<BUY, SELL, BUY>(req, volumes);
    } else if (req.start.op_type == SELL && req.middle.op_type == BUY &&
               req.fin.op_type == BUY) {
        CountVolumes<SELL, BUY, BUY>(req, volumes);
    } else if (req.start.op_type == BUY && req.middle.op_type == BUY &&
               req.fin.op_type == BUY) {
        CountVolumes<BUY, BUY, BUY>(req, volumes);
    }

    req.start.rate.volume = volumes.start;
    req.middle.rate.volume = volumes.middle;
    req.fin.rate.volume = volumes.fin;
}

/// Filters counting algorithms

template <>
/// 1 BTC/BUSD SELL BUSD/DAI SELL DAI/BTC SELL
void Algorithm::CountFilters<SELL, SELL, SELL>(RequestData& req, double fee) {
    req.start.rate.volume =
        ((int)(req.start.rate.volume / req.start.filters.stepSize)) *
        req.start.filters.stepSize;

    req.middle.rate.volume = req.start.rate.volume * req.start.rate.price * (1.0 - fee);
    req.middle.rate.volume =
        ((int)(req.middle.rate.volume / req.middle.filters.stepSize)) *
        req.middle.filters.stepSize;

    req.fin.rate.volume = req.middle.rate.volume * req.middle.rate.price * (1.0 - fee);
    req.fin.rate.volume = ((int)(req.fin.rate.volume / req.fin.filters.stepSize)) *
                          req.fin.filters.stepSize;
}

template <>
/// 2 FIS/USDT BUY FIS/BTC SELL BTC/USDT SELL
void Algorithm::CountFilters<BUY, SELL, SELL>(RequestData& req, double fee) {
    req.start.rate.volume =
        (int)(req.start.rate.volume / req.start.filters.stepSize) *
        req.start.filters.stepSize;

    req.middle.rate.volume = req.start.rate.volume * (1.0 - fee);
    req.middle.rate.volume =
        (int)(req.middle.rate.volume / req.middle.filters.stepSize) *
        req.middle.filters.stepSize;

    req.fin.rate.volume = req.middle.rate.volume * req.middle.rate.price * (1.0 - fee);
    req.fin.rate.volume = (int)(req.fin.rate.volume / req.fin.filters.stepSize) *
                          req.fin.filters.stepSize;
}

template <>
/// 3 BTC/BIDR SELL ZIL/BIDR BUY ZIL/BTC SELL
void Algorithm::CountFilters<SELL, BUY, SELL>(RequestData& req, double fee) {
    req.start.rate.volume =
        (int)(req.start.rate.volume / req.start.filters.stepSize) *
        req.start.filters.stepSize;

    req.middle.rate.volume =
        ((req.start.rate.volume * req.start.rate.price * (1.0 - fee)) / req.middle.rate.price);
    req.middle.rate.volume =
        (int)(req.middle.rate.volume / req.middle.filters.stepSize) *
        req.middle.filters.stepSize;

    req.fin.rate.volume = req.middle.rate.volume * (1.0 - fee);
    req.fin.rate.volume = (int)(req.fin.rate.volume / req.fin.filters.stepSize) *
                          req.fin.filters.stepSize;
}

template <>
/// 4 BTC/USDT BUY XZC/BTC BUY XZC/USDT SELL
void Algorithm::CountFilters<BUY, BUY, SELL>(RequestData& req, double fee) {
    req.start.rate.volume =
        (int)(req.start.rate.volume / req.start.filters.stepSize) *
        req.start.filters.stepSize;

    req.middle.rate.volume = (req.start.rate.volume * (1.0 - fee)) / req.middle.rate.price;
    req.middle.rate.volume =
        (int)(req.middle.rate.volume / req.middle.filters.stepSize) *
        req.middle.filters.stepSize;

    req.fin.rate.volume = req.middle.rate.volume* (1.0 - fee);
    req.fin.rate.volume = (int)(req.fin.rate.volume / req.fin.filters.stepSize) *
                          req.fin.filters.stepSize;
}

template <>
/// 5 BTC/USDT SELL USDT/RUB SELL BTC/RUB BUY
void Algorithm::CountFilters<SELL, SELL, BUY>(RequestData& req, double fee) {
    req.start.rate.volume =
        (int)(req.start.rate.volume / req.start.filters.stepSize) *
        req.start.filters.stepSize;

    req.middle.rate.volume = req.start.rate.volume * req.start.rate.price * (1.0 - fee);
    req.middle.rate.volume =
        (int)(req.middle.rate.volume / req.middle.filters.stepSize) *
        req.middle.filters.stepSize;

    req.fin.rate.volume =
        (req.middle.rate.volume * req.middle.rate.price * (1.0 - fee)) / req.fin.rate.price;
    req.fin.rate.volume = (int)(req.fin.rate.volume / req.fin.filters.stepSize) *
                          req.fin.filters.stepSize;
}

template <>
/// 6 REEF/USDT BUY REEF/TRY SELL USDT/TRY BUY *!!!!!!
void Algorithm::CountFilters<BUY, SELL, BUY>(RequestData& req, double fee) {
    req.start.rate.volume =
        (int)(req.start.rate.volume / req.start.filters.stepSize) *
        req.start.filters.stepSize;

    req.middle.rate.volume = req.start.rate.volume * (1.0 - fee);
    req.middle.rate.volume =
        (int)(req.middle.rate.volume / req.middle.filters.stepSize) *
        req.middle.filters.stepSize;

    req.fin.rate.volume =
        (req.middle.rate.volume * req.middle.rate.price * (1.0 - fee)) / req.fin.rate.price;
    req.fin.rate.volume = (int)(req.fin.rate.volume / req.fin.filters.stepSize) *
                          req.fin.filters.stepSize;
}

template <>
/// 7 BTC/TUSD SELL USDS/TUSD BUY BTC/USDS BUY
void Algorithm::CountFilters<SELL, BUY, BUY>(RequestData& req, double fee) {
    req.start.rate.volume =
        (int)(req.start.rate.volume / req.start.filters.stepSize) *
        req.start.filters.stepSize;

    req.middle.rate.volume =
        (req.start.rate.volume * req.start.rate.price * (1.0 - fee)) / req.middle.rate.price;
    req.middle.rate.volume =
        (int)(req.middle.rate.volume / req.middle.filters.stepSize) *
        req.middle.filters.stepSize;

    req.fin.rate.volume = (req.middle.rate.volume * (1.0 - fee)) / req.fin.rate.price;
    req.fin.rate.volume = (int)(req.fin.rate.volume / req.fin.filters.stepSize) *
                          req.fin.filters.stepSize;
}

template <>
/// 8 ETH/BTC BUY PAX/ETH BUY BTC/PAX BUY
void Algorithm::CountFilters<BUY, BUY, BUY>(RequestData& req, double fee) {

    req.start.rate.volume =
        (int)(req.start.rate.volume / req.start.filters.stepSize) *
        req.start.filters.stepSize;

    req.middle.rate.volume = (req.start.rate.volume * (1.0 - fee)) / req.middle.rate.price;
    req.middle.rate.volume =
        (int)(req.middle.rate.volume / req.middle.filters.stepSize) *
        req.middle.filters.stepSize;

    req.fin.rate.volume = (req.middle.rate.volume * (1.0 - fee)) / req.fin.rate.price;
    req.fin.rate.volume = (int)(req.fin.rate.volume / req.fin.filters.stepSize) *
                          req.fin.filters.stepSize;
}

void Algorithm::CountRequestFilters(request::RequestData& req, double fee)
{
    if (req.start.op_type == SELL && req.middle.op_type == SELL &&
        req.fin.op_type == SELL) {
        CountFilters<SELL, SELL, SELL>(req,fee);
    } else if (req.start.op_type == BUY && req.middle.op_type == SELL &&
               req.fin.op_type == SELL) {
        CountFilters<BUY, SELL, SELL>(req,fee);
    } else if (req.start.op_type == SELL && req.middle.op_type == BUY &&
               req.fin.op_type == SELL) {
        CountFilters<SELL, BUY, SELL>(req,fee);
    } else if (req.start.op_type == BUY && req.middle.op_type == BUY &&
               req.fin.op_type == SELL) {
        CountFilters<BUY, BUY, SELL>(req,fee);
    } else if (req.start.op_type == SELL && req.middle.op_type == SELL &&
               req.fin.op_type == BUY) {
        CountFilters<SELL, SELL, BUY>(req,fee);
    } else if (req.start.op_type == BUY && req.middle.op_type == SELL &&
               req.fin.op_type == BUY) {
        CountFilters<BUY, SELL, BUY>(req,fee);
    } else if (req.start.op_type == SELL && req.middle.op_type == BUY &&
               req.fin.op_type == BUY) {
        CountFilters<SELL, BUY, BUY>(req,fee);
    } else if (req.start.op_type == BUY && req.middle.op_type == BUY &&
               req.fin.op_type == BUY) {
        CountFilters<BUY, BUY, BUY>(req,fee);
    }
}

template<>
bool Algorithm::CountBenefitial<BINANCE>(request::RequestData& request, double summary_fee, 
                                double startLimit, std::string_view working_asset, size_t index)
{
    algo::Algorithm::CountRequestVolumes(request);

    double start_income, invest, result, new_benefit = 0, temp_benefit,temp_step_size;

    // Ограничиваем треугольник по балансу
    if (request.start.op_type == common::request::Operation::SELL) 
    {
        start_income = request.start.rate.volume;
        if (start_income > startLimit) request.start.rate.volume = startLimit;
    } else 
    {
        start_income = request.start.rate.volume * request.start.rate.price;
        if (start_income > startLimit)
            request.start.rate.volume = startLimit / request.start.rate.price;
    }

    temp_step_size = request.start.filters.stepSize;
    if (request.start.op_type == common::request::Operation::BUY)
        temp_step_size = (int)(((request.start.filters.minNotional / 10.0) / request.start.rate.price) / 
                                            request.start.filters.stepSize) * request.start.filters.stepSize;

    request::RequestData temp_req = request;
    bool good_benefit = false;
    double real_invest;
    temp_req.start.rate.volume += temp_step_size;
    for (int i = 0; i < 25; ++i) {
        temp_req.start.rate.volume -= temp_step_size;
        Algorithm::CountRequestFilters(temp_req,0.0);

        invest = temp_req.start.rate.volume;
        if (temp_req.start.op_type == common::request::Operation::BUY)
            invest = temp_req.start.rate.volume * temp_req.start.rate.price;
        result = temp_req.fin.rate.volume;
        if (temp_req.fin.op_type == common::request::Operation::SELL)
            result = temp_req.fin.rate.volume * temp_req.fin.rate.price;
        temp_benefit = result / (invest * (summary_fee));
        
        if (temp_benefit > new_benefit) {
            if (temp_req.start.rate.volume * temp_req.start.rate.price >= temp_req.start.filters.minNotional &&
                temp_req.middle.rate.volume * temp_req.middle.rate.price >= temp_req.middle.filters.minNotional &&
                temp_req.fin.rate.volume * temp_req.fin.rate.price >= temp_req.fin.filters.minNotional) 
            {
                good_benefit = true;
                real_invest = invest;
                new_benefit = temp_benefit;
                request = temp_req;
            } else {
                break;
            }
        }
    }
    if(!good_benefit) return false;
    // if(new_benefit - summary_fee < 0.0005) return false;

    if (new_benefit > summary_fee) {
        request.ID = count_id;
        count_id++;
        Log("Triangle: ", index);
        request.PrintMarketsOps();
        LogMore("Possible invest:", start_income, working_asset,'\n');
        LogMore("Invest:", real_invest ,working_asset,'\n');
        LogMore("Benefit: ", new_benefit, '\n');
        LogMore("Real benefit: ",new_benefit - summary_fee,'\n');
        LogMore("Is benefitial after filtering.\n");
        return true;
    } 
    return false;    
}

template<>
bool Algorithm::CountBenefitial<OKX>(request::RequestData& request, double summary_fee, 
                            double startLimit, std::string_view working_asset,size_t index)
{
    algo::Algorithm::CountRequestVolumes(request);

    double start_income, invest, result, new_benefit = 0, temp_benefit;

    // Ограничиваем треугольник по балансу
    if (request.start.op_type == common::request::Operation::SELL) 
    {
        start_income = request.start.rate.volume;
        if (start_income > startLimit) request.start.rate.volume = startLimit;
    } else 
    {
        start_income = request.start.rate.volume * request.start.rate.price;
        if (start_income > startLimit)
            request.start.rate.volume = startLimit / request.start.rate.price;
    }

    request::RequestData temp_req = request;
    bool good_benefit = false;
    double real_invest;
    temp_req.start.rate.volume += temp_req.start.filters.stepSize;
    for (int i = 0; i < 25; ++i) {
        temp_req.start.rate.volume -= temp_req.start.filters.stepSize;
        Algorithm::CountRequestFilters(temp_req, 0.001);

        invest = temp_req.start.rate.volume;
        if (temp_req.start.op_type == common::request::Operation::BUY)
            invest = temp_req.start.rate.volume * temp_req.start.rate.price;
        result = temp_req.fin.rate.volume;
        if (temp_req.fin.op_type == common::request::Operation::SELL)
            result = temp_req.fin.rate.volume * temp_req.fin.rate.price;
        temp_benefit = result / (invest * (summary_fee));
        
        if (temp_benefit > new_benefit) {
            if (temp_req.start.rate.volume >= temp_req.start.filters.minNotional &&
                temp_req.middle.rate.volume >= temp_req.middle.filters.minNotional &&
                temp_req.fin.rate.volume >= temp_req.fin.filters.minNotional) 
            {
                good_benefit = true;
                real_invest = invest;
                new_benefit = temp_benefit;
                request = temp_req;
            } else {
                break;
            }
        }
    }
    if(!good_benefit) return false;

    if (new_benefit > summary_fee) {
        request.ID = count_id;
        count_id++;
        Log("Triangle: ", index);
        request.PrintMarketsOps();
        LogMore("Invest:", real_invest ,working_asset,'\n');
        LogMore("Benefit: ", new_benefit, '\n');
        LogMore("Real benefit: ",new_benefit - summary_fee,'\n');
        LogMore("Is benefitial after filtering.\n");
        return true;
    } 
    return false; 
}


// template<>
// Benefitial Algorithm::CountRequestFilters<BINANCE>(request::RequestData& req, const Filters& filters, double summary_fee) {
//     Benefitial rez;
//     rez.inf.new_benefit = 0; 
//     double invest, result, temp_benefit,temp_step_size;
//     temp_step_size = req.start.filters.stepSize;
//     if (req.start.op_type == common::request::Operation::BUY)
//         temp_step_size = (int)(((req.start.filters.minNotional / 10.0) / req.start.rate.price) / req.start.filters.stepSize) * req.start.filters.stepSize;

//     request::RequestData temp_req = req;
//     bool good_benefit = false;
//     double real_invest;
//     temp_req.start.rate.volume += temp_step_size;
//     for (int i = 0; i < 25; ++i) {
//         temp_req.start.rate.volume -= temp_step_size;

//         Algorithm::CountAllFilters(temp_req, filters, 0);

//         invest = temp_req.start.rate.volume;
//         if (temp_req.start.op_type == common::request::Operation::BUY)
//             invest = temp_req.start.rate.volume * temp_req.start.rate.price;
//         result = temp_req.fin.rate.volume;
//         if (temp_req.fin.op_type == common::request::Operation::SELL)
//             result = temp_req.fin.rate.volume * temp_req.fin.rate.price;
//         temp_benefit = result / (invest * (summary_fee));
        
//         if (temp_benefit > rez.inf.new_benefit) {
//             if (temp_req.start.rate.volume * temp_req.start.rate.price >= req.start.filters.minNotional &&
//                 temp_req.middle.rate.volume * temp_req.middle.rate.price >= req.middle.filters.minNotional &&
//                 temp_req.fin.rate.volume * temp_req.fin.rate.price >= req.fin.filters.minNotional) 
//             {
//                 good_benefit = true;
//                 real_invest = invest;
//                 rez.inf.new_benefit = temp_benefit;
//                 req = temp_req;
//             } else {
//                 break;
//             }
//         }
//     }

//     return rez;
// }

// template<>
// Benefitial Algorithm::CountRequestFilters<OKX>(request::RequestData& req, const Filters& filters, double summary_fee) {

//     Benefitial rez;
//     rez.inf.new_benefit = 0;
//     double invest, result, temp_benefit,temp_step_size;
//     temp_step_size = req.start.filters.stepSize;
//     if (req.start.op_type == common::request::Operation::BUY)
//         temp_step_size = (int)((req.start.filters.minNotional / 10.0) / req.start.filters.stepSize) * req.start.filters.stepSize;

//     request::RequestData temp_req = req;
//     rez.is_good_benefit = false;
//     // double real_invest;
//     temp_req.start.rate.volume += temp_step_size;
//     for (int i = 0; i < 25; ++i) {
//         temp_req.start.rate.volume -= temp_step_size;
//         CountAllFilters(temp_req, filters, 0.001);

//         invest = temp_req.start.rate.volume;
//         if (temp_req.start.op_type == common::request::Operation::BUY)
//             invest = temp_req.start.rate.volume * temp_req.start.rate.price;
//         result = temp_req.fin.rate.volume;
//         if (temp_req.fin.op_type == common::request::Operation::SELL)
//             result = temp_req.fin.rate.volume * temp_req.fin.rate.price;
//         temp_benefit = result / invest;
        
//         if (temp_benefit > rez.inf.new_benefit) {
//             if (temp_req.start.rate.volume >= req.start.filters.minNotional &&
//                 temp_req.middle.rate.volume >= req.middle.filters.minNotional &&
//                 temp_req.fin.rate.volume >= req.fin.filters.minNotional) 
//             {
//                 rez.is_good_benefit = true;
//                 rez.inf.real_invest = invest;
//                 rez.inf.new_benefit = temp_benefit;
//                 req = temp_req;
//             } else {
//                 break;
//             }
//         }
//     }

//     return rez;
// }

// Benefitial Algorithm::CountRequestPlatform(request::RequestData& req, const Filters& filters, double summary_fee, CRYPTO_PLATFORMS platform)
// {
//     switch(platform)
//     {
//     case OKX:
//         return CountRequestFilters<OKX>(req,filters,summary_fee);
//         break;
//     case BINANCE:
//         return CountRequestFilters<BINANCE>(req,filters,summary_fee);
//         break;
//     }
//     return {};
// }
/*
 * 1 BID BID BID --- SELL, SELL, SELL
 * 2 ASK BID BID --- BUY , SELL, SELL
 * 3 BID ASK BID --- SELL, BUY , SELL
 * 4 ASK ASK BID --- BUY , BUY , SELL
 * 5 BID BID ASK --- SELL, SELL, BUY
 * 6 ASK BID ASK --- BUY , SELL, BUY
 * 7 BID ASK ASK --- SELL, BUY , BUY
 * 8 ASK ASK ASK --- BUY , BUY , BUY
 * */
