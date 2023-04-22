#pragma once

#include <string>
#include <vector>

namespace init {

namespace detail {

struct MarketInfo {
    std::string base_asset;
    std::string quote_asset;
    double min_notional;
    double step_size;
    double tick_size;
};

struct Triangle {
    std::vector<std::string> market;
    std::vector<std::string> option;
};

}  // namespace detail

class InitMaster {
  public:
    InitMaster(InitMaster& other) = delete;
    InitMaster(InitMaster&& other) = delete;

    static InitMaster& Get();
    void Setup();

  private:
    InitMaster();
    void Clear();

  private:
    std::vector<detail::Triangle> triangles_;
    std::vector<detail::MarketInfo> markets_;
    std::string markets_path_;
    std::string triangles_path_;
    std::string platform_;
    std::string asset_;
};

}  // namespace init