#pragma once

#include <thread>

#include <quote.hpp>
#include <session.hpp>

namespace binance {

namespace detail {

using namespace session;

class ThreadProperties : std::enable_shared_from_this<ThreadProperties> {
  public:
    ThreadProperties();

    void Start();
    void Join();

    net::io_context& IOContext();
    net::ssl::context& SSLContext();

  private:
    void ThreadFunc();

  private:
    std::shared_ptr<net::io_context> io_context_;
    std::shared_ptr<net::ssl::context> ssl_context_;
    std::shared_ptr<std::jthread> thread_;
};

}  // namespace detail

class ReaderTemplate {

public:
    virtual void Start();
    virtual void Join();

protected:
    ReaderTemplate(size_t n_threads, size_t start, size_t end, size_t n_websockets);

    virtual void InitThreads();
    virtual void InitParams();
    virtual void InitWebsockets();

    virtual void RunThreads();
    virtual void JoinThreads();

protected:
    size_t n_threads_;
    size_t start_;
    size_t end_;
    size_t n_websockets_;
    std::vector<std::shared_ptr<detail::ThreadProperties>> threads_;
    std::vector<std::string> market_names_;

};

class StreamSession {
  private:
    struct StreamHandler : session::ISessionHandler {
        explicit StreamHandler(const std::string& market_name);

        void Process(std::string msg) final;

        size_t market_id;
        market::MarketInfo market_info;
        QuoteUpdater updater;
    };

  public:
    StreamSession(size_t n_threads, size_t start, size_t end);

    void Start();
    void Join();

  private:
    void InitThreads();
    void InitParams();
    void InitWebsockets();

    void RunThreads();
    void JoinThreads();

  private:
    size_t n_threads_;
    size_t start_;
    size_t end_;
    std::vector<std::shared_ptr<detail::ThreadProperties>> threads_;
    std::vector<std::string> market_names_;
    std::vector<std::shared_ptr<session::Session<StreamHandler>>> websockets_;
};

class TickerBookSession {
  private:
    struct TickerBookHandler : session::ISessionHandler {
        explicit TickerBookHandler();

        void Process(std::string msg) final;

        std::string market_name;
    };

  public:
    TickerBookSession(size_t n_threads, size_t n_websockets, size_t start,
                      size_t end);

    void Start();
    void Join();

  private:
    void InitThreads();
    void InitParams();
    void InitWebsockets();

    void RunThreads();
    void JoinThreads();

  private:
    size_t n_threads_;
    size_t n_websockets_;
    size_t start_;
    size_t end_;
    std::vector<std::shared_ptr<detail::ThreadProperties>> threads_;
    std::vector<std::string> market_names_;
    std::vector<std::shared_ptr<session::Session<TickerBookHandler>>>
        websockets_;
};

}  // namespace binance
