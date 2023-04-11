#include "model_serialization.h"

namespace serialization {

DogRepr::DogRepr(const std::shared_ptr<const model::Dog> dog)
    : id_(dog->GetId()), name_(dog->GetName()), pos_(dog->GetPosition()), bag_capacity_(dog->GetBagCapacity()), speed_(dog->GetSpeed()), direction_(dog->GetDirection()), score_(dog->GetScore()), bag_(dog->GetBag()) {
}

[[nodiscard]] std::shared_ptr<model::Dog> DogRepr::Restore() const {
    auto dog = std::make_shared<model::Dog>(id_, name_, pos_, bag_capacity_);
    dog->SetSpeed(speed_);
    dog->SetDirection(direction_);
    dog->SetScore(score_);
    for (const auto& item : bag_) {
        if (!dog->AddItemToBag(item)) {
            throw std::runtime_error("Failed to put bag content");
        }
    }
    return dog;
}

LostObjectRepr::LostObjectRepr(const std::shared_ptr<model::LostObject> lost_object) : id_(lost_object->GetId()), type_(lost_object->GetType()), position_(lost_object->GetPosition()), value_(lost_object->GetValue()) {
}

[[nodiscard]] std::shared_ptr<model::LostObject> LostObjectRepr::Restore() const {
    return std::make_shared<model::LostObject>(id_, type_, position_, value_);
}

PlayerRepr::PlayerRepr(const std::shared_ptr<Player> player, const Token& token) : id_(*player->GetId()), dog_(player->GetDog()), token_(*token) {
}

[[nodiscard]] std::pair<std::shared_ptr<Player>, Token> PlayerRepr::Restore() const {
    return std::make_pair(std::make_shared<Player>(Player::Id{id_}, std::make_shared<model::Dog>(*dog_.Restore())), Token{token_});
}

GameSessionRepr::GameSessionRepr(
    std::shared_ptr<GameSession> game_session,
    const std::vector<std::pair<Token, std::shared_ptr<Player>>>& tokenToPlayer)
    : id_(game_session->GetId()), map_id_(*(game_session->GetMap()->GetId())) {
    players_ser_.reserve(tokenToPlayer.size());
    lost_objects_.reserve(game_session->GetLostObjects().size());

    std::ranges::transform(tokenToPlayer, std::back_inserter(players_ser_),
                           [](const auto& token_to_player) -> PlayerRepr {
                               return PlayerRepr{token_to_player.second, token_to_player.first};
                           });

    std::ranges::transform(game_session->GetLostObjects(), std::back_inserter(lost_objects_),
                           [](const auto& id_to_lost_object) -> LostObjectRepr {
                               return LostObjectRepr{id_to_lost_object.second};
                           });
};

[[nodiscard]] GameSession::Id GameSessionRepr::RestoreSessionId() const {
    return id_;
}

[[nodiscard]] model::Map::Id GameSessionRepr::RestoreMapId() const {
    return model::Map::Id(map_id_);
}

[[nodiscard]] const std::vector<LostObjectRepr>& GameSessionRepr::GetLostObjectsSerialize() const {
    return lost_objects_;
}

[[nodiscard]] const std::vector<PlayerRepr>& GameSessionRepr::GetPlayersSerialize() const {
    return players_ser_;
}

}  // namespace serialization
