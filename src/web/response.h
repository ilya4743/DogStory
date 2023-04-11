#pragma once
#include <boost/beast/http.hpp>

namespace http_handler {
namespace beast = boost::beast;
namespace http = beast::http;
// Запрос, тело которого представлено в виде строки
using StringRequest = http::request<http::string_body>;
// Ответ, тело которого представлено в виде строки
using StringResponse = http::response<http::string_body>;
using FileResponse = http::response<http::file_body>;
using JsonResponse = http::response<http::string_body>;
using namespace std::literals;

struct ContentType {
    ContentType() = delete;
    constexpr static std::string_view TEXT_HTML = "text/html"sv;
    constexpr static std::string_view PLAIN_TEXT = "text/plain"sv;
    constexpr static std::string_view APPLICATION_JSON = "application/json"sv;
};

// Создаёт StringResponse с заданными параметрами
StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type);

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type, std::string_view cache_control);

StringResponse MakeStringResponse(http::status status, std::string_view body, unsigned http_version,
                                  bool keep_alive,
                                  std::string_view content_type, std::string_view cache_control, std::string_view allow);

FileResponse MakeFileResponse(http::status status, http::file_body::value_type&& body, unsigned http_version,
                              bool keep_alive,
                              std::string_view content_type);

}  // namespace http_handler