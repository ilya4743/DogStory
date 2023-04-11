#include "game_session.h"

#include <cmath>
#include <ranges>

#include "item_dog_provider.h"

using namespace model;

geom::Vec2D DirectionToSpeed(Direction direction, double speed) noexcept {
    switch (direction) {
        case Direction::NORTH:
            return {0.0, -speed};
        case Direction::EAST:
            return {speed, 0.0};
        case Direction::WEST:
            return {-speed, 0.0};
        case Direction::SOUTH:
            return {0.0, speed};
    }
    return {};
}

GameSession::GameSession(Id id, std::shared_ptr<Map> map, LootGeneratorConfig loot_generator_config, net::io_context& ioc, std::optional<std::chrono::milliseconds> tick_period)
    : id_(std::move(id)), map_{map}, dog_id{0}, lost_object_id{0}, road_index_{map->GetRoads()}, loot_generator_(loot_gen::LootGenerator::TimeInterval(static_cast<uint64_t>(loot_generator_config.period * 1000)), loot_generator_config.probability), gen(rd()), generator_type(0, map_->GetLootTypesSize() - 1), strand_(std::make_shared<SessionStrand>(net::make_strand(ioc)))
    , tick_period_{tick_period} {
}

std::shared_ptr<Dog> GameSession::AddDog(std::string name, geom::Point2D spawn) {
    auto dog = std::make_shared<Dog>(dog_id, name, spawn, map_->GetBagCapacity());
    dogs_.emplace(dog_id, dog);
    *(dog_id) += 1;
    return dog;
}

void GameSession::AddDog(std::shared_ptr<model::Dog> dog) {
    dogs_.emplace(dog_id, dog);
    *(dog_id) += 1;
    net::dispatch(*strand_, [self = shared_from_this()]{
        self->GenerateLoot(self->loot_generator_.GetPeriod());
    });
}

std::shared_ptr<LostObject> GameSession::AddLostObject(size_t type, geom::Point2D spawn, size_t value) {
    auto lost_object = std::make_shared<LostObject>(lost_object_id, type, spawn, value);
    lost_objects_.emplace(lost_object_id, lost_object);
    *(lost_object_id) += 1;
    return lost_object;
}

void GameSession::AddLostObject(std::shared_ptr<model::LostObject> lost_object){
    lost_objects_.emplace(lost_object_id, std::move(lost_object));
    *(lost_object_id) += 1;
}

void GameSession::SetDogDirection(const Dog::Id& id, std::optional<Direction> direction) {
    if (auto it = dogs_.find(id); it != dogs_.end()) {
        auto dog = it->second;
        if (direction) {
            dog->SetDirection(*direction);
            dog->SetSpeed(DirectionToSpeed(*direction, map_->GetDogSpeed()));
        } else {
            dog->SetSpeed({});
        }
    } else {
        throw std::out_of_range("Invalid dog id");
    }
}

const GameSession::Dogs& GameSession::GetDogs() const noexcept {
    return dogs_;
}

const GameSession::LostObjects& GameSession::GetLostObjects() const noexcept {
    return lost_objects_;
}

const std::shared_ptr<Map> GameSession::GetMap() const noexcept {
    return map_;
}

const GameSession::Id& GameSession::GetId() const noexcept {
    return id_;
}

std::shared_ptr<GameSession::SessionStrand> GameSession::GetStrand() noexcept{
    return strand_;
}

void GameSession::MoveDog(Dog& dog, std::chrono::milliseconds time_delta) const {    
    constexpr double HALF_ROAD_WIDTH = 0.8 / 2.0;
    const auto pos = dog.GetPosition();
    const auto speed = dog.GetSpeed();
    const auto offset = speed * std::chrono::duration<double>(time_delta).count();

    // Получаем информацию о дорогах, на которых находится пёс
    auto roads = road_index_.GetRoadsAtPoint(
        {static_cast<Coord>(std::round(pos.x)), static_cast<Coord>(std::round(pos.y))});

    geom::Point2D min_coord{pos};
    geom::Point2D max_coord{pos};

    // Обновляет диапазон минимальных и максимальных координат,
    // по которым может перемещаться пёс
    auto update_range = [&min_coord, &max_coord](Point start, Point end) {
        min_coord = {std::min(start.x - HALF_ROAD_WIDTH, min_coord.x),
                     std::min(start.y - HALF_ROAD_WIDTH, min_coord.y)};
        max_coord = {std::max(end.x + HALF_ROAD_WIDTH, max_coord.x),
                     std::max(end.y + HALF_ROAD_WIDTH, max_coord.y)};
    };

    if (roads.horizontal) {
        update_range(roads.horizontal->GetStart(), roads.horizontal->GetEnd());
    }
    if (roads.vertical) {
        update_range(roads.vertical->GetStart(), roads.vertical->GetEnd());
    }

    auto new_pos = pos + offset;

    auto new_speed = std::invoke([new_pos, speed = speed, min_coord, max_coord]() mutable {
        if (!IsValueInRange(new_pos.x, min_coord.x, max_coord.x)) {
            speed.x = 0;
        }
        if (!IsValueInRange(new_pos.y, min_coord.y, max_coord.y)) {
            speed.y = 0;
        }
        return speed;
    });
    new_pos = {std::clamp(new_pos.x, min_coord.x, max_coord.x),
               std::clamp(new_pos.y, min_coord.y, max_coord.y)};

    dog.SetPosition(new_pos);
    dog.SetSpeed(new_speed);
    dog.SetGatherer();
    dog.UpdatePlayTime(time_delta);
    if(!dog.IsActive()) {
        dog.UpdateInactiveTime(time_delta);
    }
}

