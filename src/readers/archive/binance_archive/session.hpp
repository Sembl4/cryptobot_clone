#pragma once

#include <iostream>
#include <memory>

#include <boost/asio/strand.hpp>
#include <boost/atomic.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

namespace session {

namespace beast = boost::beast;
namespace net = boost::asio;

template <typename T>
concept READER = requires(T h, std::string b) {
    h.Process(b);
};

template <class Derived, class Base>
concept INHERITABLE = requires(Derived* d, Base* b) {
    b = dynamic_cast<Base*>(d);
};

///////////// ISessionHandler /////////////

class ISessionHandler {
    virtual void Process(std::string msg) = 0;
};

///////////// Session Template /////////////

template <typename Handler>
requires INHERITABLE<Handler, ISessionHandler>
class Session : public std::enable_shared_from_this<Session<Handler>> {
  private:
    using Resolver = net::ip::tcp::resolver;
    using ErrCode = beast::error_code;

    enum State {
        kReady,
        kRunning,
        kClosing,
        kStopped,
    };

  public:
    Session(net::io_context& ioc, net::ssl::context& ssl_context,
            Handler handler, const std::string& user_agent = "");
    Session(Session<Handler>&& other);

    void Run(const std::string& host, const std::string& port,
             const std::string& handshake_path, const std::string& text);

    void Close();

  private:
    void Panic(ErrCode ec, char const* where);

    void ResolveCallBack(ErrCode ec, Resolver::results_type results);

    void ConnectCallBack(ErrCode ec, Resolver::results_type::endpoint_type ep);

    void SSLHandShakeCallBack(ErrCode ec);

    void HandShakeCallBack(ErrCode ec);

    void ReadCallBack(ErrCode ec, size_t bytes_transferred);

    void CloseCallBack(const ErrCode& ec);

