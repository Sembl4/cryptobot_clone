#include <triangle.hpp>


#include <config.hpp>
#include <doOrdersClass.hpp>
#include <logger.hpp>

using namespace common;
using namespace binance;
using namespace triangle;
using namespace market;
using namespace quote;
using namespace config;
using namespace request;
using namespace algo;
using std::string;
using std::chrono::nanoseconds;

static double startLimit = 20.0;
static double cur_bal = 0.0;
static std::string working_asset;
static constexpr const char* Buy = "BUY";
static constexpr const char* Sell = "SELL";

static CRYPTO_PLATFORMS platform;
static QuoteManager& quote_manager = QuoteManager::Get();
static MarketManager& market_manager = MarketManager::Get();
// static cryptoClient& client;

///////////// TriangleEdge /////////////

TriangleEdge::TriangleEdge(size_t pair_id)
    : market_id_(pair_id), quote_(quote_manager.GetReader(pair_id)) {
    Init();
}

TriangleEdge::TriangleEdge(const std::string& pair_name)
    : quote_(quote_manager.GetReader(market_manager.MarketId(pair_name))) {
    Init();
}

void TriangleEdge::SetOperation(Operation op) { operation_ = op; }

double TriangleEdge::BenefitSimple(Order& order) const {
    order.market_id = market_id_;
    auto rate = quote_.Get(operation_);
    order.rate.volume = rate.volume;
    order.rate.price = rate.price;
    order.op_type = operation_;
    order.symbols_pair = this->market_.symbol;
    // Добавляем фильтры к этому ордеру
    order.filters.stepSize = this->market_.step_size;
    order.filters.minNotional = this->market_.min_notional;
    order.filters.tickSize = this->market_.tick_size;
    return operation_ == SELL ? order.rate.price : (1.0 / order.rate.price);
}

void TriangleEdge::PrintLog(Order& order) const {
    LogMore(market_.symbol, GetOperationStr(), '\n');
    LogMore("volume:", order.rate.volume, '\n');
    LogMore("price:", order.rate.price, '\n');
}

void TriangleEdge::Init() {
    market_ = market_manager.GetMarketInfo(market_id_);
}

void TriangleEdge::SetCurrencies(const string& start, const string& end) {
    start_currency_ = start;
    end_currency_ = end;
    start_id_ = market_manager.CurrencyId(start);
    end_id_ = market_manager.CurrencyId(end);

    if (start != market_manager.GetName(start_id_) ||
        end != market_manager.GetName(end_id_)) {
        Log("Wrong currency naming in", Logger::FileName(__FILE__), __LINE__);
        std::abort();
    }
}

void TriangleEdge::Print() const {
    LogMore("Market:", market_.symbol);
    if (operation_ == BUY) {
        LogMore("BUY ");
    } else {
        LogMore("SELL ");
    }
    LogMore(" id: ", market_id_, '\n');
}

std::string_view TriangleEdge::GetOperationStr() const {
    return operation_ == BUY ? Buy : Sell;
}

std::string_view TriangleEdge::Symbol() const { return market_.symbol; }

double TriangleEdge::StepSize() const { return market_.step_size; }

double TriangleEdge::MinNotional() const { return market_.min_notional; }

double TriangleEdge::TickSize() const { return market_.tick_size; }

bool TriangleEdge::IsChecked() const { return quote_.IsChecked(operation_); }

// bool TriangleEdge::IsFreshThan() const {
//     int64_t now = std::chrono::duration_cast<nanoseconds>(
//                       std::chrono::system_clock::now().time_since_epoch())
//                       .count();
//     return (now - quote_.GetLastUpdateTimestamp()) <= market_.delay_t;
// }

void TriangleEdge::SetChecked() { quote_.SetChecked(operation_); }

uint64_t TriangleEdge::GetUpdateTimestamp() const {
    return quote_.GetTimestamp(operation_);
}

Rate TriangleEdge::GetRate() const { return quote_.Get(operation_); }

///////////// Triangle /////////////

Triangle::Triangle(size_t start, size_t mid, size_t fin, size_t index)
    : start_(start), middle_(mid), finish_(fin), index_(index) {
    Init();
    if(Config::Market().trading_platform == "Binance")
    {
        CountBenefitial = algo::Algorithm::CountBenefitial<BINANCE>;
    }else if(Config::Market().trading_platform == "OKX")
    {
        CountBenefitial = algo::Algorithm::CountBenefitial<OKX>;
    }
}

