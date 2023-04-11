#include "json_serializer.h"

#include <boost/json.hpp>

#include "json_key.h"

namespace json = boost::json;

namespace json_serializer {
json::object SerializeBuilding(const model::Building& building) {
    const auto& b = building.GetBounds();
    return {
        {Key::X, b.position.x},
        {Key::Y, b.position.y},
        {Key::WIDTH, b.size.width},
        {Key::HEIGHT, b.size.height},
    };
}

json::array SerializeBuildings(const model::Map::Buildings& buildings) {
    json::array out;
    out.reserve(buildings.size());
    for (auto it = buildings.begin(); it != buildings.end(); ++it)
        out.push_back(SerializeBuilding(*it));
    return out;
}

json::object SerializeOffice(const model::Office& office) {
    const auto& id = *office.GetId();
    const auto& position = office.GetPosition();
    const auto& offset = office.GetOffset();
    return {
        {Key::ID, id},
        {Key::X, position.x},
        {Key::Y, position.y},
        {Key::OFFSET_X, offset.dx},
        {Key::OFFSET_Y, offset.dy},
    };
}

json::array SerializeOffices(const model::Map::Offices& offices) {
    json::array out;
    out.reserve(offices.size());
    for (auto it = offices.begin(); it != offices.end(); ++it)
        out.push_back(SerializeOffice(*it));
    return out;
}

json::object SerializeRoad(const model::Road& road) {
    const auto& start = road.GetStart();
    const auto& end = road.GetEnd();
    if (road.IsHorizontal())
        return {
            {Key::X0, start.x},
            {Key::Y0, start.y},
            {Key::X1, end.x},
        };
    else
        return {
            {Key::X0, start.x},
            {Key::Y0, start.y},
            {Key::Y1, end.y},
        };
}

json::array SerializeRoads(const model::Map::Roads& roads) {
    json::array out;
    out.reserve(roads.size());
    for (auto it = roads.begin(); it != roads.end(); ++it)
        out.push_back(SerializeRoad(*it));
    return out;
}

json::object SerializeLootType(const model::LootType& loot_type) {
    const auto& name = loot_type.name;
    const auto& file = loot_type.file;
    const auto& type = loot_type.type;
    const auto& rotation = loot_type.rotation;
    const auto& color = loot_type.color;
    const auto& scale = loot_type.scale;
    const auto& value = loot_type.value;

    if (color.empty() && rotation == INT_MIN)
        return {
            {Key::NAME, name},
            {Key::FILE, file},
            {Key::TYPE, type},
            {Key::SCALE, scale},
            {Key::VALUE, value},
        };
    else
        return {
            {Key::NAME, name},
            {Key::FILE, file},
            {Key::TYPE, type},
            {Key::ROTATION, rotation},
            {Key::COLOR, color},
            {Key::SCALE, scale},
            {Key::VALUE, value},
        };
}

json::array SerializeLootTypes(const model::Map::LootTypes& loot_types) {
    json::array out;
    out.reserve(loot_types.size());
    for (auto it = loot_types.begin(); it != loot_types.end(); ++it)
        out.push_back(SerializeLootType(*it));
    return out;
}

json::object SerializeMap(const model::Map& map) {
    const auto& id = *map.GetId();
    const auto& name = map.GetName();
    const auto& loot_types = map.GetLootTypes();
    const auto& roads = map.GetRoads();
    const auto& buildings = map.GetBuildings();
    const auto& offices = map.GetOffices();
    return {
        {Key::ID, id},
        {Key::NAME, name},
        {Key::LOOT_TYPES, SerializeLootTypes(loot_types)},
        {Key::ROADS, SerializeRoads(roads)},
        {Key::BUILDINGS, SerializeBuildings(buildings)},
        {Key::OFFICES, SerializeOffices(offices)},
    };
}

json::array SerializeMaps(const model::Game::Maps& maps) {
    json::array out;
    out.reserve(maps.size());
    for (auto it = maps.begin(); it != maps.end(); ++it)
        out.push_back(SerializeMap(*(*it)));
    return out;
}

std::string Serialize(const model::Map& map) {
    return json::serialize(SerializeMap(map));
}

std::string Serialize(const model::Game::Maps& maps) {
    return json::serialize(SerializeMaps(maps));
}

std::string SerializeListOfMaps(const ListMapsUseCase::Maps& maps) {
    json::array out_json;
    for (auto it = maps.begin(); it != maps.end(); ++it)
        out_json.emplace_back(json::value{{Key::ID, (*it).id}, {Key::NAME, (*it).name}});
    return json::serialize(std::move(out_json));
}

std::string JoinGame(std::string authToken, std::string playerId) {
    return json::serialize(json::object{
        {Key::AUTH_TOKEN, std::move(authToken)},
        {Key::PLAYER_ID, std::stoi(std::move(playerId))},
    });
}

std::string SerializeListOfPlayers(std::vector<PlayerInfo> players) {
    json::object out_json;
    for (auto it = players.begin(); it != players.end(); ++it)
        out_json.emplace(std::move(it->id),
                         json::object{{std::move(Key::NAME), std::move(it->name)}});
    return json::serialize(out_json);
}

json::array SerializePlayersBag(const PlayerState::Bag& bag) {
    json::array bags_json;

    for (const auto& item : bag) {
        bags_json.push_back(
            {
                {Key::ID, *item.id},
                {Key::TYPE, item.type},
            });
    }
    return bags_json;
}

json::value SerializePlayersStates(const GameState::PlayersStates& players) {
    json::object players_json;
    for (const auto& player : players) {
        players_json.insert_or_assign(
            std::to_string(*player.id),
            json::object{
                {Key::POSITION, json::array{player.position.x, player.position.y}},
                {Key::SPEED, json::array{player.speed.x, player.speed.y}},
                {Key::DIRECTION, std::string{static_cast<char>(player.direction)}},
                {Key::BAG, SerializePlayersBag(player.bag)},
                {Key::SCORE, player.score},
            });
    }
    return players_json;
}

json::value SerializeLostObjectsStates(const GameState::LostObjectsStates& lost_objects) {
    json::object lost_objects_json;
    for (const auto& lost_object : lost_objects) {
        lost_objects_json.insert_or_assign(
            std::to_string(*lost_object.id),
            json::object{
                {Key::TYPE, lost_object.type},
                {Key::POSITION, json::array{lost_object.position.x, lost_object.position.y}}});
    }
    return lost_objects_json;
}

std::string SerializeGameState(GameState game_state) {
    std::string test = json::serialize(json::object{
        {Key::PLAYERS, SerializePlayersStates(game_state.players)},
        {Key::LOST_OBJECTS, SerializeLostObjectsStates(game_state.lost_objects)}});
    return json::serialize(json::object{
        {Key::PLAYERS, SerializePlayersStates(game_state.players)},
        {Key::LOST_OBJECTS, SerializeLostObjectsStates(game_state.lost_objects)}});
}

std::string ErrorMsg(std::string code, std::string message) {
    return json::serialize(json::object{
        {ErrorKey::CODE, std::move(code)},
        {ErrorKey::MESSAGE, std::move(message)},
    });
}

json::object SerializeRecord(Record record) {
    return json::object{
        {Key::NAME, record.name},
        {Key::SCORE, record.score},
        {Key::PLAY_TIME, record.playTime},
    };
}

std::string SerializeRecords(RecordUseCase::Records records) {
    json::array out_json;
    for (auto&& record : records) {
        out_json.push_back(SerializeRecord(record));
    }
    return json::serialize(out_json);
}

}  // namespace json_serializer