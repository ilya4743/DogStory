#pragma once

#include <chrono>
#include <filesystem>

#include "model.h"
#include "player.h"

namespace json_deserializer {

std::chrono::milliseconds ExtractDeltaTime(std::string data);
MoveAction ExtractMoveAction(std::string data);
std::pair<std::string, std::string> ExtractJoinGameDataFromRequest(std::string data);
model::Game LoadGame(const std::filesystem::path& json_path);

}  // namespace json_deserializer