  private:
    boost::asio::ip::tcp::resolver resolver_;
    beast::websocket::stream<beast::ssl_stream<beast::tcp_stream>> ws_;
    Handler handler_;
    const std::string user_agent_;
    std::string host_;            // stream.binance.com
    std::string port_;            // 443
    std::string handshake_path_;  // /ws
    std::string text_;
    // {"method":"SUBSCRIBE","params":["btcusdt@bookTicker"],"id":1}
    beast::flat_buffer buffer_;
    boost::atomic<State> state_{kReady};
};

template <typename T>
requires INHERITABLE<T, ISessionHandler> Session<T>::Session(
    net::io_context& ioc, net::ssl::context& ssl_context, T handler,
    const std::string& user_agent)
    : resolver_(net::make_strand(ioc)),
      ws_(net::make_strand(ioc), ssl_context),
      handler_(std::move(handler)),
      user_agent_(user_agent) {}

template <typename T>
requires INHERITABLE<T, ISessionHandler> Session<T>::Session(Session<T>&& other)
    : resolver_(std::move(other.resolver_)),
      ws_(std::move(other.ws_)),
      handler_(std::move(other.handler_)),
      user_agent_(std::move(other.user_agent_)) {}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::Run(const std::string& host, const std::string& port,
                     const std::string& handshake_path,
                     const std::string& text) {
    host_ = host;
    port_ = port;
    handshake_path_ = handshake_path;
    text_ = text;
    state_.store(kRunning);
    resolver_.async_resolve(
        host, port_,
        beast::bind_front_handler(
            &Session<T>::ResolveCallBack,
            std::enable_shared_from_this<Session<T>>::shared_from_this()));
}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::Close() { state_.store(kClosing); }

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::Panic(ErrCode ec, const char* where) {
    std::cerr << where << ": " << ec.message() << std::endl;
}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::ResolveCallBack(ErrCode ec, Resolver::results_type results) {
    if (ec) {
        return Panic(ec, "Resolve");
    }

    // Timeout for the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    beast::get_lowest_layer(ws_).async_connect(
        results,
        beast::bind_front_handler(
            &Session<T>::ConnectCallBack,
            std::enable_shared_from_this<Session<T>>::shared_from_this()));
}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::ConnectCallBack(ErrCode ec,
                                 Resolver::results_type::endpoint_type ep) {
    if (ec) {
        return Panic(ec, "Connect");
    }

    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(10));
    host_ += ':' + std::to_string(ep.port());

    if (!SSL_set_tlsext_host_name(ws_.next_layer().native_handle(),
                                  host_.c_str())) {
        ec = beast::error_code(static_cast<int>(::ERR_get_error()),
                               net::error::get_ssl_category());
        return Panic(ec, "Connect");
    }

    ws_.next_layer().async_handshake(
        net::ssl::stream_base::client,
        beast::bind_front_handler(
            &Session<T>::SSLHandShakeCallBack,
            std::enable_shared_from_this<Session<T>>::shared_from_this()));
}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::SSLHandShakeCallBack(ErrCode ec) {
    if (ec) {
        return Panic(ec, "SSL Handshake");
    }
    namespace websocket = beast::websocket;

    beast::get_lowest_layer(ws_).expires_never();
    ws_.set_option(
        websocket::stream_base::timeout::suggested(beast::role_type::client));
    ws_.set_option(websocket::stream_base::decorator(
        [user_agent = user_agent_](websocket::request_type& req) {
            req.set(beast::http::field::user_agent, user_agent);
        }));

    ws_.async_handshake(
        host_, handshake_path_,
        beast::bind_front_handler(
            &Session<T>::HandShakeCallBack,
            std::enable_shared_from_this<Session<T>>::shared_from_this()));
}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::HandShakeCallBack(ErrCode ec) {
    if (ec) {
        return Panic(ec, "Handshake");
    }
    ws_.async_write(
        net::buffer(text_),
        beast::bind_front_handler(
            &Session<T>::ReadCallBack,
            std::enable_shared_from_this<Session<T>>::shared_from_this()));
}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::ReadCallBack(ErrCode ec, size_t bytes_transferred) {
    if (ec == net::error::shut_down || ec == net::error::operation_aborted) {
        return;
    } else if (ec) {
        return Panic(ec, "read");
    }

    handler_.Process(beast::buffers_to_string(buffer_.cdata()));

    buffer_.clear();
    if (state_.load() == kRunning) {
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                &Session<T>::ReadCallBack,
                std::enable_shared_from_this<Session<T>>::shared_from_this()));
    } else if (state_.load() == kClosing) {
        ErrCode close_ec;
        beast::websocket::close_reason cr("Finish work");
        ws_.async_close(
            cr,
            beast::bind_front_handler(
                &Session<T>::CloseCallBack,
                std::enable_shared_from_this<Session<T>>::shared_from_this()));
    } else {
        std::cerr << "Something went wrong in websocket ReadCallBack\n";
        std::abort();
    }
}

template <typename T>
requires INHERITABLE<T, ISessionHandler>
void Session<T>::CloseCallBack(const ErrCode& ec) {
    if (ec) {
        return Panic(ec, "Close");
    }
    state_.store(kStopped);
}

}  // namespace session

/// Usage Example
/*
struct Hand : SessionHandler {
    virtual void Process(std::string msg) override final { std::cout << msg <<
std::endl; }
};
 int main() {
    auto const host = "stream.binance.com";
    auto const port = "9443";
    auto const path = "/ws";
    auto const text =
        R"({"method": "SUBSCRIBE", "params": ["btcusdt@bookTicker"], "id": 0})";

    auto ctx = std::make_shared<boost::asio::ssl::context>(
        boost::asio::ssl::context::tlsv12_client);
    net::io_context ioc;
    Hand hand;
    auto session = std::make_shared<Session<Hand>>(ioc, *ctx, hand);
    session->Run(host, port, path, text);
    ioc.run();
    return 0;
    }
 * */
