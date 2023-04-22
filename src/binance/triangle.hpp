#pragma once

#include <quote.hpp>
#include <request_data.hpp>
#include <algorithms.hpp>

#include <memory>
#include <optional>
#include <thread>
#include <functional>

namespace binance {

namespace triangle {

using common::request::Operation;
///////////// TriangleEdge /////////////

class TriangleEdge {
  public:
    TriangleEdge(size_t pair_id);

    TriangleEdge(const std::string& pair_type);

    // Counts benefit and fills single order data
    double BenefitSimple(common::request::Order& order) const;
    void SetOperation(Operation op);
    void SetCurrencies(const std::string& start, const std::string& end);
    void Print() const;
    void PrintLog(common::request::Order& order) const;
    [[nodiscard]] std::string_view GetOperationStr() const;
    [[nodiscard]] std::string_view Symbol() const;
    [[nodiscard]] double StepSize() const;
    [[nodiscard]] double MinNotional() const;
    [[nodiscard]] double TickSize() const;
    bool IsChecked() const;
    void SetChecked();
    uint64_t GetUpdateTimestamp() const;  // in nanoseconds
    common::Rate GetRate() const;

  private:
    void Init();

  private:
    size_t market_id_;
    common::quote::QuoteReader quote_;
    size_t start_id_;             // Optional
    size_t end_id_;               // Optional
    std::string start_currency_;  // Optional
    std::string end_currency_;    // Optional
    common::market::MarketInfo market_;
    Operation operation_;
};

///////////// Triangle /////////////

class Triangle {
  public:
    Triangle(size_t start, size_t mid, size_t fin, size_t index);

    // Whole triangle's benefit
    bool BenefitSimple(common::request::RequestData& request);
    // Set and reset methods
    void SetOperations(Operation s, Operation m, Operation f);
    void SetCurrencies(const std::string& start, const std::string& first,
                       const std::string& second);
    void SetChecked();

    // Condition checking methods
    [[nodiscard]] bool IsChecked() const;
    //    [[nodiscard]] bool IsFreshThan() const;  // nanoseconds

    // Log methods
    void Print() const;
    void PrintEdgesUpdateTimestamps() const;
    void PrintWithMarkets() const;

  private:
    void Init();

    void InitEdge(TriangleEdge& edge);

    std::function<bool(common::request::RequestData& req, double summary_fee, 
                          	double limit, std::string_view asset, size_t index)> CountBenefitial;

  private:
    TriangleEdge start_;
    TriangleEdge middle_;
    TriangleEdge finish_;
    size_t index_;
    double summary_fee;
};

///////////// TriangleFactory /////////////

struct TriangleSerial {
    TriangleSerial(const TriangleSerial& other);

    TriangleSerial(size_t s, size_t m, size_t f);

    struct Hash {
        size_t operator()(const TriangleSerial& serial) const;
    };

    bool operator==(const TriangleSerial& other) const;

    size_t start;
    size_t middle;
    size_t fin;
};

class TrianglePack;

class TriangleFactory {
  public:
    ~TriangleFactory() = default;

    TriangleFactory(TriangleFactory& other) = delete;
    TriangleFactory(TriangleFactory&& other) = delete;

    static TriangleFactory& Get();
    size_t BuildTriangle(size_t start, size_t mid, size_t fin);
    void BuildTrianglePack();
    TrianglePack GetHandler(size_t begin, size_t end);
    size_t TrianglesCount() const;
    void PrintAllTriangles() const;
    void PrintAllTrianglesWithMarkets() const;

  private:
    TriangleFactory() = default;

    void Clear();

  private:
    std::unordered_map<TriangleSerial, size_t, TriangleSerial::Hash>
        triangles_id_;
    std::vector<Triangle> triangles_list_;
};

///////////// TrianglePack /////////////

class TrianglePack {
  public:
    TrianglePack(std::vector<Triangle>&, size_t begin, size_t end);
    TrianglePack(TrianglePack&&);

    ~TrianglePack() = default;
    TrianglePack(const TrianglePack& other) = delete;
    TrianglePack& operator=(const TrianglePack& other) = delete;

    void Start();
    void Stop();

  private:
    void WorkLoop();

  private:
    const std::string run_type_;
    std::vector<Triangle>& triangles_;
    const size_t begin_;
    const size_t end_;
    std::unique_ptr<std::thread> thread_{nullptr};
    std::atomic<bool> process_{false};
};

}  // namespace triangle

}  // namespace binance