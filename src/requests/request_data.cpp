#include <request_data.hpp>
#include <logger.hpp>


using namespace common;
using namespace request;
using namespace quote;

static QuoteManager& quote_manager = QuoteManager::Get();

Rate Order::ActualRate() const {
    return quote_manager.GetRate(market_id, op_type);
}

double Order::ActualPrice() const {
    return quote_manager.GetRate(market_id, op_type).price;
}

double Order::ActualVolume() const {
    return quote_manager.GetRate(market_id, op_type).volume;
}

bool Order::Decrease() const {
    return quote_manager.GetUpdater(market_id).CompareAndDecrease(
        static_cast<int32_t>(op_type), rate.price, rate.volume);
}

bool Order::MatchPrice() const {
    return quote_manager.GetReader(market_id).MatchPrice(
        static_cast<size_t>(op_type), rate.price);
}

bool RequestData::CompareAndDecrease() const {
    if (start.MatchPrice() && middle.MatchPrice() && fin.MatchPrice()) {
        start.Decrease();
        middle.Decrease();
        fin.Decrease();
        return true;
    }
    return false;
}

void RequestData::PrintMarketsOps() const
{
    LogMore("Request ID: ", ID,"\n");
    LogMore(start.symbols_pair, start.op_type, ";");
    LogMore(middle.symbols_pair, middle.op_type, ";");
    LogMore(fin.symbols_pair, fin.op_type, ";\n");
}