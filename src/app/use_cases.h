#pragma once
#include <chrono>

#include "game_session.h"
#include "geom.h"
#include "model.h"
#include "player.h"

struct Record {
    std::string name;
    size_t score;
    int64_t playTime;
};

struct LostObject {
    model::LostObject::Id id;
    size_t type;
    geom::Point2D position;
};

struct BagItem {
    model::FoundObject::Id id;
    size_t type;
};

struct PlayerState {
    using Bag = std::vector<BagItem>;
    Player::Id id;
    geom::Point2D position;
    geom::Vec2D speed;
    model::Direction direction;
    Bag bag;
    size_t score;
};

struct GameState {
    using PlayersStates = std::vector<PlayerState>;
    using LostObjectsStates = std::vector<LostObject>;

    PlayersStates players;
    LostObjectsStates lost_objects;
};

class GameStateUseCase {
   public:
    GameStateUseCase(std::reference_wrapper<const model::Game> game,
                     std::reference_wrapper<const PlayersToken> player_tokens,
                     std::reference_wrapper<const Players> players);
    GameState GetGameState(const Token& token) const;

   private:
    const model::Game& game_;
    const Players& players_;
    const PlayersToken& player_tokens_;
};

class Application;

class JoinGameUseCase {
   public:
    JoinGameUseCase(PlayersToken& players_tokens, Players& players, bool randomize_spawn_points);
    std::pair<std::string, std::string> Join(std::shared_ptr<GameSession> session, std::string name);

   private:
    PlayersToken* player_tokens_;
    Players* players_;
    bool randomize_spawn_points_;
};

struct MapInfo {
    std::string id;
    std::string name;
};

class ListMapsUseCase {
   public:
    using Maps = std::vector<MapInfo>;

    explicit ListMapsUseCase(const model::Game::Maps& maps);
    const Maps& ListMaps() const noexcept;

   private:
    Maps maps_;
};

class GetMapUseCase {
   public:
    explicit GetMapUseCase(std::reference_wrapper<const model::Game> game);
    const std::shared_ptr<model::Map> FindMap(const std::string& id) const;

   private:
    const model::Game& game_;
};

struct PlayerInfo {
    std::string id;
    std::string name;
};

class ListPlayersUseCase {
   public:
    ListPlayersUseCase(std::reference_wrapper<const model::Game> game,
                       std::reference_wrapper<const PlayersToken> player_tokens,
                       std::reference_wrapper<const Players> players);
    std::vector<PlayerInfo> ListPlayers(const Token& token) const;

   private:
    const model::Game& game_;
    const PlayersToken& player_tokens_;
    const Players& players_;
};

class MovePlayerUseCase {
   public:
    MovePlayerUseCase(model::Game& game, std::reference_wrapper<const PlayersToken> player_tokens);
    void Move(const Token& token, MoveAction action);

   private:
    model::Game& game_;
    const PlayersToken& player_tokens_;
};

class GameSession;

class TickUseCase {
   public:
    using Sessions = std::vector<std::shared_ptr<GameSession>>;
    explicit TickUseCase(Sessions& sessions);
    void Tick(std::chrono::milliseconds delta);

   private:
    Sessions& sessions_;
};

#include "database.h"

class RecordUseCase {
   public:
    using Records = std::vector<Record>;
    explicit RecordUseCase(PlayerRecordRepository& player_record_repository);
    Records GetRecords(size_t offset, size_t limit);
    void AddRecords(const std::vector<PlayerRecord>& player_records);

   private:
    PlayerRecordRepository& player_record_repository_;
};