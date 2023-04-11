#pragma once
#include <filesystem>

#include "database.h"
#include "model.h"
#include "player.h"
#include "ticker.h"
#include "use_cases.h"
namespace fs = std::filesystem;

class Application : public std::enable_shared_from_this<Application> {
   public:
    using Sessions = std::vector<std::shared_ptr<GameSession>>;
    using SessionIdHasher = util::TaggedHasher<GameSession::Id>;
    using SessionIdToIndex = std::unordered_map<GameSession::Id, size_t, SessionIdHasher>;
    using MapIdHasher = util::TaggedHasher<model::Map::Id>;
    using MapIdToSessionIdToIndex = std::unordered_map<model::Map::Id, SessionIdToIndex, MapIdHasher>;

    explicit Application(model::Game& game, bool randomize_spawn_points, net::io_context& ioc, std::optional<std::chrono::milliseconds> tick_period, std::optional<fs::path> state_file_path, std::optional<std::chrono::milliseconds> state_period, const DbConnectrioSettings& db_settings);
    const ListMapsUseCase::Maps& ListMaps() const noexcept;
    const std::shared_ptr<model::Map> FindMap(const std::string& id) const;
    std::pair<std::string, std::string> JoinGame(const std::string& map_id, std::string name);
    std::vector<PlayerInfo> ListPlayers(const Token& token) const;
    const GameState GetGameState(const Token& token) const;
    void MovePlayer(const Token& token, MoveAction action);
    void Tick(std::chrono::milliseconds delta);
    std::optional<RecordUseCase::Records> GetRecords(std::optional<size_t> offset, std::optional<size_t> limit);
    std::optional<std::shared_ptr<GameSession>> GetSessionByToken(const Token& token);
    std::optional<std::shared_ptr<GameSession>> FindSessionsByMapId(const model::Map::Id& session_map_id) const noexcept;
    std::shared_ptr<GameSession> AddSession(const std::shared_ptr<model::Map> session_map);
    void AddSession(std::shared_ptr<GameSession> session);
    void SaveGame();
    void RestoreGame();
    std::optional<fs::path> GetStateFilePath();
    void CommitGameRecords(const std::vector<PlayerRecord>& player_records);
    void RemoveInactivePlayers(const GameSession::Id& session_id);

   private:
    model::Game& game_;
    Players players_;
    GameSession::Id session_id{0};
    Sessions sessions_;
    postgres::Database db_;
    PlayersToken player_tokens_;
    ListMapsUseCase list_maps_{game_.GetMaps()};
    GetMapUseCase map_finder_{game_};
    JoinGameUseCase joiner_;
    ListPlayersUseCase list_players_{game_, player_tokens_, players_};
    GameStateUseCase game_state_{game_, player_tokens_, players_};
    MovePlayerUseCase mover_{game_, player_tokens_};
    TickUseCase ticker_{sessions_};
    RecordUseCase record_use_case;
    net::io_context& ioc_;
    std::optional<std::chrono::milliseconds> tick_period_;
    std::optional<fs::path> state_file_path_;
    std::optional<std::chrono::milliseconds> state_period_;
    std::shared_ptr<Ticker> save_game_ticker_;
    MapIdToSessionIdToIndex map_id_to_sessions_id_to_index_;
};