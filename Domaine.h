#pragma once

class Domaine {
public:
    Domaine() : hp(20) {}

    int getHP() const { return hp; }
    void setHP(int hp) { this->hp = hp; }

private:
    int hp;
};
