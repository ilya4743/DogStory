#pragma once
#include <chrono>
#include <iostream>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

#include "collision_detector.h"
#include "geom.h"
#include "loot_generator.h"
#include "model_constants.h"
#include "tagged.h"

namespace model {

using Dimension = int;
using Coord = Dimension;

struct Point {
    Coord x, y;
};

struct Size {
    Dimension width, height;
};

struct Rectangle {
    Point position;
    Size size;
};

struct Offset {
    Dimension dx, dy;
};

class Road {
    struct HorizontalTag {
        explicit HorizontalTag() = default;
    };

    struct VerticalTag {
        explicit VerticalTag() = default;
    };

   public:
    constexpr static HorizontalTag HORIZONTAL{};
    constexpr static VerticalTag VERTICAL{};

    Road(HorizontalTag, Point start, Coord end_x) noexcept;
    Road(VerticalTag, Point start, Coord end_y) noexcept;
    bool IsHorizontal() const noexcept;
    bool IsVertical() const noexcept;
    Point GetStart() const noexcept;
    Point GetEnd() const noexcept;
    Road Normalized() const noexcept;

   private:
    Point start_;
    Point end_;
};

class Building {
   public:
    explicit Building(Rectangle bounds) noexcept;
    const Rectangle& GetBounds() const noexcept;

   private:
    Rectangle bounds_;
};

class Office : public collision_detector::Item {
   public:
    using Id = util::Tagged<std::string, Office>;

    Office(Id id, model::Point position, Offset offset, double width) noexcept;
    const Id& GetId() const noexcept;
    Offset GetOffset() const noexcept;

   private:
    Id id_;
    Offset offset_;
};

struct LootType {
    std::string name;
    std::string file;
    std::string type;
    int rotation;
    std::string color;
    double scale;
    int64_t value;
};

class Map {
   public:
    using Id = util::Tagged<std::string, Map>;
    using Roads = std::vector<Road>;
    using Buildings = std::vector<Building>;
    using Offices = std::vector<Office>;
    using LootTypes = std::vector<LootType>;

    Map(Id id, std::string name, double dog_speed, uint64_t bag_capacity) noexcept;
    const Id& GetId() const noexcept;
    const std::string& GetName() const noexcept;
    const Buildings& GetBuildings() const noexcept;
    const Roads& GetRoads() const noexcept;
    const Offices& GetOffices() const noexcept;
    const double& GetDogSpeed() const noexcept;
    const LootTypes& GetLootTypes() const noexcept;
    const size_t GetLootTypesSize() const noexcept;
    const int64_t& GetLootTypeValue(uint64_t index) const noexcept;

    const uint64_t& GetBagCapacity() const noexcept;
    void AddRoad(const Road& road);
    void AddBuilding(const Building& building);
    void AddOffice(Office office);
    void AddLootType(LootType loot_type);

   private:
    using OfficeIdToIndex = std::unordered_map<Office::Id, size_t, util::TaggedHasher<Office::Id>>;

    Id id_;
    std::string name_;
    Roads roads_;
    Buildings buildings_;
    Offices offices_;
    OfficeIdToIndex warehouse_id_to_index_;
    double dog_speed_;
    uint64_t bag_capacity_;
    LootTypes loot_types_;
};

enum class Direction {
    NORTH = 'U',
    EAST = 'R',
    WEST = 'L',
    SOUTH = 'D',
    STOP
};

class LostObject;
using Score = unsigned;
using ObjectType = unsigned;

struct FoundObject {
    using Id = util::Tagged<uint32_t, FoundObject>;

    Id id{0u};
    ObjectType type{0u};
    int64_t value{0};
    [[nodiscard]] auto operator<=>(const FoundObject&) const = default;
};

class Dog {
    inline static std::chrono::seconds max_inactive_time_{ONE_MINUTE_IN_SECONDS};

   public:
    using Id = util::Tagged<uint32_t, Dog>;
    using Bag = std::vector<FoundObject>;

    Dog(Id id, std::string name, geom::Point2D position, uint64_t bag_capacity);
    const Id& GetId() const noexcept;
    const std::string GetName() const noexcept;
    const geom::Point2D& GetPosition() const noexcept;
    const geom::Vec2D& GetSpeed() const noexcept;
    const Bag& GetBag() const noexcept;
    const Score& GetScore() const noexcept;
    Direction GetDirection() const noexcept;
    const collision_detector::Gatherer& GetGatherer() const;
    const size_t& GetBagCapacity() const noexcept;
    void SetSpeed(geom::Vec2D speed) noexcept;
    void SetPosition(geom::Point2D position) noexcept;
    void SetDirection(Direction direction) noexcept;
    void SetGatherer() noexcept;
    void SetScore(size_t score) noexcept;
    bool isFullBag() const noexcept;
    bool isEmptyBag() const noexcept;
    [[nodiscard]] bool AddItemToBag(FoundObject item);
    void ClearBag();
    std::optional<std::chrono::seconds> GetPlayTime();
    void UpdatePlayTime(const std::chrono::milliseconds& delta_time);
    void UpdateInactiveTime(const std::chrono::milliseconds& delta_time);
    void SetActive(bool active);
    bool IsActive() noexcept;
    static void SetMaxInactiveTime(size_t max_inactive_time_in_seconds);

   private:
    Id id_;
    std::string name_;
    geom::Point2D position_;
    geom::Vec2D speed_;
    Direction direction_{Direction::NORTH};
    collision_detector::Gatherer gatherer_;
    size_t bag_capacity_;
    Bag bag;
    Score score_;
    std::chrono::milliseconds inactive_time_;
    std::chrono::milliseconds live_time_;
    bool is_active_{true};
};

class RoadIndex {
   public:
    struct Roads {
        std::optional<Road> horizontal;
        std::optional<Road> vertical;
    };

    explicit RoadIndex(const Map::Roads& roads);

    Roads GetRoadsAtPoint(Point pt) const noexcept;

   private:
    using RoadFragment = std::pair<Coord, Coord>;
    using RoadFragments = std::vector<RoadFragment>;
    using RoadFragmentsMap = std::unordered_map<Coord, RoadFragments>;

    static void MergeFragments(RoadFragmentsMap& fragments_map);

    RoadFragmentsMap horizontal_roads_;
    RoadFragmentsMap vertical_roads_;
};

bool IsValueInRange(double value, double min_value, double max_value);

class LostObject : public collision_detector::Item {
   public:
    using Id = util::Tagged<uint32_t, LostObject>;

    LostObject(Id id, size_t type, geom::Point2D position, size_t value);
    const Id& GetId() const noexcept;
    const ObjectType& GetType() const noexcept;
    const size_t& GetValue() const noexcept;

   private:
    Id id_;
    ObjectType type_;
    size_t value_;
};

struct LootGeneratorConfig {
    double period;
    double probability;
};

class Game {
   public:
    using Maps = std::vector<std::shared_ptr<Map>>;

    void AddMap(Map map);
    const Maps& GetMaps() const noexcept;
    const std::shared_ptr<Map> FindMap(const Map::Id& id) const noexcept;
    void SetLootGeneratorConfig(LootGeneratorConfig generator);
    const LootGeneratorConfig& GetLootGeneratorConfig() const noexcept;

   private:
    using MapIdHasher = util::TaggedHasher<Map::Id>;
    using MapIdToIndex = std::unordered_map<Map::Id, size_t, MapIdHasher>;

    LootGeneratorConfig loot_generator_config_;
    Maps maps_;
    MapIdToIndex map_id_to_index_;
};

}  // namespace model
