#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>

#include "sdk.h"

using namespace std::literals;

namespace http_server {
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace sys = boost::system;

class SessionBase {
   public:
    // Запрещаем копирование и присваивание объектов SessionBase и его наследников
    SessionBase(const SessionBase&) = delete;
    SessionBase& operator=(const SessionBase&) = delete;
    void Run();
    tcp::endpoint GetEndpoint() const;

   protected:
    explicit SessionBase(tcp::socket&& socket) : stream_(std::move(socket)) {}

    using HttpRequest = http::request<http::string_body>;

    ~SessionBase() = default;

    template <typename Body, typename Fields>
    void Write(http::response<Body, Fields>&& response) {
        // Запись выполняется асинхронно, поэтому response перемещаем в область кучи
        auto safe_response = std::make_shared<http::response<Body, Fields>>(std::move(response));

        auto self = GetSharedThis();
        http::async_write(stream_, *safe_response,
                          [safe_response, self](beast::error_code ec, std::size_t bytes_written) {
                              self->OnWrite(safe_response->need_eof(), ec, bytes_written);
                          });
    }

   private:
    // tcp_stream содержит внутри себя сокет и добавляет поддержку таймаутов
    beast::tcp_stream stream_;
    beast::flat_buffer buffer_;
    HttpRequest request_;

    virtual std::shared_ptr<SessionBase> GetSharedThis() = 0;
    virtual void HandleRequest(HttpRequest&& request) = 0;

    void Read();
    void OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read);
    void Close();
    void OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written);
};

template <typename RequestHandler>
class Session : public SessionBase, public std::enable_shared_from_this<Session<RequestHandler>> {
   public:
    template <typename Handler>
    Session(tcp::socket&& socket, Handler&& request_handler)
        : SessionBase(std::move(socket)), request_handler_(std::forward<Handler>(request_handler)) {
    }

   private:
    RequestHandler request_handler_;

    std::shared_ptr<SessionBase> GetSharedThis() override {
        return this->shared_from_this();
    }

    void HandleRequest(HttpRequest&& request) override {
        // Захватываем умный указатель на текущий объект Session в лямбде,
        // чтобы продлить время жизни сессии до вызова лямбды.
        // Используется generic-лямбда функция, способная принять response произвольного типа
        request_handler_(GetEndpoint(), std::move(request), [self = this->shared_from_this()](auto&& response) {
            self->Write(std::move(response));
        });
    }
};

class ListenerBase {
   public:
    void Run();

   protected:
    ListenerBase(net::io_context& ioc, const tcp::endpoint& endpoint);
    ~ListenerBase() = default;

   private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

    virtual std::shared_ptr<ListenerBase> GetSharedThis() = 0;
    virtual void AsyncRunSession(tcp::socket&& socket) = 0;

    void DoAccept();
    void OnAccept(sys::error_code ec, tcp::socket socket);
};

template <typename RequestHandler>
class Listener : public ListenerBase, public std::enable_shared_from_this<Listener<RequestHandler>> {
   public:
    template <typename Handler>
    Listener(net::io_context& ioc, const tcp::endpoint& endpoint, Handler&& request_handler)
        : ListenerBase(ioc, endpoint), request_handler_(std::forward<Handler>(request_handler)) {
    }

   private:
    RequestHandler request_handler_;

    std::shared_ptr<ListenerBase> GetSharedThis() override {
        return this->shared_from_this();
    }
    void AsyncRunSession(tcp::socket&& socket) override {
        std::make_shared<Session<RequestHandler>>(std::move(socket), request_handler_)->Run();
    }
};

template <typename RequestHandler>
void ServeHttp(net::io_context& ioc, const tcp::endpoint& endpoint, RequestHandler&& handler) {
    // При помощи decay_t исключим ссылки из типа RequestHandler,
    // чтобы Listener хранил RequestHandler по значению
    using MyListener = Listener<std::decay_t<RequestHandler>>;

    std::make_shared<MyListener>(ioc, endpoint, std::forward<RequestHandler>(handler))->Run();
}

}  // namespace http_server
