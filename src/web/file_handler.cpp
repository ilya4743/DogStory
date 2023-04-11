#include "file_handler.h"

#include <charconv>
#include <iostream>

namespace http_handler {
using namespace std::literals;
// Возвращает true, если каталог p содержится внутри base_path.
bool FileHandler::IsSubPath(fs::path path, fs::path base) {
    // Приводим оба пути к каноничному виду (без . и ..)
    path = fs::weakly_canonical(path);
    base = fs::weakly_canonical(base);

    // Проверяем, что все компоненты base содержатся внутри path
    for (auto b = base.begin(), p = path.begin(); b != base.end(); ++b, ++p) {
        if (p == path.end() || *p != *b) {
            return false;
        }
    }
    return true;
}

std::string FileHandler::DecodeUrl(std::string_view encoded_url) {
    std::string decoded_url;
    decoded_url.reserve(encoded_url.size());
    char symbol;
    for (size_t i = 0; i < encoded_url.size(); i++) {
        symbol = encoded_url[i];
        if (symbol == '%' && i < encoded_url.size() - 2) {
            std::from_chars(&encoded_url[i + 1], &encoded_url[i + 3], symbol, 16);
            i = i + 2;
        } else if (symbol == '+') {
            symbol = ' ';
        }
        decoded_url.push_back(symbol);
    }
    return decoded_url;
}

std::string_view FileHandler::GetMimeType(std::string_view extension) {
    if (extension == ".htm"sv || extension == ".html"sv) return "text/html"sv;
    if (extension == ".css"sv) return "text/css"sv;
    if (extension == ".txt"sv) return "text/plain"sv;
    if (extension == ".js"sv) return "text/javascript"sv;
    if (extension == ".json"sv) return "application/json"sv;
    if (extension == ".xml"sv) return "application/xml"sv;
    if (extension == ".png"sv) return "image/png"sv;
    if (extension == ".jpg"sv || extension == ".jpe"sv || extension == ".jpeg"sv) return "image/jpeg"sv;
    if (extension == ".gif"sv) return "image/gif"sv;
    if (extension == ".bmp"sv) return "image/bmp"sv;
    if (extension == ".ico"sv) return "image/ico"sv;
    if (extension == ".tiff"sv || extension == ".tif"sv) return "image/tiff"sv;
    if (extension == ".svg"sv || extension == ".svgz"sv) return "image/svg+xml"sv;
    if (extension == ".mp3"sv) return "audio/mpeg"sv;
    return {};
}

std::variant<StringResponse, FileResponse> FileHandler::FileRequestHandler(const StringRequest& request) {
    auto target = request.target();
    auto path = root_ / std::filesystem::path{DecodeUrl(target)}.lexically_relative("/");
    if (IsSubPath(path, root_))
        if (exists(path)) {
            if (is_directory(path))
                path = root_ / "index.html"sv;
            http::file_body::value_type file;
            beast::error_code ec;
            file.open(path.c_str(), beast::file_mode::read, ec);
            if (ec) {
                throw std::runtime_error("Failed to open file");
            }
            return MakeFileResponse(http::status::ok, std::move(file), request.version(), request.keep_alive(),
                                    GetMimeType(path.extension().c_str()));
        } else {
        }
    return MakeStringResponse(http::status::not_found, std::string_view{"Not found"sv}, request.version(), request.keep_alive(),
                              ContentType::PLAIN_TEXT);
}

}  // namespace http_handler