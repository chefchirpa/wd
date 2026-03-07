#include "Board.h"
#include <iostream>
#include <cmath>

Board::Board() {
    for (int i = 0; i < LENGTH; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            grid[i][j] = nullptr;
        }
    }
}

bool Board::isWithinBounds(Position pos) const {
    return pos.x >= 0 && pos.x < LENGTH && pos.y >= 0 && pos.y < WIDTH;
}

bool Board::isOccupied(Position pos) const {
    if (!isWithinBounds(pos)) return true;
    return grid[pos.x][pos.y] != nullptr;
}

bool Board::placeFidele(Fidele* fidele, Position pos) {
    if (!fidele || !isWithinBounds(pos) || isOccupied(pos)) {
        return false;
    }

    grid[pos.x][pos.y] = fidele;
    fidele->setPosition(pos);
    fidele->setAlive(true);
    return true;
}

bool Board::moveFidele(Fidele* fidele, const std::vector<Position>& path) {
    if (!fidele || !fidele->isAlive() || path.empty()) {
        return false;
    }

    if (!isValidMove(fidele, path)) {
        return false;
    }

    Position startPos = fidele->getPosition();
    Position finalPos = path.back();

    // Remove from old position
    grid[startPos.x][startPos.y] = nullptr;

    // Place on new position
    grid[finalPos.x][finalPos.y] = fidele;
    fidele->setPosition(finalPos);

    return true;
}

bool Board::isValidMove(Fidele* fidele, const std::vector<Position>& path) const {
    if (!fidele || !fidele->isAlive() || path.empty()) {
        return false;
    }

    Position currentPos = fidele->getPosition();
    Position finalPos = path.back();

    // Cannot end movement on an occupied square (living or dead token)
    if (isOccupied(finalPos) && finalPos != currentPos) {
        return false;
    }

    Movement moveStats = fidele->getMovement();
    int forwardMoved = 0;
    int backwardMoved = 0;
    int leftMoved = 0;
    int rightMoved = 0;

    for (const auto& nextPos : path) {
        if (!isWithinBounds(nextPos)) {
            return false;
        }

        int dx = nextPos.x - currentPos.x;
        int dy = nextPos.y - currentPos.y;

        // Ensure moving exactly one square orthogonally per step in the path
        if (std::abs(dx) + std::abs(dy) != 1) {
            return false; // Diagonal or jumping
        }

        // Cannot pass over a living enemy Fidele
        Fidele* occ = grid[nextPos.x][nextPos.y];
        if (occ != nullptr && occ != fidele) {
            if (occ->isAlive() && occ->getOwner() != fidele->getOwner()) {
                return false;
            }
            // Passing over an allied Fidele or a dead token is allowed (but ending on it is checked above)
        }

        // Calculate directional movement based on player perspective
        // Assuming Player1 starts at x=0 (moving towards x=LENGTH-1)
        // Assuming Player2 starts at x=LENGTH-1 (moving towards x=0)

        if (fidele->getOwner() == Player::Player1) {
            if (dx == 1) forwardMoved++;
            else if (dx == -1) backwardMoved++;
            else if (dy == 1) rightMoved++;
            else if (dy == -1) leftMoved++;
        } else if (fidele->getOwner() == Player::Player2) {
            if (dx == -1) forwardMoved++;
            else if (dx == 1) backwardMoved++;
            else if (dy == -1) rightMoved++;
            else if (dy == 1) leftMoved++;
        }

        currentPos = nextPos;
    }

    // Ensure the total steps taken in each direction don't exceed the Fidele's stats
    if (forwardMoved > moveStats.forward ||
        backwardMoved > moveStats.backward ||
        leftMoved > moveStats.left ||
        rightMoved > moveStats.right) {
        return false;
    }

    return true;
}

bool Board::killFidele(Fidele* fidele) {
    if (!fidele || !fidele->isAlive()) {
        return false;
    }

    fidele->setAlive(false);
    fidele->setDeathPosition(fidele->getPosition());

    // According to rules: "retourner son pion... mais laisser ce pion sur la case où il a succombé"
    // The Fidele remains in the grid (as a dead token)
    return true;
}

bool Board::resurrectFidele(Fidele* fidele) {
    if (!fidele || fidele->isAlive()) {
        return false;
    }

    Position deathPos = fidele->getDeathPosition();

    // Check if death position is still occupied by something else
    // Since the dead token stays on the board, if grid[x][y] == fidele, it means it's just reviving itself.
    // If it's something else, then the spot is blocked, which the rules say we must handle,
    // but the basic rule is it revives where it died.
    if (grid[deathPos.x][deathPos.y] != fidele && grid[deathPos.x][deathPos.y] != nullptr) {
        // Technically, another piece could be standing on the dead token.
        // We'd need displacement logic, but for simplicity, fail if blocked by another Fidele.
        return false;
    }

    grid[deathPos.x][deathPos.y] = fidele;
    fidele->setAlive(true);
    fidele->setPosition(deathPos);

    return true;
}

void Board::printBoard() const {
    for (int i = 0; i < LENGTH; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            Fidele* f = grid[i][j];
            if (f == nullptr) {
                std::cout << ". ";
            } else {
                if (!f->isAlive()) {
                    std::cout << "x "; // Dead token
                } else if (f->getOwner() == Player::Player1) {
                    std::cout << "1 ";
                } else {
                    std::cout << "2 ";
                }
            }
        }
        std::cout << std::endl;
    }
}
