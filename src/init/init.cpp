#include <init.hpp>

#include <binanceClient.hpp>
#include <config.hpp>
#include <tuple>

#include <unordered_map>

using namespace common;
using namespace init;
using namespace detail;
using std::string;
using std::vector;

InitMaster::InitMaster() {}

InitMaster& InitMaster::Get() {
    static InitMaster master;
    return master;
}

void InitMaster::Setup() {
    platform_ = config::Config::Market().trading_platform;
    asset_ = config::Config::Market().working_asset;
    markets_path_ = "pairs_" + asset_ + "_" + platform_ + ".txt";
    triangles_path_ = "triangles_" + asset_ + "_" + platform_ + ".txt";
    Clear();
    system("cp ../scripts/startup_script.py .");
    string command = "python3 startup_script.py " + asset_ + " " + platform_;
    system(command.c_str());
    command = "cp ../research_data/" + markets_path_ + " .";
    system(command.c_str());
    command = "cp ../research_data/" + triangles_path_ + " .";
    system(command.c_str());
}

void InitMaster::Clear() {
    triangles_.clear();
    markets_.clear();
    std::string command = "rm -rf " + markets_path_;
    system(command.c_str());
    command = "rm -rf " + triangles_path_;
    system(command.c_str());
}
