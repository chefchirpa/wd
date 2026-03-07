#pragma once

#include <string>

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
    Fidele(const std::string& name, FideleType type, int hp, int range, Movement movement)
        : name(name), type(type), hp(hp), range(range), movement(movement) {}

    std::string getName() const { return name; }
    FideleType getType() const { return type; }
    int getHP() const { return hp; }
    int getRange() const { return range; }
    Movement getMovement() const { return movement; }

private:
    std::string name;
    FideleType type;
    int hp;
    int range;
    Movement movement;
};
