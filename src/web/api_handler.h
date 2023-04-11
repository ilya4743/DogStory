#pragma once
#include <boost/url/params_view.hpp>
#include <string_view>

#include "application.h"
#include "model.h"
#include "response.h"

namespace http_handler {

namespace beast = boost::beast;
namespace http = beast::http;
using StringResponse = http::response<http::string_body>;
using StringRequest = http::request<http::string_body>;

template <typename T>
T GetValueFromUrlParameter(const boost::urls::params_view& params, const std::string& key) {
    std::stringstream ss;
    ss << (*params.find(key)).value;
    T res;
    ss >> res;
    return res;
};

struct API {
    API() = delete;
    constexpr static std::string_view IS_API{"/api/"};
    constexpr static std::string_view MAPS{"/api/v1/maps"};
    constexpr static std::string_view MAP_ID{"/api/v1/maps/"};
    constexpr static std::string_view JOIN_GAME{"/api/v1/game/join"};
    constexpr static std::string_view LIST_PLAYERS{"/api/v1/game/players"};
    constexpr static std::string_view GAME_STATE{"/api/v1/game/state"};
    constexpr static std::string_view PLAYER_ACTION{"/api/v1/game/player/action"};
    constexpr static std::string_view TICK{"/api/v1/game/tick"};
    constexpr static std::string_view RECORD{"/api/v1/game/records"};
};

class ApiHandler {
   public:
    explicit ApiHandler(std::shared_ptr<Application> app);
    bool isApiRequest(const StringRequest& request);
    StringResponse ApiHandlerRequest(const StringRequest& request);

   private:
    StringResponse ListOfMaps();
    StringResponse GetMap();
    StringResponse JoinGame();
    StringResponse ListOfPlayers();
    StringResponse GetGameState();
    StringResponse GetPlayerAction();
    StringResponse Tick();
    StringResponse Record();

    std::shared_ptr<Application> app_;
    StringRequest request_;
    constexpr static auto TOKEN_BEARER = "Bearer"sv;
    constexpr static auto TOKEN_BEARER_SIZE = TOKEN_BEARER.size() + 1;
    constexpr static auto BEARER_AUTHORIZATION_TOKEN_SIZE = TOKEN_BEARER_SIZE + 32;
};

}  // namespace http_handler