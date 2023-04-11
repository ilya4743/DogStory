#pragma once

#include <random>
#include <vector>

#include "game_session.h"
#include "model.h"
#include "tagged.h"

enum class MoveAction {
    STOP = 0,
    MOVE_LEFT = 'L',
    MOVE_RIGHT = 'R',
    MOVE_UP = 'U',
    MOVE_DOWN = 'D',
    INVALID = -1
};

std::optional<model::Direction> MoveActionToDirection(MoveAction action);

class Player {
   public:
    using Id = util::Tagged<uint32_t, Player>;

    Player(Id id, std::shared_ptr<model::Dog> dog);
    const Id& GetId() const noexcept;
    std::shared_ptr<model::Dog> GetDog() const noexcept;
    const std::shared_ptr<GameSession> GetSession() const noexcept;
    void SetSession(std::shared_ptr<GameSession> session);
    void Move(MoveAction action);

   private:
    std::shared_ptr<GameSession> session_;
    std::shared_ptr<model::Dog> dog_;
    Id id_;
};

class Players {
   public:
    using PlayerIdHasher = util::TaggedHasher<Player::Id>;
    using PlayerIdToPlayer = std::unordered_map<Player::Id, std::shared_ptr<Player>, PlayerIdHasher>;

    std::shared_ptr<Player> Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<GameSession> session);
    void Add(std::shared_ptr<Player> player);
    const std::optional<PlayerIdToPlayer> FindPlayersBySessionId(const GameSession::Id& session_id) const;
    void ErasePlayerFromSession(const GameSession::Id& session_id, const Player::Id& player_id);
    void EraseSession(const GameSession::Id& session_id);

   private:
    using DogIdHasher = util::TaggedHasher<model::Dog::Id>;
    using SessionIdHasher = util::TaggedHasher<GameSession::Id>;
    using SessionIdToPlayerIdToPlayer = std::unordered_map<GameSession::Id, PlayerIdToPlayer, SessionIdHasher>;
    SessionIdToPlayerIdToPlayer session_id_to_player_id_to_player_;
    Player::Id next_player_id_{0u};
};

namespace detail {
struct TokenTag {};
}  // namespace detail

using Token = util::Tagged<std::string, detail::TokenTag>;
using TokenHasher = util::TaggedHasher<Token>;

class PlayersToken {
   public:
    std::shared_ptr<Player> FindPlayerByToken(const Token& token) const noexcept;
    Token FindTokenByPlayerId(Player::Id id) const noexcept;
    Token AddPlayer(std::shared_ptr<Player> player);
    void AddPlayer(std::shared_ptr<Player> player, Token token);
    const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& GetPlayersToken() noexcept;
    void EraseTokenByPlayerId(const Player::Id& id);

   private:
    using PlayerIdHasher = util::TaggedHasher<Player::Id>;
    using PlayerIdToToken = std::unordered_map<Player::Id, Token, PlayerIdHasher>;

    std::random_device random_device_;

    std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher> tokens;
    PlayerIdToToken players_id_to_token;

    std::mt19937_64 generator1_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    std::mt19937_64 generator2_{[this] {
        std::uniform_int_distribution<std::mt19937_64::result_type> dist;
        return dist(random_device_);
    }()};
    Token Generate();
};