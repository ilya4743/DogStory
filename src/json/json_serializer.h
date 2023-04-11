#pragma once

#include <string>

#include "model.h"
#include "use_cases.h"

namespace json_serializer {
std::string Serialize(const model::Map& map);
std::string Serialize(const model::Game::Maps& maps);
std::string SerializeListOfMaps(const ListMapsUseCase::Maps& maps);
std::string JoinGame(std::string authToken, std::string playerId);
std::string SerializeListOfPlayers(std::vector<PlayerInfo> players);
std::string SerializeGameState(GameState game_state);
std::string SerializeRecords(RecordUseCase::Records records);

std::string ErrorMsg(std::string code, std::string message);
}  // namespace json_serializer