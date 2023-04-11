#include "application.h"

#include <ranges>

#include "database_invariants.h"
#include "model_serialization.h"

Application::Application(model::Game& game, bool randomize_spawn_points, net::io_context& ioc, std::optional<std::chrono::milliseconds> tick_period, std::optional<fs::path> state_file_path, std::optional<std::chrono::milliseconds> state_period, const DbConnectrioSettings& db_settings)
    : game_{game}, joiner_{player_tokens_, players_, randomize_spawn_points}, ioc_{ioc}, tick_period_{tick_period}, state_file_path_{state_file_path}, state_period_{state_period}, db_{db_settings}, record_use_case{db_.GetPlayerRecordRepository()} {
}

const ListMapsUseCase::Maps& Application::ListMaps() const noexcept {
    return list_maps_.ListMaps();
}

const std::shared_ptr<model::Map> Application::FindMap(const std::string& id) const {
    return map_finder_.FindMap(id);
}

const GameState Application::GetGameState(const Token& token) const {
    return game_state_.GetGameState(token);
}

void Application::MovePlayer(const Token& token, MoveAction action) {
    mover_.Move(token, action);
}

void Application::Tick(std::chrono::milliseconds delta) {
    ticker_.Tick(delta);
    static int period = state_period_
                            ? state_period_.value().count()
                            : 0;
    if (!state_period_) {
        return;
    }
    period -= delta.count();
    if (period <= 0) {
        SaveGame();
        period = state_period_.value().count();
    }
}

std::optional<RecordUseCase::Records> Application::GetRecords(std::optional<size_t> offset, std::optional<size_t> limit) {
    size_t start{0};
    size_t records_limit{db_invariants::DEFAULT_LIMIT};
    if (offset) {
        start = *offset;
    }
    if (limit) {
        if (*limit > db_invariants::DEFAULT_LIMIT) {
            return std::nullopt;
        }
        records_limit = *limit;
    }

    return record_use_case.GetRecords(start, records_limit);
}

std::vector<PlayerInfo> Application::ListPlayers(const Token& token) const {
    return list_players_.ListPlayers(token);
}

std::pair<std::string, std::string> Application::JoinGame(const std::string& map_id, std::string name) {
    auto const session = FindSessionsByMapId(model::Map::Id{map_id});
    if (session.has_value()) {
        return joiner_.Join(session.value(), std::move(name));
    } else if (auto map = game_.FindMap(model::Map::Id{map_id})) {
        auto new_session = AddSession(map);
        auto out = joiner_.Join(new_session, std::move(name));
        new_session->Run();
        return out;
    } else
        throw std::runtime_error{"Join game Error, invalid map"};
}

std::optional<std::shared_ptr<GameSession>> Application::FindSessionsByMapId(const model::Map::Id& session_map_id) const noexcept {
    auto it_sessions_id_to_index_ = map_id_to_sessions_id_to_index_.find(session_map_id);
    if (it_sessions_id_to_index_ != map_id_to_sessions_id_to_index_.end()) {
        auto [map_id, sessions_id_to_index_] = *it_sessions_id_to_index_;
        auto it_session = sessions_id_to_index_.begin();
        if (it_session != sessions_id_to_index_.end()) {
            return sessions_[it_session->second];
        }
    }
    return std::nullopt;
}

std::shared_ptr<GameSession> Application::AddSession(const std::shared_ptr<model::Map> session_map) {
    using namespace std::literals;
    auto session = std::make_shared<GameSession>(session_id, session_map, game_.GetLootGeneratorConfig(), ioc_, tick_period_);
    const size_t index = sessions_.size();
    // if (auto [it, inserted] = session_id_to_index_.emplace(session->GetId(), index); !inserted) {
    //     throw std::invalid_argument("Session with id "s + std::to_string(*session->GetId()) + " already exists"s);
    // } else {
    try {
        sessions_.emplace_back(session);
        auto it_sessions_id_to_index_ = map_id_to_sessions_id_to_index_.find(session_map->GetId());
        if (it_sessions_id_to_index_ != map_id_to_sessions_id_to_index_.end()) {
            it_sessions_id_to_index_->second.emplace(session->GetId(), index);
        } else {
            SessionIdToIndex session_id_to_index;
            session_id_to_index.emplace(session->GetId(), index);
            map_id_to_sessions_id_to_index_.emplace(session_map->GetId(), std::move(session_id_to_index));
        }
        *(session_id) += 1;
    } catch (...) {
        // session_id_to_index_.erase(it);
        throw;
    }
    //}

    session->AddHandlingFinishedPlayersEvent(
        [self = shared_from_this()](const std::vector<PlayerRecord>& player_records) {
            self->CommitGameRecords(player_records);
        });
    session->AddRemoveInactivePlayersHandler(
        [self = shared_from_this()](const GameSession::Id& session_id) {
            self->RemoveInactivePlayers(session_id);
        });

    return session;
}

