#pragma once

#include <vector>
#include "Fidele.h"
#include "Domaine.h"
#include "Types.h"

class Board {
public:
    static const int LENGTH = 12;
    static const int WIDTH = 9;

    Board();

    // Place a Fidele on the board at a specific position
    bool placeFidele(Fidele* fidele, Position pos);

    // Attempt to move a Fidele through a path of positions
    bool moveFidele(Fidele* fidele, const std::vector<Position>& path);

    // Attempt to move a unit interactively by specifying direction (Up, Down, Left, Right) and distance
    bool moveUnit(Fidele* fidele, const std::string& direction, int distance);

    // Verify if a move through a path is valid according to the rules
    bool isValidMove(Fidele* fidele, const std::vector<Position>& path) const;

    // Helper for grid context (like checking adjacent allies)
    Fidele* getFideleAt(Position pos) const;

    // Helper to determine if two positions are on the same 3x3 tile
    bool isSameTile(Position p1, Position p2) const;

    // Helper to check if a square is occupied by any Fidele (alive or dead)
    bool isOccupied(Position pos) const;

    // Helper to check standard combat range
    bool isInRange(Position p1, Position p2, int range) const;

    // Turn Modifiers state
    TurnModifiers turnMods;

    // Combat logic
    void attackFidele(Fidele* attacker, Fidele* defender);
    void attackDomaine(Fidele* attacker, Domaine* targetDomaine);

    // Advanced Ability: Mermaid Song
    void useMermaidSong(Fidele* mermaid, Fidele* target);

    // Advanced Ability: Cyclops (Roman) Column AOE Attack
    void executeCyclopsAoEAttack(Fidele* cyclope);

    // Get Domain for player
    Domaine* getDomaine(Player player);

    // Kill a Fidele, marking it dead and leaving its token on the board
    bool killFidele(Fidele* fidele);

    // Resurrect a Fidele at its death position
    bool resurrectFidele(Fidele* fidele);

    // Find first empty position in the player's starting rows
    Position getFirstEmptyStartRow(Player player) const;

    // Place a Fidele during setup (max 3 per player on their first 3 rows)
    bool placeInitialFidele(Fidele* fidele, Position pos, Player player);

    // Place a new Fidele from hand during the game (first 3 rows, no overall limit)
    bool placeNewFidele(Fidele* fidele, Position pos, Player player);

    // Print a visual representation of the board
    void displayBoard() const;

    // Remove a Fidele strictly from the board grid (used to bypass initial placement issues when forcing positions)
    void removeFideleFromGrid(Fidele* fidele);

    // Reset TurnModifiers and restore states (like Dionysus ownership)
    void resetTurnModifiers();

private:
    Domaine domaine1;
    Domaine domaine2;

    int initialPlacementsP1 = 0;
    int initialPlacementsP2 = 0;

    int rollDice() const;

    // 2D grid storing pointers to Fidele. A square can have a living Fidele,
    // or a dead Fidele token (which prevents stopping, but allows passing over).
    Fidele* grid[LENGTH][WIDTH];

    // Helper to check if a coordinate is within bounds
    bool isWithinBounds(Position pos) const;
};
