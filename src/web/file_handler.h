#pragma once
#include <filesystem>
#include <variant>

#include "response.h"

namespace http_handler {
namespace fs = std::filesystem;

class FileHandler {
   private:
    // Возвращает true, если каталог p содержится внутри base_path.
    bool IsSubPath(fs::path path, fs::path base);
    std::string DecodeUrl(std::string_view encoded_url);
    std::string_view GetMimeType(std::string_view extension);
    fs::path& root_;

   public:
    explicit FileHandler(fs::path& root) : root_(root) {}
    std::variant<StringResponse, FileResponse> FileRequestHandler(const StringRequest& request);
};

}  // namespace http_handler