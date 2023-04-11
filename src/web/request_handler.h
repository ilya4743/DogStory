#pragma once

#include "api_handler.h"
#include "file_handler.h"
#include "http_server.h"
#include "model.h"

namespace http_handler {
namespace net = boost::asio;
using tcp = net::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using JsonResponse = http::response<http::string_body>;

class RequestHandler : public std::enable_shared_from_this<RequestHandler> {
   public:
    explicit RequestHandler(std::shared_ptr<Application> app, std::filesystem::path& root, net::strand<net::io_context::executor_type> strand)
        : api_handler(app), file_handler(root), strand_{strand} {
    }

    RequestHandler(const RequestHandler&) = delete;
    RequestHandler& operator=(const RequestHandler&) = delete;

    template <typename Body, typename Allocator, typename Send>
    void operator()(tcp::endpoint&& endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        if (api_handler.isApiRequest(std::move(req))) {
            auto handle = [self = shared_from_this(), send, req = std::forward<decltype(req)>(req)] {
                assert(self->strand_.running_in_this_thread());
                return send(self->api_handler.ApiHandlerRequest(req));
            };
            return net::dispatch(strand_, handle);
        } else {
            std::visit(
                [&send](auto&& result) {
                    send(std::forward<decltype(result)>(result));
                },
                file_handler.FileRequestHandler(req));
        }
    }

   private:
    ApiHandler api_handler;
    FileHandler file_handler;
    net::strand<net::io_context::executor_type> strand_;
};

}  // namespace http_handler
