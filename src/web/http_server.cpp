#include "http_server.h"

#include <boost/asio/dispatch.hpp>
#include <iostream>

#include "logger.h"

namespace http_server {

void ReportError(beast::error_code ec, std::string_view what) {
    std::cerr << what << ": "sv << ec.message() << std::endl;
}

void SessionBase::Run() {
    // Вызываем метод Read, используя executor объекта stream_.
    // Таким образом вся работа со stream_ будет выполняться, используя его executor
    net::dispatch(stream_.get_executor(),
                  beast::bind_front_handler(&SessionBase::Read, GetSharedThis()));
}

void SessionBase::Read() {
    // Очищаем запрос от прежнего значения (метод Read может быть вызван несколько раз)
    request_ = {};
    stream_.expires_after(30s);
    // Считываем request_ из stream_, используя buffer_ для хранения считанных данных
    http::async_read(stream_, buffer_, request_,
                     // По окончании операции будет вызван метод OnRead
                     beast::bind_front_handler(&SessionBase::OnRead, GetSharedThis()));
}

void SessionBase::OnRead(beast::error_code ec, [[maybe_unused]] std::size_t bytes_read) {
    if (ec == http::error::end_of_stream) {
        // Нормальная ситуация - клиент закрыл соединение
        return Close();
    }
    if (ec) {
        json::value error{{"code", ec.value()},
                          {"text", ec.message()},
                          {"where", "read"}};
        BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, error) << "error";
        return ReportError(ec, "read"sv);
    }
    HandleRequest(std::move(request_));
}

void SessionBase::Close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_send, ec);
}

void SessionBase::OnWrite(bool close, beast::error_code ec, [[maybe_unused]] std::size_t bytes_written) {
    if (ec) {
        json::value error{{"code", ec.value()},
                          {"text", ec.message()},
                          {"where", "write"}};
        BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, error) << "error";
        return ReportError(ec, "write"sv);
    }

    if (close) {
        // Семантика ответа требует закрыть соединение
        return Close();
    }

    // Считываем следующий запрос
    Read();
}

tcp::endpoint SessionBase::GetEndpoint() const {
    return stream_.socket().remote_endpoint();
}

ListenerBase::ListenerBase(net::io_context& ioc, const tcp::endpoint& endpoint)
    : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
    // Открываем acceptor, используя протокол (IPv4 или IPv6), указанный в endpoint
    acceptor_.open(endpoint.protocol());

    // После закрытия TCP-соединения сокет некоторое время может считаться занятым,
    // чтобы компьютеры могли обменяться завершающими пакетами данных.
    // Однако это может помешать повторно открыть сокет в полузакрытом состоянии.
    // Флаг reuse_address разрешает открыть сокет, когда он "наполовину закрыт"
    acceptor_.set_option(net::socket_base::reuse_address(true));
    // Привязываем acceptor к адресу и порту endpoint
    acceptor_.bind(endpoint);
    // Переводим acceptor в состояние, в котором он способен принимать новые соединения
    // Благодаря этому новые подключения будут помещаться в очередь ожидающих соединений
    acceptor_.listen(net::socket_base::max_listen_connections);
}

void ListenerBase::Run() {
    DoAccept();
}

void ListenerBase::DoAccept() {
    acceptor_.async_accept(
        // Передаём последовательный исполнитель, в котором будут вызываться обработчики
        // асинхронных операций сокета
        net::make_strand(ioc_),
        // С помощью bind_front_handler создаём обработчик, привязанный к методу OnAccept
        // текущего объекта.
        // Так как Listener — шаблонный класс, нужно подсказать компилятору, что
        // shared_from_this — метод класса, а не свободная функция.
        // Для этого вызываем его, используя this
        // Этот вызов bind_front_handler аналогичен
        // namespace ph = std::placeholders;
        // std::bind(&Listener::OnAccept, this->shared_from_this(), ph::_1, ph::_2)
        beast::bind_front_handler(&ListenerBase::OnAccept, GetSharedThis()));
}

// Метод socket::async_accept создаст сокет и передаст его передан в OnAccept
void ListenerBase::OnAccept(sys::error_code ec, tcp::socket socket) {
    if (ec) {
        json::value error{{"code", ec.value()},
                          {"text", ec.message()},
                          {"where", "accept"}};
        BOOST_LOG_TRIVIAL(error) << logging::add_value(additional_data, error) << "error";
        return ReportError(ec, "accept"sv);
    }

    // Асинхронно обрабатываем сессию
    AsyncRunSession(std::move(socket));

    // Принимаем новое соединение
    DoAccept();
}

}  // namespace http_server
