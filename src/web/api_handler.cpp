#include "api_handler.h"

#include <boost/url/parse.hpp>

#include "json_deserializer.h"
#include "json_serializer.h"

namespace http_handler {

ApiHandler::ApiHandler(std::shared_ptr<Application> app) : app_{app} {
}

bool ApiHandler::isApiRequest(const StringRequest& request) {
    auto target = request.target();
    return target.starts_with(API::IS_API);
};

StringResponse ApiHandler::ApiHandlerRequest(const StringRequest& request) {
    request_ = std::move(request);
    auto target = request_.target();
    if (target == API::MAPS)
        return ListOfMaps();

    if (target.starts_with(API::MAP_ID))
        return GetMap();

    if (target == API::JOIN_GAME)
        return JoinGame();

    if (target == API::LIST_PLAYERS)
        return ListOfPlayers();

    if (target == API::GAME_STATE)
        return GetGameState();

    if (target == API::PLAYER_ACTION)
        return GetPlayerAction();

    if (target == API::TICK)
        return Tick();

    if (target.starts_with(API::RECORD))
        return Record();

    auto response = json_serializer::ErrorMsg("badRequest", "Bad request");
    return MakeStringResponse(http::status::bad_request, std::string_view{response}, request.version(), request.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv);
}

StringResponse ApiHandler::ListOfMaps() {
    if (request_.method() == http::verb::get || request_.method() == http::verb::head) {
        const auto& maps = app_->ListMaps();
        auto response = json_serializer::SerializeListOfMaps(maps);
        return MakeStringResponse(http::status::ok, std::string_view{response}, request_.version(), request_.keep_alive(),
                                  ContentType::APPLICATION_JSON, "no-cache"sv);
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Invalid method");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
}

StringResponse ApiHandler::GetMap() {
    if (request_.method() == http::verb::get || request_.method() == http::verb::head) {
        std::string id = std::string{request_.target().substr(API::MAP_ID.size(), request_.target().size())};
        const auto map = app_->FindMap(id);
        if (map) {
            auto response = json_serializer::Serialize(*map);
            return MakeStringResponse(http::status::ok, std::string_view{response}, request_.version(), request_.keep_alive(),
                                      ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
        } else {
            auto response = json_serializer::ErrorMsg("mapNotFound", "Map not found");
            return MakeStringResponse(http::status::not_found, std::string_view{response}, request_.version(), request_.keep_alive(),
                                      ContentType::APPLICATION_JSON, "no-cache"sv);
        }
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Invalid method");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
}

StringResponse ApiHandler::JoinGame() {
    if (request_.method() == http::verb::post) {
        auto it = request_.find(http::field::content_type);
        if (it == request_.end() || beast::iequals(it->value(), ContentType::APPLICATION_JSON)) {
            std::pair<std::string, std::string> data_join;
            try {
                data_join = json_deserializer::ExtractJoinGameDataFromRequest(request_.body());
            } catch (const std::exception& e) {
                auto response = json_serializer::ErrorMsg("invalidArgument", "Join game request parse error");
                return MakeStringResponse(http::status::bad_request, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
            if (data_join.first.empty()) {
                auto response = json_serializer::ErrorMsg("invalidArgument", "Invalid name");
                return MakeStringResponse(http::status::bad_request, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
            auto join_data = app_->JoinGame(data_join.second, data_join.first);
            if (join_data.first.empty() && join_data.second.empty()) {
                auto response = json_serializer::ErrorMsg("mapNotFound", "Map not found");
                return MakeStringResponse(http::status::not_found, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
            auto response = json_serializer::JoinGame(join_data.first, join_data.second);
            return MakeStringResponse(http::status::ok, std::string_view{response}, request_.version(), request_.keep_alive(),
                                      ContentType::APPLICATION_JSON, "no-cache"sv);
        }
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Only POST method is expected");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "POST"sv);
}

StringResponse ApiHandler::ListOfPlayers() {
    if (request_.method() == http::verb::get || request_.method() == http::verb::head) {
        auto it = request_.find(http::field::authorization);
        if (it == request_.end()) {
            auto response = json_serializer::ErrorMsg("invalidToken", "Authorization header is missing");
            return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                      ContentType::APPLICATION_JSON, "no-cache"sv);
        }
        auto value = it->value();
        if (value.starts_with(TOKEN_BEARER) && value.size() == BEARER_AUTHORIZATION_TOKEN_SIZE) {
            std::string token_str{value.begin() + TOKEN_BEARER_SIZE, value.begin() + BEARER_AUTHORIZATION_TOKEN_SIZE};
            Token token(std::move(token_str));
            auto players = app_->ListPlayers(token);
            if (players.size() != 0) {
                auto response = json_serializer::SerializeListOfPlayers(players);
                return MakeStringResponse(http::status::ok, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            } else {
                auto response = json_serializer::ErrorMsg("unknownToken", "Player token has not been found");
                return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
        } else {
            auto response = json_serializer::ErrorMsg("invalidToken", "Authorization header is missing");
            return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                      ContentType::APPLICATION_JSON, "no-cache"sv);
        }
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Invalid method");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
}

StringResponse ApiHandler::GetGameState() {
    if (request_.method() == http::verb::get || request_.method() == http::verb::head) {
        auto it = request_.find(http::field::authorization);
        if (it == request_.end()) {
            auto response = json_serializer::ErrorMsg("invalidToken", "Authorization header is required");
            return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                      ContentType::APPLICATION_JSON, "no-cache"sv);
        }
        auto value = it->value();
        if (value.starts_with(TOKEN_BEARER)) {
            if (value.size() != BEARER_AUTHORIZATION_TOKEN_SIZE) {
                auto response = json_serializer::ErrorMsg("invalidToken", "Authorization header is required");
                return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
            std::string token_str{value.begin() + TOKEN_BEARER_SIZE, value.begin() + BEARER_AUTHORIZATION_TOKEN_SIZE};
            Token token(std::move(token_str));
            auto game_state = app_->GetGameState(token);
            if (game_state.players.size() != 0) {
                auto response = json_serializer::SerializeGameState(game_state);
                return MakeStringResponse(http::status::ok, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            } else {
                auto response = json_serializer::ErrorMsg("unknownToken", "Player token has not been found");
                return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
        }
        auto response = json_serializer::ErrorMsg("invalidMethod", "Only POST method is expected");
        return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                                  ContentType::APPLICATION_JSON, "no-cache"sv, "POST"sv);
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Invalid method");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
}

StringResponse ApiHandler::GetPlayerAction() {
    if (request_.method() == http::verb::post) {
        auto it = request_.find(http::field::authorization);
        if (it == request_.end()) {
            auto response = json_serializer::ErrorMsg("invalidToken", "Authorization header is required");
            return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                      ContentType::APPLICATION_JSON, "no-cache"sv);
        }
        auto value = it->value();
        if (value.starts_with(TOKEN_BEARER)) {
            if (value.size() != BEARER_AUTHORIZATION_TOKEN_SIZE) {
                auto response = json_serializer::ErrorMsg("invalidToken", "Authorization header is required");
                return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
            std::string token_str{value.begin() + TOKEN_BEARER_SIZE, value.begin() + BEARER_AUTHORIZATION_TOKEN_SIZE};
            Token token(std::move(token_str));
            auto game_state = app_->GetGameState(token);
            if (game_state.players.size() != 0) {
                auto direction = json_deserializer::ExtractMoveAction(request_.body());
                app_->MovePlayer(token, direction);
                return MakeStringResponse(http::status::ok, "{}"sv, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            } else {
                auto response = json_serializer::ErrorMsg("unknownToken", "Player token has not been found");
                return MakeStringResponse(http::status::unauthorized, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
        }
        auto response = json_serializer::ErrorMsg("invalidMethod", "Only POST method is expected");
        return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                                  ContentType::APPLICATION_JSON, "no-cache"sv, "POST"sv);
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Invalid method");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
}

StringResponse ApiHandler::Tick() {
    if (request_.method() == http::verb::post) {
        auto it = request_.find(http::field::content_type);
        if (it == request_.end() || beast::iequals(it->value(), ContentType::APPLICATION_JSON)) {
            std::chrono::milliseconds delta_time{0};
            try {
                delta_time = json_deserializer::ExtractDeltaTime(request_.body());
                app_->Tick(delta_time);
                return MakeStringResponse(http::status::ok, "{}"sv, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            } catch (const std::exception& e) {
                auto response = json_serializer::ErrorMsg("invalidArgument", "Failed to parse tick request JSON");
                return MakeStringResponse(http::status::bad_request, std::string_view{response}, request_.version(), request_.keep_alive(),
                                          ContentType::APPLICATION_JSON, "no-cache"sv);
            }
        }
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Invalid method");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
}

namespace url_invariants {

const std::string URL_PARAMETER_START = "start";
const std::string URL_PARAMETER_MAX_ITEMS = "maxItems";

}  // namespace url_invariants

StringResponse ApiHandler::Record() {
    if (request_.method() == http::verb::get) {
        std::optional<size_t> offset;
        std::optional<size_t> limit;
        auto params = boost::urls::url_view{request_.target()}.params();

        if (params.contains(url_invariants::URL_PARAMETER_START)) {
            offset = GetValueFromUrlParameter<size_t>(params, url_invariants::URL_PARAMETER_START);
        }

        if (params.contains(url_invariants::URL_PARAMETER_MAX_ITEMS)) {
            limit = GetValueFromUrlParameter<size_t>(params, url_invariants::URL_PARAMETER_MAX_ITEMS);
        }
        auto records = app_->GetRecords(offset, limit);
        auto response = json_serializer::SerializeRecords(records.value());
        return MakeStringResponse(http::status::ok, std::string_view{response}, request_.version(), request_.keep_alive(),
                                  ContentType::APPLICATION_JSON, "no-cache"sv);
    }
    auto response = json_serializer::ErrorMsg("invalidMethod", "Invalid method");
    return MakeStringResponse(http::status::method_not_allowed, std::string_view{response}, request_.version(), request_.keep_alive(),
                              ContentType::APPLICATION_JSON, "no-cache"sv, "GET, HEAD"sv);
}

}  // namespace http_handler