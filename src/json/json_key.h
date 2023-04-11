#pragma once

struct Key {
    Key() = delete;
    constexpr static auto MAPS{"maps"};
    constexpr static auto ID{"id"};
    constexpr static auto NAME{"name"};
    constexpr static auto BUILDINGS{"buildings"};
    constexpr static auto X0{"x0"};
    constexpr static auto X1{"x1"};
    constexpr static auto Y0{"y0"};
    constexpr static auto Y1{"y1"};
    constexpr static auto ROADS{"roads"};
    constexpr static auto X{"x"};
    constexpr static auto Y{"y"};
    constexpr static auto WIDTH{"w"};
    constexpr static auto HEIGHT{"h"};
    constexpr static auto OFFSET_X{"offsetX"};
    constexpr static auto OFFSET_Y{"offsetY"};
    constexpr static auto OFFICES{"offices"};
    constexpr static auto DOG_SPEED{"dogSpeed"};
    constexpr static auto DEFAULT_DOG_SPEED{"defaultDogSpeed"};

    constexpr static auto USER_NAME{"userName"};
    constexpr static auto MAP_ID{"mapId"};
    constexpr static auto AUTH_TOKEN{"authToken"};
    constexpr static auto PLAYER_ID{"playerId"};
    constexpr static auto PLAYERS{"players"};
    constexpr static auto POSITION{"pos"};
    constexpr static auto SPEED{"speed"};
    constexpr static auto DIRECTION{"dir"};
    constexpr static auto MOVE{"move"};
    constexpr static auto TIME_DELTA{"timeDelta"};
    constexpr static auto LOOT_GENERATOR_CONFIG{"lootGeneratorConfig"};
    constexpr static auto PERIOD{"period"};
    constexpr static auto PROBABILITY{"probability"};
    constexpr static auto LOOT_TYPES{"lootTypes"};
    constexpr static auto FILE{"file"};
    constexpr static auto TYPE{"type"};
    constexpr static auto ROTATION{"rotation"};
    constexpr static auto COLOR{"color"};
    constexpr static auto SCALE{"scale"};
    constexpr static auto LOST_OBJECTS{"lostObjects"};
    constexpr static auto DEFAULT_BAG_CAPACITY{"defaultBagCapacity"};
    constexpr static auto BAG_CAPACITY{"bagCapacity"};
    constexpr static auto BAG{"bag"};
    constexpr static auto VALUE{"value"};
    constexpr static auto SCORE{"score"};
    constexpr static auto PLAY_TIME{"playTime"};
    constexpr static auto DOG_RETIREMENT_TIME{"dogRetirementTime"};
};

struct ErrorKey {
    ErrorKey() = delete;
    constexpr static auto CODE{"code"};
    constexpr static auto MESSAGE{"message"};
};