void Triangle::Init() {
    summary_fee = 1.0;
    InitEdge(start_);
    InitEdge(middle_);
    InitEdge(finish_);
}

void Triangle::InitEdge(TriangleEdge& edge) {
    const auto& no_fee_list = market_manager.NoFeeMarkets();
    auto find =
        std::find(no_fee_list.cbegin(), no_fee_list.cend(), edge.Symbol());
    if (find == no_fee_list.cend()) {
        summary_fee *= Config::Limits().fee_step;
    }
}

void Triangle::SetOperations(Operation s, Operation m, Operation f) {
    start_.SetOperation(s);
    middle_.SetOperation(m);
    finish_.SetOperation(f);
}

void Triangle::SetCurrencies(const string& start, const string& first,
                             const string& second) {
    start_.SetCurrencies(start, first);
    middle_.SetCurrencies(first, second);
    finish_.SetCurrencies(second, start);
}

bool Triangle::IsChecked() const {
    return start_.IsChecked() || middle_.IsChecked() || finish_.IsChecked();
}

// bool Triangle::IsFreshThan() const {
//     return start_.IsFreshThan() && middle_.IsFreshThan() &&
//            finish_.IsFreshThan();
// }

void Triangle::SetChecked() {
    start_.SetChecked();
    middle_.SetChecked();
    finish_.SetChecked();
}

bool Triangle::BenefitSimple(request::RequestData& request) {
    /// If Quotes haven't been updated yet
    auto check_start_t =
        std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::system_clock::now().time_since_epoch())
            .count();
    if (IsChecked()) {
        return false;
    }
    /// delay in nanoseconds
    // if (!IsFreshThan()) {
    //     return false;
    // }
    SetChecked();

    double benefit = start_.BenefitSimple(request.start);
    benefit *= middle_.BenefitSimple(request.middle);
    benefit *= finish_.BenefitSimple(request.fin);

    if (request.start.rate.volume <= 0 || request.middle.rate.volume <= 0 ||
        request.fin.rate.volume <= 0) {
        return false;
    }
    // this->Print();
    // LogMore(request.start.symbols_pair,request.start.rate.price,request.start.rate.volume);
    // LogMore(request.middle.symbols_pair,request.middle.rate.price,request.middle.rate.volume);
    // LogMore(request.fin.symbols_pair,request.fin.rate.price,request.fin.rate.volume);
    // // LogMore("Potential volume:",p_volume, "=",p_volume_usdt, working_asset,'\n');
    // LogMore("Benefit: ", benefit, '\n');
    /// If not beneficial return
    if (benefit < summary_fee || benefit > 2) {
        return false;
    }
    
    // if (benefit <= summary_fee) {
    //     return false;
    // }
    /// Count real volumes

    if(this->CountBenefitial(request, summary_fee, startLimit, working_asset, index_))
    {
        auto check_finish_t =
            std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        Log("Triangle check costed: ", check_finish_t - check_start_t,
            "nanoseconds\n");
        return true;
    }
    return false;
}

///////////// TriangleFactory /////////////

using hash = std::hash<size_t>;

TriangleSerial::TriangleSerial(const TriangleSerial& other)
    : start(other.start), middle(other.middle), fin(other.fin) {}

TriangleSerial::TriangleSerial(size_t s, size_t m, size_t f)
    : start(s), middle(m), fin(f) {}

size_t TriangleSerial::Hash::operator()(const TriangleSerial& serial) const {
    return hash()(serial.start) + 2 * hash()(serial.middle) +
           3 * hash()(serial.fin);
}

bool TriangleSerial::operator==(const TriangleSerial& other) const {
    return start == other.start && middle == other.middle && fin == other.fin;
}

size_t TriangleFactory::BuildTriangle(size_t start, size_t mid, size_t fin) {
    TriangleSerial serial(start, mid, fin);
    if (triangles_id_.contains(serial)) {
        return triangles_id_[serial];
    }
    triangles_list_.emplace_back(start, mid, fin, triangles_list_.size());
    size_t index = triangles_list_.size() - 1;
    triangles_id_[serial] = index;
    return index;
}

size_t TriangleFactory::TrianglesCount() const {
    return triangles_list_.size();
}

