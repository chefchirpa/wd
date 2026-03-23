#pragma once

#include "God.h"
#include "Board.h"
#include <string>
#include <vector>

// Forward declaration
struct PlayerState;

class GodPowerManager {
public:
    static bool playGod(God* god, PlayerState& owner, Board& board, Player activePlayer, const std::string& targetName, std::vector<Fidele>& allFideles);
};
