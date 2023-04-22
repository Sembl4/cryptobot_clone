#pragma once

#include <request_data.hpp>
#include <cryptoClient.hpp>
namespace common::algo {

struct FilterBase {
    double stepSize;
    double minNotional;
    double tickSize;
};
struct Filters {
    FilterBase start;
    FilterBase middle;
    FilterBase fin;
};


struct Benefitial
{
    bool is_good_benefit;
    struct {
        double new_benefit;
        double real_invest;
    } inf;
};


namespace detail {}  // namespace detail

struct Algorithm {
    template <request::Operation A, request::Operation B, request::Operation C>
    static void CountVolumes(request::RequestData& req,
                             request::Volume& volumes);


    static void CountRequestVolumes(request::RequestData& req);
    

    template <request::Operation A, request::Operation B, request::Operation C>
    static void CountFilters(request::RequestData& req, double fee);

    // static void CountAllFilters(request::RequestData& req,const Filters& filters, double fee);

    static void CountRequestFilters(request::RequestData& req, double fee);

    template <CRYPTO_PLATFORMS>
    static bool CountBenefitial(request::RequestData& req, double summary_fee, 
                                        double limit, std::string_view asset, size_t index);


    // static Benefitial CountRequestPlatform(request::RequestData& req,
    //                                 const Filters& filters, double summary_fee, CRYPTO_PLATFORMS platform);
};

}  // namespace binance::algo