#pragma once

#include <string>
#include "Types.h"

enum class PowerType {
    Green,
    Red,
    Unknown
};

class God {
public:
    static Language currentLanguage;

    God(const std::string& nameEn, const std::string& nameFr,
        const std::string& abilityEn, const std::string& abilityFr,
        const std::string& descEn, const std::string& descFr,
        Faction faction, PowerType type)
        : nameEn(nameEn), nameFr(nameFr), abilityEn(abilityEn), abilityFr(abilityFr),
          descEn(descEn), descFr(descFr), faction(faction), powerType(type), played(false) {}

    std::string getName() const { return currentLanguage == Language::English ? nameEn : nameFr; }
    std::string getAbility() const { return currentLanguage == Language::English ? abilityEn : abilityFr; }
    std::string getDescription() const { return currentLanguage == Language::English ? descEn : descFr; }

    Faction getFaction() const { return faction; }
    PowerType getPowerType() const { return powerType; }

    bool hasBeenPlayed() const { return played; }
    void setPlayed(bool state) { played = state; }

private:
    std::string nameEn;
    std::string nameFr;
    std::string abilityEn;
    std::string abilityFr;
    std::string descEn;
    std::string descFr;

    Faction faction;
    PowerType powerType;
    bool played;
};