void GameSession::SetTickPeriod(const std::optional<std::chrono::milliseconds>& tick_period){
    tick_period_=tick_period;
}

void GameSession::Tick(std::chrono::milliseconds time_delta) {
    for (auto& [id, dog] : dogs_) {
        MoveDog(*dog, time_delta);
        dog->GetPlayTime();
    }

    std::vector<std::shared_ptr<collision_detector::Item>> items;
    std::vector<std::shared_ptr<model::Dog>> dogs;
    for (auto [id, lost_obj] : lost_objects_) {
        items.push_back(lost_obj);
    }
    for (auto office : map_->GetOffices()) {
        items.push_back(std::make_shared<model::Office>(office));
    }
    for (auto [id, dog] : dogs_) {

        dogs.push_back(dog);
    }

    model::ItemDogProvider provider(std::move(items), std::move(dogs));

    auto collected_loot = collision_detector::FindGatherEvents(std::move(provider));

    for (auto&& loot : collected_loot) {
        auto item = provider.TryCastItemTo<LostObject>(loot.item_id);
        auto gatherer = dogs_.find(provider.GetDogId(loot.gatherer_id))->second;
        if (!gatherer.get()->isFullBag() && item != nullptr) {
            FoundObject found_object{FoundObject::Id{*item->GetId()}, item->GetType(), item->GetValue()};
            gatherer->AddItemToBag(found_object);
            lost_objects_.erase(item->GetId());
        }
        auto office = provider.TryCastItemTo<Office>(loot.item_id);
        if (!gatherer.get()->isEmptyBag() && office != nullptr) {
            gatherer->ClearBag();
        }
    }

    RemoveInactiveDogs();
}

void GameSession::GenerateLoot(const std::chrono::milliseconds& delta_time) {
    auto loot_size = loot_generator_.Generate(delta_time, lost_objects_.size(), dogs_.size());

    for (int i = 0; i < loot_size; i++) {
        auto type = generator_type(gen);
        std::random_device rd;
        std::mt19937 generator_(rd());
        size_t road_index = std::uniform_int_distribution<size_t>{0, map_->GetRoads().size() - 1}(generator_);
        auto& road = map_->GetRoads()[road_index];
        double phase = std::uniform_real_distribution<double>{0.0, 1.0}(generator_);
        auto p0 = road.GetStart();
        auto p1 = road.GetEnd();
        geom::Point2D spawn{std::lerp(p0.x, p1.x, phase), std::lerp(p0.y, p1.y, phase)};
        AddLostObject(type, spawn, map_->GetLootTypeValue(type));
    }
};

void GameSession::Run(){
    if(tick_period_.has_value()){
        update_game_state_ticker_ = std::make_shared<Ticker>(
            strand_,
            tick_period_.value(),
            [self = shared_from_this()](const std::chrono::milliseconds& delta_time) {
                self->Tick(self->tick_period_.value());
            }
        );
        update_game_state_ticker_->Start();
    }
    generate_loot_ticker_ = std::make_shared<Ticker>(
        strand_,
        loot_generator_.GetPeriod(),
        [self = shared_from_this()](const std::chrono::milliseconds& delta_time) {
                self->GenerateLoot(delta_time);
        }
    );
    generate_loot_ticker_->Start();
}

void GameSession::AddRemoveInactivePlayersHandler(
        std::function<void(const GameSession::Id&)> handler) {
    remove_inactive_players_sig.connect(handler);
};

void GameSession::AddHandlingFinishedPlayersEvent(
    std::function<void(const std::vector<PlayerRecord>&)> handler) {
    handle_finished_players_sig.connect(handler);
};

void GameSession::RemoveInactiveDogs() {
    std::vector<PlayerRecord> player_records;

    std::ranges::copy(
        dogs_
        | std::views::values
        | std::views::filter(
            [](auto dog) {
                return static_cast<bool>(dog->GetPlayTime());
            }
        ) | std::views::transform(
            [](auto dog)->PlayerRecord {
                return PlayerRecord{dog->GetName(),
                                        dog->GetScore(),
                                        dog->GetPlayTime().value().count()};
            }
        ), std::back_inserter(player_records)
    );

    if(player_records.empty()) {
        return;
    }

    std::erase_if(dogs_, [](const auto& item) {
        auto const& [dog_id, dog] = item;
        return dog->GetPlayTime();
    });

    handle_finished_players_sig(std::move(player_records));
    remove_inactive_players_sig(id_);
};