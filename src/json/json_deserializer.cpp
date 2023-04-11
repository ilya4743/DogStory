#include "json_deserializer.h"

#include <boost/json.hpp>
#include <fstream>

#include "json_key.h"
#include "model_constants.h"

using namespace std::literals;

namespace json = boost::json;
namespace sys = boost::system;

namespace json_deserializer {

std::string ReadFile(const std::filesystem::path& json_path) {
    std::ifstream stream{json_path};
    if (!stream) {
        throw std::runtime_error("Failed to open file "s + json_path.string());
    }

    std::stringstream data;

    if (!(data << stream.rdbuf())) {
        throw std::runtime_error("Failed to read file content");
    }
    return data.str();
}

model::Point MakePoint(int x, int y) {
    return model::Point{x, y};
}

model::Offset MakeOffset(int offsetX, int offsetY) {
    return model::Offset{offsetX, offsetY};
}

model::Rectangle MakeRectangle(int x, int y, int w, int h) {
    return model::Rectangle{MakePoint(x, y), w, h};
}

template <typename TagOrientation>
model::Road MakeRoad(TagOrientation orientation, int x0, int y0, int xy1) {
    return model::Road{orientation, MakePoint(x0, y0), xy1};
}

model::Building MakeBuilding(int x, int y, int w, int h) {
    return model::Building{MakeRectangle(x, y, w, h)};
}

model::Office MakeOffice(std::string id, int x, int y, int offsetX, int offsetY) {
    return model::Office(model::Office::Id{id}, MakePoint(x, y), MakeOffset(offsetX, offsetY), model::DEFAULT_OFFICE_WIDTH);
}

model::Road ExtractRoad(json::value road) {
    auto x0 = road.at(Key::X0).as_int64();
    auto y0 = road.at(Key::Y0).as_int64();

    if (road.as_object().contains(Key::X1)) {
        auto x1 = road.at(Key::X1).as_int64();
        return MakeRoad(model::Road::HORIZONTAL, x0, y0, x1);
    } else {
        auto y1 = road.at(Key::Y1).as_int64();
        return MakeRoad(model::Road::VERTICAL, x0, y0, y1);
    }
}

model::Building ExtractBuilding(json::value building) {
    auto x = building.at(Key::X).as_int64();
    auto y = building.at(Key::Y).as_int64();
    auto w = building.at(Key::WIDTH).as_int64();
    auto h = building.at(Key::HEIGHT).as_int64();
    return MakeBuilding(x, y, w, h);
}

model::Office ExtractOffice(json::value office) {
    std::string id = office.at(Key::ID).as_string().c_str();
    auto x = office.at(Key::X).as_int64();
    auto y = office.at(Key::Y).as_int64();
    auto offsetX = office.at(Key::OFFSET_X).as_int64();
    auto offsetY = office.at(Key::OFFSET_Y).as_int64();
    return MakeOffice(id, x, y, offsetX, offsetY);
}

model::LootType MakeLootType(std::string name, std::string file, std::string type,
                             int rotation, std::string color, double scale, int64_t value) {
    return model::LootType{name, file, type, rotation, color, scale, value};
}

model::LootType ExtractLootType(json::value loot_type) {
    std::string name = loot_type.as_object().at(Key::NAME).as_string().c_str();
    std::string file = loot_type.as_object().at(Key::FILE).as_string().c_str();
    std::string type = loot_type.as_object().at(Key::TYPE).as_string().c_str();
    int rotation = INT_MIN;
    if (loot_type.as_object().contains(Key::ROTATION))
        rotation = loot_type.as_object().at(Key::ROTATION).as_int64();
    std::string color;
    if (loot_type.as_object().contains(Key::COLOR))
        color = loot_type.as_object().at(Key::COLOR).as_string().c_str();
    auto scale = loot_type.as_object().at(Key::SCALE).as_double();
    int64_t value = loot_type.as_object().at(Key::VALUE).as_int64();
    return MakeLootType(name, file, type, rotation, color, scale, value);
}

model::Map MakeMap(std::string id, std::string name, double dog_speed, uint64_t bag_capacity) {
    return model::Map{model::Map::Id{id}, name, dog_speed, bag_capacity};
}

model::Map ExtractMap(json::value map, double dog_speed, uint64_t bag_capacity) {
    auto id = map.at(Key::ID).as_string().c_str();
    auto name = map.at(Key::NAME).as_string().c_str();
    if (map.as_object().contains(Key::DOG_SPEED))
        dog_speed = map.at(Key::DOG_SPEED).as_double();

    if (map.as_object().contains(Key::BAG_CAPACITY))
        bag_capacity = map.at(Key::BAG_CAPACITY).as_uint64();

    model::Map out = MakeMap(id, name, dog_speed, bag_capacity);

    for (auto it = map.at(Key::LOOT_TYPES).as_array().begin();
         it != map.at(Key::LOOT_TYPES).as_array().end(); ++it)
        out.AddLootType(ExtractLootType(*it));

    for (auto it = map.at(Key::ROADS).as_array().begin();
         it != map.at(Key::ROADS).as_array().end(); ++it)
        out.AddRoad(ExtractRoad(*it));

    for (auto it = map.at(Key::BUILDINGS).as_array().begin();
         it != map.at(Key::BUILDINGS).as_array().end(); ++it)
        out.AddBuilding(ExtractBuilding(*it));

    for (auto it = map.at(Key::OFFICES).as_array().begin();
         it != map.at(Key::OFFICES).as_array().end(); ++it)
        out.AddOffice(ExtractOffice(*it));

    return out;
}

MoveAction ExtractMoveAction(std::string data) {
    auto object = json::parse(data).as_object();
    auto value = object.at("move"s).as_string();
    if (value.empty())
        return MoveAction{0};
    else
        return MoveAction{value.c_str()[0]};
}

std::pair<std::string, std::string> ExtractJoinGameDataFromRequest(std::string data) {
    auto value = json::parse(data);
    return std::make_pair((std::string)value.at(Key::USER_NAME).as_string(),
                          (std::string)value.at(Key::MAP_ID).as_string());
}

std::chrono::milliseconds ExtractDeltaTime(std::string data) {
    auto value = json::parse(data);
    return std::chrono::milliseconds{value.as_object().at(Key::TIME_DELTA).as_int64()};
}

model::LootGeneratorConfig MakeLootGeneratorConfig(double period, double probability) {
    return model::LootGeneratorConfig{period, probability};
}

model::LootGeneratorConfig ExtractLootGeneratorConfig(json::value loot_generator_config) {
    auto period = loot_generator_config.as_object().at(Key::PERIOD).as_double();
    auto probability = loot_generator_config.as_object().at(Key::PROBABILITY).as_double();
    return MakeLootGeneratorConfig(period, probability);
}

model::Game LoadGame(const std::filesystem::path& json_path) {
    // Загрузить содержимое файла json_path, например, в виде строки
    // Распарсить строку как JSON, используя boost::json::parse
    // Загрузить модель игры из файла
    model::Game game;
    auto data = ReadFile(json_path);
    auto value = json::parse(data);
    double dog_speed = model::DEFAULT_DOG_SPEED;
    if (value.as_object().contains(Key::DEFAULT_DOG_SPEED))
        dog_speed = value.as_object().at(Key::DEFAULT_DOG_SPEED).as_double();

    uint64_t bag_capacity = model::DEFAULT_BAG_CAPACITY;
    if (value.as_object().contains(Key::DEFAULT_BAG_CAPACITY))
        bag_capacity = value.as_object().at(Key::DEFAULT_BAG_CAPACITY).as_uint64();

    if (value.as_object().contains(Key::DOG_RETIREMENT_TIME))
        model::Dog::SetMaxInactiveTime(boost::json::value_to<size_t>(value.as_object().at(Key::DOG_RETIREMENT_TIME)));

    auto loot_generator_config = ExtractLootGeneratorConfig(value.as_object().at(Key::LOOT_GENERATOR_CONFIG));
    game.SetLootGeneratorConfig(loot_generator_config);
    for (auto it = value.as_object().at(Key::MAPS).as_array().begin();
         it != value.as_object().at(Key::MAPS).as_array().end(); ++it)
        game.AddMap(ExtractMap(*it, dog_speed, bag_capacity));

    return game;
}

}  // namespace json_deserializer