#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>

#include "logger.h"

using tcp = boost::asio::ip::tcp;
namespace beast = boost::beast;
namespace http = beast::http;
namespace fs = std::filesystem;

namespace json = boost::json;

template <class SomeRequestHandler>
class LoggingRequestHandler {
   public:
    LoggingRequestHandler(SomeRequestHandler handler)
        : decorated_(std::move(handler)) {
    }

    template <typename Body, typename Allocator, typename Send>
    void operator()(tcp::endpoint&& endpoint, http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
        auto ip = endpoint.address().to_string();
        auto start_ts = std::chrono::system_clock::now();
        LogRequest(ip, req);

        auto logging_send = [send = std::forward<Send>(send), ip, start_ts](auto&& response) {
            auto end_ts = std::chrono::system_clock::now();
            LogResponse(ip, response, end_ts - start_ts);
            send(std::forward<decltype(response)>(response));
        };

        decorated_(std::forward<decltype(endpoint)>(endpoint), std::forward<decltype(req)>(req), logging_send);
    }

   private:
    template <typename Body, typename Allocator>
    static void LogRequest(const std::string& ip, const http::request<Body, http::basic_fields<Allocator>>& request) {
        json::value value{{"ip", ip},
                          {"URI", request.target()},
                          {"method", request.method_string()}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, value) << "request received";
    }

    template <typename T>
    static void LogResponse(const std::string& ip, const http::response<T>& response, std::chrono::system_clock::duration duration) {
        std::string_view content_type;
        if (response.count(response[http::field::content_type]) != 0)
            content_type = "null";
        else
            content_type = response[http::field::content_type];
        auto dur_millis = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count();
        json::value value{{"response_time", dur_millis},
                          {"code", response.result_int()},
                          {"content_type", content_type}};
        BOOST_LOG_TRIVIAL(info) << logging::add_value(additional_data, value) << "response sent";
    }

    SomeRequestHandler decorated_;
};