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
    static bool useCelerisAirSupport(Board* board, Fidele* celeris, Fidele* allyTarget);
    static void useAmphisbaenaAttack(Board* board, Fidele* amphisbaena, Fidele* target1, Fidele* target2);
    static bool useParcaeThreadOfLife(Board* board, Fidele* parcae, Fidele* deadAlly);
    static bool useCaladriusBloodDonation(Fidele* caladrius, Fidele* allyTarget);

    // Post-Move Interrupts
    static void handlePostMoveInterrupts(Board* board, Fidele* movedUnit);

    // Helpers
    static void printAbilityTrigger(const Fidele* f);
};
