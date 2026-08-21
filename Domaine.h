#pragma once

#include "Types.h"

class Domaine {
public:
    Domaine(Player owner) : owner(owner), hp(20) {}

    Player getOwner() const { return owner; }

    int getHP() const { return hp; }
    void setHP(int hp) { this->hp = hp; }

    void takeDamage(int amount) {
        hp -= amount;
        if (hp < 0) hp = 0;
    }

private:
    Player owner;
    int hp;
};
