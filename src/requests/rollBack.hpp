#pragma once

#include <binanceClient.hpp>
#include <blocking_queue.hpp>
#include <optional>
#include <request_data.hpp>

namespace answers
{
struct AwaitAnswers {
    std::vector<uint64_t> start;
    std::vector<uint64_t> mid;
    std::vector<uint64_t> fin;
};

struct AnsReq {
    AwaitAnswers awaits;
    common::request::RequestData requests;
};

struct Answers {
    bool start;
    bool mid;
    bool fin;
    
    bool isOk()
    {
        return start && mid && fin;
    }
};

bool checkAnswers(cryptoClient& client ,AnsReq ans);
}// namespace answers

namespace rollback {
struct RollBack
{
    static void RollBackBalance(cryptoClient& client);

    template <CRYPTO_PLATFORMS>
    static void rollBack(cryptoClient& client);    
};

}  // namespace rollback