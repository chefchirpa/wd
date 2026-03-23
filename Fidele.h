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
    static Language currentLanguage;

    Fidele(const std::string& nameEn, const std::string& nameFr,
           const std::string& abilityEn, const std::string& abilityFr,
           const std::string& descEn, const std::string& descFr,
           Faction faction, FideleType type, int hp, int range, Movement movement, Player owner = Player::None)
        : nameEn(nameEn), nameFr(nameFr), abilityEn(abilityEn), abilityFr(abilityFr),
          descEn(descEn), descFr(descFr), faction(faction), type(type),
          hp(hp), currentHp(hp), range(range), movement(movement), owner(owner),
          alive(true), currentPosition{-1, -1}, deathPosition{-1, -1} {}

    std::string getName() const { return currentLanguage == Language::English ? nameEn : nameFr; }
    std::string getAbility() const { return currentLanguage == Language::English ? abilityEn : abilityFr; }
    std::string getDescription() const { return currentLanguage == Language::English ? descEn : descFr; }

    Faction getFaction() const { return faction; }
    FideleType getType() const { return type; }
    int getHP() const { return hp; }
    int getCurrentHP() const { return currentHp; }
    void takeDamage(int amount) {
        currentHp -= amount;
        if (currentHp < 0) currentHp = 0;
    }
    void setHP(int hp) { currentHp = hp; }
    void resetHP() {
        currentHp = hp;
        emperorsGraceUsed = false;
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

    // Apply special ability logic modifying combat stats
    void applyOffensiveAbility(Fidele* target, int& additionalAttackBonus) const;
    void applyDefensiveAbility(class Board* board, Fidele* attacker, int& additionalDefenseBonus) const;

    bool getSkipNextAttack() const { return skipNextAttack; }
    void setSkipNextAttack(bool skip) { skipNextAttack = skip; }

    bool getIsAsleep() const { return isAsleep; }
    void setAsleep(bool asleep) { isAsleep = asleep; }

    bool getEmperorsGraceUsed() const { return emperorsGraceUsed; }
    void setEmperorsGraceUsed(bool used) { emperorsGraceUsed = used; }

    bool isImmuneToAbilities() const { return (abilityEn == "Cuirassier" || abilityFr == "Cuirassier"); }

    Position getPosition() const { return currentPosition; }
    void setPosition(Position pos) { currentPosition = pos; }

    Position getDeathPosition() const { return deathPosition; }
    void setDeathPosition(Position pos) { deathPosition = pos; }

private:
    std::string nameEn;
    std::string nameFr;
    std::string abilityEn;
    std::string abilityFr;
    std::string descEn;

public:
    // Helper to override ability for testing specific dynamic triggers
    void setAbility(const std::string& en, const std::string& fr) {
        abilityEn = en;
        abilityFr = fr;
    }
    std::string descFr;

    Faction faction;
    FideleType type;
    int hp;
    int currentHp;
    int range;
    Movement movement;

    Player owner;
    bool alive;
    Position currentPosition;
    Position deathPosition;
    bool skipNextAttack = false;
    bool isAsleep = false;
    bool emperorsGraceUsed = false;
};
