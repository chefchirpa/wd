#pragma once

#include <vector>
#include "Fidele.h"
#include "God.h"

class DeckBuilder {
public:
    static void launchInteractiveBuilder(const std::vector<Fidele>& allFideles, const std::vector<God>& allGods, Faction targetFaction);
};