void Application::AddSession(std::shared_ptr<GameSession> session) {
    using namespace std::literals;
    if (session->GetId() >= session_id) {
        session_id = GameSession::Id{*session->GetId() + 1};
    }
    const size_t index = sessions_.size();
    // if (auto [it, inserted] = session_id_to_index_.emplace(session->GetId(), index); !inserted) {
    //     throw std::invalid_argument("Session with id "s + std::to_string(*session->GetId()) + " already exists"s);
    // } else {
    try {
        sessions_.emplace_back(session);
        auto it_sessions_id_to_index_ = map_id_to_sessions_id_to_index_.find(session->GetMap()->GetId());
        if (it_sessions_id_to_index_ != map_id_to_sessions_id_to_index_.end()) {
            it_sessions_id_to_index_->second.emplace(session->GetId(), index);
        } else {
            SessionIdToIndex session_id_to_index;
            session_id_to_index.emplace(session->GetId(), index);
            map_id_to_sessions_id_to_index_.emplace(session->GetMap()->GetId(), std::move(session_id_to_index));
        }
    } catch (...) {
        // session_id_to_index_.erase(it);
        throw;
    }
    //}

    session->AddHandlingFinishedPlayersEvent(
        [self = shared_from_this()](const std::vector<PlayerRecord>& player_records) {
            self->CommitGameRecords(player_records);
        });
    session->AddRemoveInactivePlayersHandler(
        [self = shared_from_this()](const GameSession::Id& session_id) {
            self->RemoveInactivePlayers(session_id);
        });
}

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <filesystem>
#include <fstream>

void Application::SaveGame() {
    std::vector<serialization::GameSessionRepr> sessions_repr;
    sessions_repr.reserve(sessions_.size());
    for (auto session : sessions_) {
        std::vector<std::pair<Token, std::shared_ptr<Player>>> players;
        // players_.FindPlayerByDogId
        auto players_session = players_.FindPlayersBySessionId(session->GetId());
        for (auto [player_id, player] : players_session.value()) {
            auto token = player_tokens_.FindTokenByPlayerId(player_id);
            players.emplace_back(token, player);
        }

        sessions_repr.emplace_back(session, std::move(players));
    }
    std::ofstream file(state_file_path_.value());
    boost::archive::text_oarchive oa(file);
    oa << sessions_repr;
    file.close();
    // std::filesystem::rename("tmp.txt", state_file_path_.value());
}

void Application::RestoreGame() {
    if (!state_file_path_.has_value())
        return;
    else if (!state_file_path_.value().has_filename())
        return;
    if (!(fs::exists(state_file_path_.value()))) {
        if (state_period_.has_value()) {
            save_game_ticker_ = std::make_shared<Ticker>(
                ioc_,
                state_period_.value(),
                [self = shared_from_this()](const std::chrono::milliseconds& delta_time) {
                    self->SaveGame();
                });
            save_game_ticker_->Start();
        }
        return;
    }
    std::vector<serialization::GameSessionRepr> sessions_repr;
    std::ifstream file1(state_file_path_.value());
    boost::archive::text_iarchive ia(file1);
    ia >> sessions_repr;
    file1.close();
    sessions_.reserve(sessions_repr.size());
    for (auto&& session_repr : sessions_repr) {
        auto session = std::make_shared<GameSession>(GameSession::Id{*session_repr.RestoreSessionId()}, game_.FindMap(session_repr.RestoreMapId()), game_.GetLootGeneratorConfig(), ioc_, tick_period_);

        for (auto&& player_repr : session_repr.GetPlayersSerialize()) {
            auto [player, token] = player_repr.Restore();
            player_tokens_.AddPlayer(player, token);
            player->SetSession(session);
            players_.Add(player);
            session->AddDog(player->GetDog());
        }

        for (auto&& lost_object_repr : session_repr.GetLostObjectsSerialize()) {
            auto lost_object = lost_object_repr.Restore();
            session->AddLostObject(lost_object);
        }

        AddSession(session);
        session->Run();
    }

    if (state_period_.has_value()) {
        save_game_ticker_ = std::make_shared<Ticker>(
            ioc_,
            state_period_.value(),
            [self = shared_from_this()](const std::chrono::milliseconds& delta_time) {
                self->SaveGame();
            });
        save_game_ticker_->Start();
    }
}

std::optional<fs::path> Application::GetStateFilePath() {
    return state_file_path_;
}

void Application::CommitGameRecords(const std::vector<PlayerRecord>& player_records) {
    record_use_case.AddRecords(player_records);
};

void Application::RemoveInactivePlayers(const GameSession::Id& session_id) {
    std::unordered_map<Token,
                       Player::Id,
                       TokenHasher>
        token_to_player_id_for_delete;
    auto session_players = players_.FindPlayersBySessionId(session_id).value();

    for (auto it = session_players.begin(); it != session_players.end(); ++it) {
        if (!it->second->GetSession()->GetDogs().contains(it->second->GetDog()->GetId())) {
            auto player_id = it->first;
            players_.ErasePlayerFromSession(it->second->GetSession()->GetId(), player_id);
            if (players_.FindPlayersBySessionId(session_id)->size() == 0) {
                players_.EraseSession(it->second->GetSession()->GetId());
                auto tmp = sessions_.begin();
                for (auto its = sessions_.begin(); its != sessions_.end(); ++its) {
                    if ((*its)->GetId() == it->second->GetSession()->GetId()) {
                        tmp = its;
                        sessions_.erase(tmp);
                        break;
                    }
                }
            }
            player_tokens_.EraseTokenByPlayerId(player_id);
        }
    }
};

std::optional<std::shared_ptr<GameSession>> Application::GetSessionByToken(const Token& token) {
    auto player = player_tokens_.FindPlayerByToken(token);
    return player->GetSession();
}
