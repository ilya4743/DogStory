#include "model.h"

#include <boost/range/algorithm/sort.hpp>
#include <cassert>
#include <cmath>
#include <stdexcept>

#include "item_dog_provider.h"

namespace model {
using namespace std::literals;

Road::Road(HorizontalTag, Point start, Coord end_x) noexcept
    : start_{start}, end_{end_x, start.y} {
}

Road::Road(VerticalTag, Point start, Coord end_y) noexcept
    : start_{start}, end_{start.x, end_y} {
}

bool Road::IsHorizontal() const noexcept {
    return start_.y == end_.y;
}

bool Road::IsVertical() const noexcept {
    return start_.x == end_.x;
}

Point Road::GetStart() const noexcept {
    return start_;
}

Point Road::GetEnd() const noexcept {
    return end_;
}

Road Road::Normalized() const noexcept {
    auto result = *this;
    if (IsHorizontal()) {
        if (result.start_.x > result.end_.x) {
            std::swap(result.start_.x, result.end_.x);
        }
    } else if (result.start_.y > result.end_.y) {
        std::swap(result.start_.y, result.end_.y);
    }
    return result;
}

Building::Building(Rectangle bounds) noexcept
    : bounds_{bounds} {
}

const Rectangle& Building::GetBounds() const noexcept {
    return bounds_;
}

Office::Office(Id id, model::Point position, Offset offset, double width) noexcept
    : Item({position.x, position.y}, width), id_{std::move(id)}, offset_{offset} {
}

const Office::Id& Office::GetId() const noexcept {
    return id_;
}

Offset Office::GetOffset() const noexcept {
    return offset_;
}

Map::Map(Id id, std::string name, double dog_speed, uint64_t bag_capacity) noexcept
    : id_(std::move(id)), name_(std::move(name)), dog_speed_{dog_speed}, bag_capacity_{bag_capacity} {
}

const Map::Id& Map::GetId() const noexcept {
    return id_;
}

const std::string& Map::GetName() const noexcept {
    return name_;
}

const Map::Buildings& Map::GetBuildings() const noexcept {
    return buildings_;
}

const Map::Roads& Map::GetRoads() const noexcept {
    return roads_;
}

const Map::Offices& Map::GetOffices() const noexcept {
    return offices_;
}

const double& Map::GetDogSpeed() const noexcept {
    return dog_speed_;
}

const Map::LootTypes& Map::GetLootTypes() const noexcept {
    return loot_types_;
}

const size_t Map::GetLootTypesSize() const noexcept {
    return loot_types_.size();
}

const int64_t& Map::GetLootTypeValue(uint64_t index) const noexcept {
    return loot_types_[index].value;
}

const uint64_t& Map::GetBagCapacity() const noexcept {
    return bag_capacity_;
}

void Map::AddRoad(const Road& road) {
    roads_.emplace_back(road);
}

void Map::AddBuilding(const Building& building) {
    buildings_.emplace_back(building);
}

Dog::Dog(Id id, std::string name, geom::Point2D position, uint64_t bag_capacity)
    : id_(std::move(id)), name_(std::move(name)), position_(std::move(position)), speed_(), gatherer_{{0.0, 0.0}, {0.0, 0.0}, DEFAULT_DOG_WIDTH}, bag_capacity_{bag_capacity}, score_{0}, inactive_time_{0}, live_time_{0} {
    bag.reserve(bag_capacity_);
}

const Dog::Id& Dog::GetId() const noexcept {
    return id_;
}
const std::string Dog::GetName() const noexcept {
    return name_;
}
const geom::Point2D& Dog::GetPosition() const noexcept {
    return position_;
}

const geom::Vec2D& Dog::GetSpeed() const noexcept {
    return speed_;
}

const Dog::Bag& Dog::GetBag() const noexcept {
    return bag;
}

const Score& Dog::GetScore() const noexcept {
    return score_;
}

void Dog::SetSpeed(geom::Vec2D speed) noexcept {
    speed_ = speed;
    if (speed_ != geom::Vec2D{0, 0}) {
        is_active_ = true;
        inactive_time_ = std::chrono::milliseconds{0};
    } else {
        is_active_ = false;
    }
}

void Dog::SetPosition(geom::Point2D position) noexcept {
    position_ = std::move(position);
}

void Dog::SetDirection(Direction direction) noexcept {
    direction_ = direction;
}

void Dog::SetGatherer() noexcept {
    switch (direction_) {
        case Direction::NORTH:
            gatherer_.start_pos = {position_.x, position_.y - DEFAULT_DOG_HEIGHT};
            gatherer_.end_pos = position_;
            break;
        case Direction::EAST:
            gatherer_.start_pos = {position_.x + DEFAULT_DOG_HEIGHT, position_.y};
            gatherer_.end_pos = position_;
            break;
        case Direction::WEST:
            gatherer_.start_pos = {position_.x - DEFAULT_DOG_HEIGHT, position_.y};
            gatherer_.end_pos = position_;
            break;
        case Direction::SOUTH:
            gatherer_.start_pos = {position_.x, position_.y + DEFAULT_DOG_HEIGHT};
            gatherer_.end_pos = position_;
            break;
    }
}

void Dog::SetScore(size_t score) noexcept {
    score_ = score;
}

Direction Dog::GetDirection() const noexcept {
    return direction_;
}

const collision_detector::Gatherer& Dog::GetGatherer() const {
    return gatherer_;
};

const size_t& Dog::GetBagCapacity() const noexcept {
    return bag_capacity_;
}

bool Dog::isFullBag() const noexcept {
    return bag_capacity_ <= bag.size();
}

bool Dog::isEmptyBag() const noexcept {
    return bag.empty();
}

[[nodiscard]] bool Dog::AddItemToBag(FoundObject item) {
    if (isFullBag())
        return false;
    bag.push_back(std::move(item));
    return true;
}

void Dog::ClearBag() {
    for (int i = 0; i < bag.size(); i++)
        score_ += bag[i].value;
    bag.clear();
}

std::optional<std::chrono::seconds> Dog::GetPlayTime() {
    if (is_active_ || std::chrono::duration_cast<std::chrono::seconds>(inactive_time_) < max_inactive_time_) {
        return std::nullopt;
    }
    return std::chrono::duration_cast<std::chrono::seconds>(live_time_);
};

void Dog::UpdatePlayTime(const std::chrono::milliseconds& delta_time) {
    live_time_ += delta_time;
}

void Dog::UpdateInactiveTime(const std::chrono::milliseconds& delta_time) {
    inactive_time_ += delta_time;
}

void Dog::SetMaxInactiveTime(size_t max_inactive_time_in_seconds) {
    max_inactive_time_ = std::chrono::seconds(max_inactive_time_in_seconds);
}

void Dog::SetActive(bool active) {
    is_active_ = active;
}

bool Dog::IsActive() noexcept {
    return is_active_;
}

void Map::AddOffice(Office office) {
    if (warehouse_id_to_index_.contains(office.GetId())) {
        throw std::invalid_argument("Duplicate warehouse");
    }

    const size_t index = offices_.size();
    Office& o = offices_.emplace_back(std::move(office));
    try {
        warehouse_id_to_index_.emplace(o.GetId(), index);
    } catch (...) {
        // Удаляем офис из вектора, если не удалось вставить в unordered_map
        offices_.pop_back();
        throw;
    }
}

void Map::AddLootType(LootType loot_type) {
    loot_types_.emplace_back(loot_type);
}

RoadIndex::RoadIndex(const Map::Roads& roads) {
    for (const Road& r : roads) {
        auto norm_road = r.Normalized();
        if (r.IsHorizontal()) {
            horizontal_roads_[r.GetStart().y].emplace_back(norm_road.GetStart().x,
                                                           norm_road.GetEnd().x);
        } else {
            vertical_roads_[r.GetStart().x].emplace_back(norm_road.GetStart().y,
                                                         norm_road.GetEnd().y);
        }
    }

    MergeFragments(horizontal_roads_);
    MergeFragments(vertical_roads_);
}

RoadIndex::Roads RoadIndex::GetRoadsAtPoint(Point pt) const noexcept {
    Roads result;

    struct Comp {
        bool operator()(Coord c, const RoadFragment& f) {
            return c < f.first;
        }
        bool operator()(const RoadFragment& f, Coord c) {
            return f.second < c;
        }
    };

    if (auto it = horizontal_roads_.find(pt.y); it != horizontal_roads_.end()) {
        const auto& fragments = it->second;
        if (auto [l_bound, u_bound] = std::equal_range(fragments.begin(), fragments.end(), pt.x, Comp{});
            l_bound != u_bound) {
            assert(std::distance(l_bound, u_bound) == 1);
            auto [x_start, x_end] = *l_bound;
            result.horizontal.emplace(Road::HORIZONTAL, Point{x_start, pt.y}, x_end);
        }
    }

    if (auto it = vertical_roads_.find(pt.x); it != vertical_roads_.end()) {
        if (auto [l_bound, u_bound] = std::equal_range(it->second.begin(), it->second.end(), pt.y, Comp{});
            l_bound != u_bound) {
            assert(std::distance(l_bound, u_bound) == 1);
            auto [y_start, y_end] = *l_bound;
            result.vertical.emplace(Road::VERTICAL, Point{pt.x, y_start}, y_end);
        }
    }

    return result;
}

void RoadIndex::MergeFragments(RoadFragmentsMap& fragments_map) {
    for (auto& [_, fragments] : fragments_map) {
        boost::range::sort(fragments, [](const RoadFragment& lhs, const RoadFragment& rhs) {
            return lhs.first < rhs.first;
        });
        RoadFragment* fragment = fragments.data();
        for (RoadFragment& f : fragments) {
            if (f.first > fragment->second) {
                ++fragment;
                *fragment = f;
            } else {
                fragment->second = std::max(fragment->second, f.second);
            }
        }
        fragments.resize(fragment - fragments.data() + 1);
    }
}

bool IsValueInRange(double value, double min_value, double max_value) {
    return (value >= min_value) && (value <= max_value);
}

LostObject::LostObject(Id id, size_t type, geom::Point2D position, size_t value)
    : Item(position, DEFAULT_LOOT_WIDTH), id_(std::move(id)), type_{type}, value_{value} {
}

const LostObject::Id& LostObject::GetId() const noexcept {
    return id_;
}

const ObjectType& LostObject::GetType() const noexcept {
    return type_;
}

const size_t& LostObject::GetValue() const noexcept {
    return value_;
}

void Game::AddMap(Map map) {
    const size_t index = maps_.size();
    if (auto [it, inserted] = map_id_to_index_.emplace(map.GetId(), index); !inserted) {
        throw std::invalid_argument("Map with id "s + *map.GetId() + " already exists"s);
    } else {
        try {
            auto map_ptr = std::make_shared<Map>(std::move(map));
            maps_.emplace_back(std::move(map_ptr));
        } catch (...) {
            map_id_to_index_.erase(it);
            throw;
        }
    }
}

const Game::Maps& Game::GetMaps() const noexcept {
    return maps_;
}

const std::shared_ptr<Map> Game::FindMap(const Map::Id& id) const noexcept {
    if (auto it = map_id_to_index_.find(id); it != map_id_to_index_.end()) {
        return maps_.at(it->second);
    }
    return nullptr;
}

void Game::SetLootGeneratorConfig(LootGeneratorConfig generator) {
    loot_generator_config_ = generator;
}

const LootGeneratorConfig& Game::GetLootGeneratorConfig() const noexcept {
    return loot_generator_config_;
}
}  // namespace model
