#include "use_cases.h"

GameStateUseCase::GameStateUseCase(std::reference_wrapper<const model::Game> game,
                                   std::reference_wrapper<const PlayersToken> player_tokens,
                                   std::reference_wrapper<const Players> players)
    : game_{game.get()}, players_{players.get()}, player_tokens_{player_tokens.get()} {
}

GameState GameStateUseCase::GetGameState(const Token& token) const {
    auto player = player_tokens_.FindPlayerByToken(token);
    if (player == nullptr)
        return {};

    auto players = players_.FindPlayersBySessionId(player->GetSession()->GetId());

    const auto lost_objects = player->GetSession()->GetLostObjects();
    GameState result;
    result.players.reserve(players->size());
    result.lost_objects.reserve(lost_objects.size());

    for (const auto& [player_id, player] : players.value()) {
        auto dog = player->GetDog();
        PlayerState::Bag bag;
        for (const auto& item : dog->GetBag()) {
            bag.emplace_back(item.id, item.type);
        }
        result.players.emplace_back(player->GetId(), (*dog).GetPosition(), (*dog).GetSpeed(), (*dog).GetDirection(), std::move(bag), (*dog).GetScore());
    }

    for (const auto& [lost_object_id, lost_object] : lost_objects) {
        result.lost_objects.emplace_back(lost_object_id, (*lost_object).GetType(), (*lost_object).GetPosition());
    }

    result.players = {result.players.rbegin(), result.players.rend()};
    result.lost_objects = {result.lost_objects.rbegin(), result.lost_objects.rend()};
    return result;
}

JoinGameUseCase::JoinGameUseCase(PlayersToken& players_tokens, Players& players, bool randomize_spawn_points) : player_tokens_{&players_tokens}, players_{&players}, randomize_spawn_points_{randomize_spawn_points} {
}

geom::Point2D GenerateSpawnPoint(const model::Map::Roads& roads, bool isRandom) {
    if (isRandom) {
        std::random_device rd;
        std::mt19937 generator_(rd());
        size_t road_index = std::uniform_int_distribution<size_t>{0, roads.size() - 1}(generator_);
        auto& road = roads[road_index];
        double phase = std::uniform_real_distribution<double>{0.0, 1.0}(generator_);
        auto p0 = road.GetStart();
        auto p1 = road.GetEnd();
        return {std::lerp(p0.x, p1.x, phase), std::lerp(p0.y, p1.y, phase)};
    }
    const auto p = roads.at(0).GetStart();
    return {static_cast<double>(p.x), static_cast<double>(p.y)};
}

std::pair<std::string, std::string> JoinGameUseCase::Join(std::shared_ptr<GameSession> session, std::string name) {
    if (name.empty()) {
        throw std::runtime_error{"Join game Error, invalid name"};
    }
    auto spawn_point = GenerateSpawnPoint(session->GetMap()->GetRoads(), randomize_spawn_points_);
    auto player = players_->Add(session->AddDog(std::move(name), spawn_point), session);
    auto token = player_tokens_->AddPlayer(player);
    return std::make_pair(*token, std::to_string(*(player->GetId())));
}

ListMapsUseCase::ListMapsUseCase(const model::Game::Maps& maps) {
    maps_.reserve(maps.size());
    for (const auto& map : maps) {
        maps_.emplace_back(MapInfo{*(map.get()->GetId()), map.get()->GetName()});
    }
}

const ListMapsUseCase::Maps& ListMapsUseCase::ListMaps() const noexcept {
    return maps_;
}

GetMapUseCase::GetMapUseCase(std::reference_wrapper<const model::Game> game)
    : game_{game.get()} {
}

const std::shared_ptr<model::Map> GetMapUseCase::FindMap(const std::string& id) const {
    return game_.FindMap(model::Map::Id{id});
}

ListPlayersUseCase::ListPlayersUseCase(std::reference_wrapper<const model::Game> game,
                                       std::reference_wrapper<const PlayersToken> player_tokens,
                                       std::reference_wrapper<const Players> players)
    : game_{game.get()}, player_tokens_{player_tokens.get()}, players_{players.get()} {
}

std::vector<PlayerInfo> ListPlayersUseCase::ListPlayers(const Token& token) const {
    auto player = player_tokens_.FindPlayerByToken(token);
    if (player == nullptr)
        return {};

    auto players = players_.FindPlayersBySessionId(player->GetSession()->GetId());

    std::vector<PlayerInfo> result;
    result.reserve(players->size());

    for (const auto [player_id, player] : players.value()) {
        result.emplace_back(PlayerInfo{std::to_string(*player->GetId()), player->GetDog()->GetName()});
    }

    return {result.rbegin(), result.rend()};
}

MovePlayerUseCase::MovePlayerUseCase(model::Game& game, std::reference_wrapper<const PlayersToken> player_tokens)
    : game_{game}, player_tokens_{player_tokens.get()} {
}

void MovePlayerUseCase::Move(const Token& token, MoveAction action) {
    auto player = player_tokens_.FindPlayerByToken(token);
    if (player)
        net::dispatch(*(player->GetSession()->GetStrand()), [player, &action] {
            player->Move(action);
        });
}

TickUseCase::TickUseCase(TickUseCase::Sessions& sessions)
    : sessions_{sessions} {
}

void TickUseCase::Tick(std::chrono::milliseconds delta) {
    for (auto& session : sessions_) {
        net::dispatch(*(session->GetStrand()), [session, &delta] {
            session->Tick(delta);
        });
    }
}

RecordUseCase::RecordUseCase(PlayerRecordRepository& player_record_repository) : player_record_repository_(player_record_repository) {
}

RecordUseCase::Records RecordUseCase::GetRecords(size_t offset, size_t limit) {
    auto player_records = player_record_repository_.GetRecordsTable(offset, limit);
    Records records;
    records.reserve(player_records.size());
    for (auto&& player_record : player_records) {
        records.emplace_back(Record{player_record.GetName(), player_record.GetScore(), player_record.GetPlayTime()});
    }
    return records;
}

void RecordUseCase::AddRecords(const std::vector<PlayerRecord>& player_records) {
    player_record_repository_.SaveRecordsTable(player_records);
};