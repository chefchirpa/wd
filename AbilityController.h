#pragma once

#include "Board.h"
#include "Fidele.h"

class AbilityController {
public:
    // Passive Blockers
    static bool isProtectedByHector(Board* board, Fidele* target);

    // Active / Alternative Attack actions
    static bool useMoiraiThreadOfDeath(Board* board, Fidele* moirai, Fidele* target);
    static bool useTitanEarthquake(Board* board, Fidele* titan, Position tileTargetPos, std::vector<Position>& newPositions);
    static bool usePrometheusFire(Board* board, Fidele* prometheus, Fidele* allyTarget, Fidele* enemyTarget);

    // Post-Move Interrupts
    static void handlePostMoveInterrupts(Board* board, Fidele* movedUnit);

    // Helpers
    static void printAbilityTrigger(const Fidele* f);
};
