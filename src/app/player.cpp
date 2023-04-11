#include "player.h"

#include <iomanip>

std::optional<model::Direction> MoveActionToDirection(MoveAction action) {
    using MoveAction = MoveAction;

    switch (action) {
        case MoveAction::STOP:
            return std::nullopt;
        case MoveAction::MOVE_LEFT:
            return model::Direction::WEST;
        case MoveAction::MOVE_RIGHT:
            return model::Direction::EAST;
        case MoveAction::MOVE_UP:
            return model::Direction::NORTH;
        case MoveAction::MOVE_DOWN:
            return model::Direction::SOUTH;
    }
    throw std::invalid_argument("Invalid move action");
}

Player::Player(Id id, std::shared_ptr<model::Dog> dog)
    : id_{std::move(id)}, dog_(std::move(dog)) {
}

const Player::Id& Player::GetId() const noexcept {
    return id_;
}

std::shared_ptr<model::Dog> Player::GetDog() const noexcept {
    return dog_;
}

const std::shared_ptr<GameSession> Player::GetSession() const noexcept {
    return session_;
}

void Player::SetSession(std::shared_ptr<GameSession> session){
    session_=session;
}


void Player::Move(MoveAction action) {
    session_->SetDogDirection(dog_->GetId(), MoveActionToDirection(action));
}

Token PlayersToken::Generate() {
    std::stringstream stringstream;
    stringstream << std::setw(16) << std::setfill('0') << std::hex << generator1_();
    stringstream << std::setw(16) << std::setfill('0') << std::hex << generator2_();
    return Token{std::move(stringstream.str())};
}

std::shared_ptr<Player> PlayersToken::FindPlayerByToken(const Token& token) const noexcept {
    auto itPlayer = tokens.find(token);
    if (itPlayer != tokens.end())
        return (*itPlayer).second;
    else
        return nullptr;
}

Token PlayersToken::FindTokenByPlayerId(Player::Id id) const noexcept{
    auto itToken=players_id_to_token.find(id);
    if (itToken != players_id_to_token.end())
        return (*itToken).second;
}

Token PlayersToken::AddPlayer(std::shared_ptr<Player> player) {
    while (1) {
        auto token = Generate();
        if ((tokens.try_emplace(token, player)).second){
            players_id_to_token.emplace(player->GetId(), token);
            return token;
        }
    }
}

void PlayersToken::AddPlayer(std::shared_ptr<Player> player, Token token) {
    players_id_to_token.emplace(player->GetId(),token);
    tokens.emplace(token, player);
}

const std::unordered_map<Token, std::shared_ptr<Player>, TokenHasher>& PlayersToken::GetPlayersToken() noexcept{   
    return tokens;
};

void PlayersToken::EraseTokenByPlayerId(const Player::Id& id){
    auto token=players_id_to_token.find(id)->second;
    tokens.erase(tokens.find(token));
    players_id_to_token.erase(players_id_to_token.find(id));
}

std::shared_ptr<Player> Players::Add(std::shared_ptr<model::Dog> dog, std::shared_ptr<GameSession> session) {
    auto player = std::make_shared<Player>(next_player_id_, dog);
    player->SetSession(session);
    auto it_player_id_to_player = session_id_to_player_id_to_player_.find(session->GetId());
    if(it_player_id_to_player!=session_id_to_player_id_to_player_.end()){
        it_player_id_to_player->second.emplace(player->GetId(), player);
    } else{
        PlayerIdToPlayer player_id_to_player;
        player_id_to_player.emplace(player->GetId(), player);
        session_id_to_player_id_to_player_.emplace(session->GetId(), std::move(player_id_to_player));
    }
    ++(*next_player_id_);
    return player;
}

void Players::Add(std::shared_ptr<Player> player) {
    if(player->GetId()>=next_player_id_)
        next_player_id_=Player::Id{*player->GetId()+1};

    auto it_player_id_to_player = session_id_to_player_id_to_player_.find(player->GetSession()->GetId());
    if(it_player_id_to_player!=session_id_to_player_id_to_player_.end()){
        it_player_id_to_player->second.emplace(player->GetId(), player);
    } else{
        PlayerIdToPlayer player_id_to_player;
        player_id_to_player.emplace(player->GetId(), player);
        session_id_to_player_id_to_player_.emplace(player->GetSession()->GetId(), std::move(player_id_to_player));
    }
}

const std::optional<Players::PlayerIdToPlayer> Players::FindPlayersBySessionId(const GameSession::Id& session_id) const {
    auto it_player_id_to_player = session_id_to_player_id_to_player_.find(session_id);
    if(it_player_id_to_player!=session_id_to_player_id_to_player_.end()){
        auto [session_id, player_id_to_player]=*it_player_id_to_player;
        return player_id_to_player;
    }
    return std::nullopt;
}

void Players::ErasePlayerFromSession(const GameSession::Id& session_id, const Player::Id& player_id){
    session_id_to_player_id_to_player_.find(session_id)->second.erase(session_id_to_player_id_to_player_.find(session_id)->second.find(player_id));
}

void Players::EraseSession(const GameSession::Id& session_id){
    session_id_to_player_id_to_player_.erase(session_id_to_player_id_to_player_.find(session_id));
}