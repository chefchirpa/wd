#pragma once

#include <string>
#include "Types.h"

enum class FideleType {
    Heros,
    Creature,
    Divinite
};

struct Movement {
    int forward;
    int backward;
    int left;
    int right;
};

class Fidele {
public:
    Fidele(const std::string& name, FideleType type, int hp, int range, Movement movement, Player owner = Player::None)
        : name(name), type(type), hp(hp), currentHp(hp), range(range), movement(movement), owner(owner),
          alive(true), currentPosition{-1, -1}, deathPosition{-1, -1} {}

    std::string getName() const { return name; }
    FideleType getType() const { return type; }
    int getHP() const { return hp; }
    int getCurrentHP() const { return currentHp; }
    void takeDamage(int amount) {
        currentHp -= amount;
        if (currentHp < 0) currentHp = 0;
    }

    int getRange() const { return range; }
    Movement getMovement() const { return movement; }

    Player getOwner() const { return owner; }
    void setOwner(Player p) { owner = p; }

    bool isAlive() const { return alive; }
    void setAlive(bool state) { alive = state; }

    int getAttackBonus() const {
        switch (type) {
            case FideleType::Heros: return 2;
            case FideleType::Creature: return 1;
            case FideleType::Divinite: return 0;
        }
        return 0;
    }

    int getDefenseBonus() const {
        switch (type) {
            case FideleType::Divinite: return 2;
            case FideleType::Creature: return 1;
            case FideleType::Heros: return 0;
        }
        return 0;
    }

    Position getPosition() const { return currentPosition; }
    void setPosition(Position pos) { currentPosition = pos; }

    Position getDeathPosition() const { return deathPosition; }
    void setDeathPosition(Position pos) { deathPosition = pos; }

private:
    std::string name;
    FideleType type;
    int hp;
    int currentHp;
    int range;
    Movement movement;

    Player owner;
    bool alive;
    Position currentPosition;
    Position deathPosition;
};
