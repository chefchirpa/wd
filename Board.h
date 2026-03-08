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

    // Verify if a move through a path is valid according to the rules
    bool isValidMove(Fidele* fidele, const std::vector<Position>& path) const;

    // Combat logic
    void attackFidele(Fidele* attacker, Fidele* defender);
    void attackDomaine(Fidele* attacker, Domaine* targetDomaine);

    // Get Domain for player
    Domaine* getDomaine(Player player);

    // Kill a Fidele, marking it dead and leaving its token on the board
    bool killFidele(Fidele* fidele);

    // Resurrect a Fidele at its death position
    bool resurrectFidele(Fidele* fidele);

    // Print a simple representation of the board for debugging
    void printBoard() const;

private:
    Domaine domaine1;
    Domaine domaine2;

    int rollDice() const;
    bool isInRange(Position p1, Position p2, int range) const;

    // 2D grid storing pointers to Fidele. A square can have a living Fidele,
    // or a dead Fidele token (which prevents stopping, but allows passing over).
    Fidele* grid[LENGTH][WIDTH];

    // Helper to check if a coordinate is within bounds
    bool isWithinBounds(Position pos) const;

    // Helper to check if a square is occupied by any Fidele (alive or dead)
    bool isOccupied(Position pos) const;
};
