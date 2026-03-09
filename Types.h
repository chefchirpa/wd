#pragma once

enum class Player {
    Player1,
    Player2,
    None
};

enum class Language {
    English,
    French
};

enum class Faction {
    Greek,
    Roman,
    None
};

struct Position {
    int x;
    int y;

    bool operator==(const Position& other) const {
        return x == other.x && y == other.y;
    }
    bool operator!=(const Position& other) const {
        return !(*this == other);
    }
};

#include <vector>
#include <deque>

// Forward declarations to allow struct PlayerState to compile cleanly
class Fidele;
class God;

struct PlayerState {
    std::deque<Fidele*> deck;
    std::vector<Fidele*> hand;
    std::vector<God*> gods;
    Player playerId;
};

class Domaine;

struct TurnModifiers {
    bool aresDoubleDamage = false;
    bool vulcanDoubleDice = false;
    Player activeGodPlayer = Player::None; // The player who cast the turn-wide God

    bool neptuneNullifyEnemyAbilities = false;

    std::vector<Fidele*> minervaImmuneFideles;
    std::vector<Domaine*> minervaImmuneDomaines;

    std::vector<std::pair<Fidele*, Player>> dionysusMindControlled; // <Target, OriginalOwner>

    void reset() {
        aresDoubleDamage = false;
        vulcanDoubleDice = false;
        activeGodPlayer = Player::None;
        neptuneNullifyEnemyAbilities = false;
        minervaImmuneFideles.clear();
        minervaImmuneDomaines.clear();

        // (Note: Restore ownership logic is now handled in Board.cpp or main.cpp since Fidele is incomplete here)
        dionysusMindControlled.clear();
    }
};
