#pragma once
#include <boost/serialization/vector.hpp>

#include "model.h"
#include "player.h"

namespace geom {

template <typename Archive>
void serialize(Archive& ar, Point2D& point, [[maybe_unused]] const unsigned version) {
    ar& point.x;
    ar& point.y;
}

template <typename Archive>
void serialize(Archive& ar, Vec2D& vec, [[maybe_unused]] const unsigned version) {
    ar& vec.x;
    ar& vec.y;
}

}  // namespace geom

namespace model {

template <typename Archive>
void serialize(Archive& ar, FoundObject& obj, [[maybe_unused]] const unsigned version) {
    ar&(*obj.id);
    ar&(obj.type);
    ar&(obj.value);
}

}  // namespace model

namespace serialization {

// DogRepr (DogRepresentation) - сериализованное представление класса Dog
class DogRepr {
   public:
    DogRepr() = default;
    explicit DogRepr(const std::shared_ptr<const model::Dog> dog);
    [[nodiscard]] std::shared_ptr<model::Dog> Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& name_;
        ar& pos_;
        ar& bag_capacity_;
        ar& speed_;
        ar& direction_;
        ar& score_;
        ar& bag_;
    }

   private:
    model::Dog::Id id_ = model::Dog::Id{0u};
    std::string name_;
    geom::Point2D pos_;
    size_t bag_capacity_ = 0;
    geom::Vec2D speed_;
    model::Direction direction_ = model::Direction::NORTH;
    model::Score score_ = 0;
    model::Dog::Bag bag_;
};

class LostObjectRepr {
   public:
    LostObjectRepr() = default;
    explicit LostObjectRepr(const std::shared_ptr<model::LostObject> lost_object);
    [[nodiscard]] std::shared_ptr<model::LostObject> Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& type_;
        ar& position_;
        ar& value_;
    }

   private:
    model::LostObject::Id id_;
    model::ObjectType type_;
    geom::Point2D position_;
    size_t value_;
};

class PlayerRepr {
   public:
    PlayerRepr() = default;
    explicit PlayerRepr(const std::shared_ptr<Player> player, const Token& token);
    [[nodiscard]] std::pair<std::shared_ptr<Player>, Token> Restore() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& dog_;
        ar& token_;
    }

   private:
    Player::Id id_;
    DogRepr dog_;
    std::string token_;
};

class GameSessionRepr {
   public:
    GameSessionRepr() = default;
    GameSessionRepr(
        std::shared_ptr<GameSession> game_session,
        const std::vector<std::pair<Token, std::shared_ptr<Player>>>& tokenToPalyer);

    [[nodiscard]] GameSession::Id RestoreSessionId() const;
    [[nodiscard]] model::Map::Id RestoreMapId() const;
    [[nodiscard]] const std::vector<LostObjectRepr>& GetLostObjectsSerialize() const;
    [[nodiscard]] const std::vector<PlayerRepr>& GetPlayersSerialize() const;

    template <typename Archive>
    void serialize(Archive& ar, [[maybe_unused]] const unsigned version) {
        ar&* id_;
        ar& map_id_;
        ar& players_ser_;
        ar& lost_objects_;
    }

   private:
    GameSession::Id id_;
    std::string map_id_;
    std::vector<PlayerRepr> players_ser_;
    std::vector<LostObjectRepr> lost_objects_;
};

}  // namespace serialization
