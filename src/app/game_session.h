#pragma once
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/signals2/signal.hpp>
#include <random>

#include "model.h"
#include "player_record.h"
#include "ticker.h"

namespace net = boost::asio;

class GameSession : public std::enable_shared_from_this<GameSession> {
   public:
    using Id = util::Tagged<uint32_t, GameSession>;
    using DogIdHasher = util::TaggedHasher<model::Dog::Id>;
    using LostObjectIdHasher = util::TaggedHasher<model::LostObject::Id>;
    using Dogs = std::unordered_map<model::Dog::Id, std::shared_ptr<model::Dog>, DogIdHasher>;
    using LostObjects = std::unordered_map<model::LostObject::Id, std::shared_ptr<model::LostObject>, LostObjectIdHasher>;
    using SessionStrand = net::strand<net::io_context::executor_type>;

    explicit GameSession(Id id, std::shared_ptr<model::Map> map, model::LootGeneratorConfig loot_generator_config, net::io_context& ioc, std::optional<std::chrono::milliseconds> tick_period);
    std::shared_ptr<model::Dog> AddDog(std::string name, geom::Point2D spawn);
    void AddDog(std::shared_ptr<model::Dog> dog);
    std::shared_ptr<model::LostObject> AddLostObject(size_t type, geom::Point2D spawn, size_t value);
    void AddLostObject(std::shared_ptr<model::LostObject> lost_object);
    void SetDogDirection(const model::Dog::Id& id, std::optional<model::Direction> direction);
    const Dogs& GetDogs() const noexcept;
    const LostObjects& GetLostObjects() const noexcept;
    const std::shared_ptr<model::Map> GetMap() const noexcept;
    const Id& GetId() const noexcept;
    std::shared_ptr<SessionStrand> GetStrand() noexcept;
    void SetTickPeriod(const std::optional<std::chrono::milliseconds>& tick_period);
    void Tick(std::chrono::milliseconds time_delta);
    void MoveDog(model::Dog& dog, std::chrono::milliseconds time_delta) const;
    void GenerateLoot(const std::chrono::milliseconds& delta_time);
    void Run();
    void AddRemoveInactivePlayersHandler(std::function<void(const GameSession::Id&)> handler);
    void AddHandlingFinishedPlayersEvent(std::function<void(const std::vector<PlayerRecord>&)> handler);

   private:
    Id id_;
    Dogs dogs_;
    LostObjects lost_objects_;
    const std::shared_ptr<model::Map> map_;
    model::Dog::Id dog_id{0};
    model::LostObject::Id lost_object_id;
    model::RoadIndex road_index_;
    loot_gen::LootGenerator loot_generator_;
    std::random_device rd;
    std::default_random_engine gen;
    std::uniform_int_distribution<size_t> generator_type;
    std::shared_ptr<SessionStrand> strand_;
    std::optional<std::chrono::milliseconds> tick_period_;
    std::shared_ptr<Ticker> update_game_state_ticker_;
    std::shared_ptr<Ticker> generate_loot_ticker_;

    boost::signals2::signal<void(const GameSession::Id&)> remove_inactive_players_sig;
    boost::signals2::signal<void(const std::vector<PlayerRecord>&)> handle_finished_players_sig;

    void RemoveInactiveDogs();
};