void TriangleFactory::BuildTrianglePack() {
    Clear();
    Log("----Building Triangles----");
    /// Init working asset name
    working_asset = Config::Market().working_asset;
    /// Init thresholds
    startLimit = Config::Limits().order_start_limit;
    Log("Start limit for triangles is: ", startLimit);
    /// Build triangles
    const std::string file_name =
        "triangles_" + Config::Market().working_asset + "_" + Config::Market().trading_platform + ".txt";
    Log("Building triangles from file", file_name);
    std::ifstream file(file_name);
    if (!file.is_open()) {
        std::cerr << "Couldn't open file in BuildTrianglePack\n";
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        assert(false);
    }
    std::vector<size_t> ids(3, 0);
    std::vector<Operation> op(3, BUY);
    std::string market;
    std::string oper;
    while (!file.eof()) {
        for (size_t index = 0; index < 3; ++index) {
            market.clear();
            oper.clear();
            file >> market;
            if (market.length() <= 3) {
                return;
            }
            file >> oper;
            if (oper.length() < 3) {
                return;
            }
            ids[index] = market_manager.MarketId(market);
            op[index] = oper == Sell ? SELL : BUY;
        }
        size_t t_index = BuildTriangle(ids[0], ids[1], ids[2]);
        triangles_list_[t_index].SetOperations(op[0], op[1], op[2]);
    }
    Log("Triangles successfully built, count:", TrianglesCount());
}

void TriangleFactory::Clear() {
    triangles_list_.clear();
    triangles_id_.clear();
}

void Triangle::Print() const {
    Log("Triangle: ", index_);
    LogMore(start_.Symbol(), start_.GetOperationStr(), ";");
    LogMore(middle_.Symbol(), middle_.GetOperationStr(), ";");
    LogMore(finish_.Symbol(), finish_.GetOperationStr(), ";\n");
    // LogFlush();
}

void Triangle::PrintWithMarkets() const {
    Log("Triangle: ", index_);
    LogMore(start_.Symbol(), start_.StepSize(), start_.MinNotional(),
            start_.GetOperationStr(), ";");
    LogMore(middle_.Symbol(), middle_.StepSize(), middle_.MinNotional(),
            middle_.GetOperationStr(), ";");
    LogMore(finish_.Symbol(), finish_.StepSize(), finish_.MinNotional(),
            finish_.GetOperationStr(), ";\n");
    LogFlush();
}

void Triangle::PrintEdgesUpdateTimestamps() const {
    LogMore("last update timestamp for start: ", start_.GetUpdateTimestamp(),
            '\n');
    LogMore("last update timestamp for middle: ", middle_.GetUpdateTimestamp(),
            '\n');
    LogMore("last update timestamp for finish: ", finish_.GetUpdateTimestamp(),
            '\n');
}

void TriangleFactory::PrintAllTriangles() const {
    for (size_t i = 0; i < triangles_list_.size(); ++i) {
        triangles_list_[i].Print();
    }
}

void TriangleFactory::PrintAllTrianglesWithMarkets() const {
    for (size_t i = 0; i < triangles_list_.size(); ++i) {
        triangles_list_[i].PrintWithMarkets();
    }
}

TriangleFactory& TriangleFactory::Get() {
    static TriangleFactory factory;
    return factory;
}

TrianglePack TriangleFactory::GetHandler(size_t begin, size_t end) {
    return TrianglePack(triangles_list_, begin, end);
}

///////////// TrianglePack /////////////

TrianglePack::TrianglePack(std::vector<Triangle>& list, size_t begin,
                           size_t end)
    :run_type_(Config::Limits().run_type), triangles_(list), begin_(begin), end_(end) {}

TrianglePack::TrianglePack(TrianglePack&& other)
    : triangles_(other.triangles_), begin_(other.begin_), end_(other.end_) {}

void TrianglePack::WorkLoop() {
    static cryptoClient& client = cryptoClient::getClient();
    // client.Connect();
    static doOrders orders(client);
    std::cerr << "WoorkloopStart\n" << std::flush;
    LogFlush();
    while (process_.load()) 
    {
        // cur_bal = client.balances().assets[working_asset];
        for (size_t i = begin_; i < end_; ++i) {
            RequestData request;
            bool is_beneficial = triangles_[i].BenefitSimple(request);

            if (is_beneficial) {
                orders.addOrder(request);
            }
        }
    }
}

void TrianglePack::Start() {
    Log("Triangle handler started for segment: ", begin_, end_);
    LogFlush();
    process_.store(true);
    thread_ = std::make_unique<std::thread>([this]() { WorkLoop(); });
}

void TrianglePack::Stop() {
    Log("Triangle handler stopped for segment: ", begin_, end_);
    process_.store(false);
    thread_->join();
    Log("Triangle handler thread joined");
}
