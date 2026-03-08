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
