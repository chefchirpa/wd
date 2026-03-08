#include "Board.h"
#include <iostream>
#include <cmath>
#include <cstdlib> // For rand
#include <ctime>   // For time
#include <algorithm> // For std::find

Board::Board() : domaine1(Player::Player1), domaine2(Player::Player2) {
    std::srand(std::time(nullptr)); // Initialize random seed
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

bool Board::moveUnit(Fidele* fidele, const std::string& direction, int distance) {
    if (!fidele || !fidele->isAlive() || distance <= 0) {
        return false;
    }

    std::vector<Position> path;
    Position currentPos = fidele->getPosition();
    Player owner = fidele->getOwner();

    // Build the path step-by-step
    for (int i = 1; i <= distance; ++i) {
        Position nextPos = currentPos;

        // Map "Up", "Down", "Left", "Right" based on the player's perspective.
        // P1 starts at rows 0-2 (top), moves "Up/Forward" towards row 11 (+x).
        // P2 starts at rows 9-11 (bottom), moves "Up/Forward" towards row 0 (-x).
        if (owner == Player::Player1) {
            if (direction == "Up") nextPos.x += 1;
            else if (direction == "Down") nextPos.x -= 1;
            else if (direction == "Left") nextPos.y -= 1;
            else if (direction == "Right") nextPos.y += 1;
        } else if (owner == Player::Player2) {
            if (direction == "Up") nextPos.x -= 1;
            else if (direction == "Down") nextPos.x += 1;
            else if (direction == "Left") nextPos.y += 1;
            else if (direction == "Right") nextPos.y -= 1;
        }

        path.push_back(nextPos);
        currentPos = nextPos;
    }

    // Call existing moveFidele which validates the path using isValidMove
    // (checking stats limits, diagonals, and collisions)
    bool success = moveFidele(fidele, path);
    if (!success) {
        std::cout << "Action impossible : ce déplacement n'est pas autorisé par les cieux." << std::endl;
    }
    return success;
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

Domaine* Board::getDomaine(Player player) {
    if (player == Player::Player1) return &domaine1;
    if (player == Player::Player2) return &domaine2;
    return nullptr;
}

int Board::rollDice() const {
    static const int dieFaces[10] = {0, 1, 1, 2, 2, 3, 3, 4, 4, 5};
    int index = std::rand() % 10;
    return dieFaces[index];
}

bool Board::isInRange(Position p1, Position p2, int range) const {
    // Combat rules state attacks must be on the same row OR column (no diagonals)
    if (p1.x == p2.x) {
        return std::abs(p1.y - p2.y) <= range;
    } else if (p1.y == p2.y) {
        return std::abs(p1.x - p2.x) <= range;
    }
    return false;
}

void Board::attackFidele(Fidele* attacker, Fidele* defender) {
    if (!attacker || !defender || !attacker->isAlive() || !defender->isAlive()) return;
    if (attacker->getOwner() == defender->getOwner()) return; // Can't attack own

    if (!isInRange(attacker->getPosition(), defender->getPosition(), attacker->getRange())) {
        std::cout << "Attack failed: " << defender->getName() << " is out of range." << std::endl;
        return;
    }

    int attackRoll = rollDice();
    int defenseRoll = rollDice();

    int additionalAttackBonus = 0;
    attacker->applyOffensiveAbility(defender, additionalAttackBonus);

    int totalAttack = attackRoll + attacker->getAttackBonus() + additionalAttackBonus;
    int totalDefense = defenseRoll + defender->getDefenseBonus();

    int damage = totalAttack - totalDefense;
    if (damage < 0) damage = 0;

    std::cout << attacker->getName() << " lance l'assaut ! (Attaque totale: " << totalAttack << ", Défense totale: " << totalDefense << ")" << std::endl;

    if (damage > 0) {
        defender->takeDamage(damage);
        std::cout << "Dégâts infligés : " << damage << ". PV restants du défenseur : " << defender->getCurrentHP() << "." << std::endl;

        if (defender->getCurrentHP() <= 0) {
            killFidele(defender);
            Domaine* defenderDomaine = getDomaine(defender->getOwner());
            std::string domainName = (defender->getOwner() == Player::Player1) ? "Olympe" : "Panthéon";

            std::cout << defender->getName() << " est tombé au combat ! Le domaine " << domainName << " perd 1 PV." << std::endl;

            if (defenderDomaine) {
                defenderDomaine->takeDamage(1);
                if (defenderDomaine->getHP() <= 0) {
                    std::cout << "\n*** VICTOIRE ! ***\n";
                    std::cout << "Le domaine " << domainName << " a été détruit. Fin de la guerre !" << std::endl;
                }
            }
        }
    } else {
        std::cout << "L'attaque échoue ! " << defender->getName() << " bloque le coup sans subir de dégâts." << std::endl;
    }
}

void Board::attackDomaine(Fidele* attacker, Domaine* targetDomaine) {
    if (!attacker || !attacker->isAlive() || !targetDomaine) return;
    if (attacker->getOwner() == targetDomaine->getOwner()) return; // Can't attack own domain

    // Domain range check
    // Domain 1 is adjacent to x=0, Domain 2 is adjacent to x=LENGTH-1
    bool inRange = false;
    Position p = attacker->getPosition();
    int r = attacker->getRange();

    if (targetDomaine->getOwner() == Player::Player1) {
        // Domain 1 is at x < 0. Distance is simply attacker's x coordinate + 1
        if ((p.x + 1) <= r) inRange = true;
    } else if (targetDomaine->getOwner() == Player::Player2) {
        // Domain 2 is at x >= LENGTH. Distance is LENGTH - attacker's x coordinate
        if ((LENGTH - p.x) <= r) inRange = true;
    }

    if (!inRange) {
        std::cout << "Attack failed: Domain is out of range." << std::endl;
        return;
    }

    int attackRoll = rollDice();
    int totalAttack = attackRoll + attacker->getAttackBonus();

    std::string domainName = (targetDomaine->getOwner() == Player::Player1) ? "Olympe" : "Panthéon";

    // Domain has no defense, takes 100% damage
    std::cout << attacker->getName() << " lance l'assaut directement sur le domaine " << domainName << " !" << std::endl;

    targetDomaine->takeDamage(totalAttack);
    std::cout << "Dégâts infligés : " << totalAttack << ". PV restants du domaine " << domainName << " : " << targetDomaine->getHP() << "." << std::endl;

    if (targetDomaine->getHP() <= 0) {
        std::cout << "\n*** VICTOIRE ! ***\n";
        std::cout << "Le domaine " << domainName << " a été détruit. Fin de la guerre !" << std::endl;
    }
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
    fidele->resetHP();
    fidele->setPosition(deathPos);

    return true;
}

void Board::removeFideleFromGrid(Fidele* fidele) {
    if (!fidele) return;
    Position currentPos = fidele->getPosition();
    if (isWithinBounds(currentPos) && grid[currentPos.x][currentPos.y] == fidele) {
        grid[currentPos.x][currentPos.y] = nullptr;
    }
}

Position Board::getFirstEmptyStartRow(Player player) const {
    if (player == Player::Player1) {
        for (int x = 0; x < 3; ++x) {
            for (int y = 0; y < WIDTH; ++y) {
                if (grid[x][y] == nullptr) return {x, y};
            }
        }
    } else if (player == Player::Player2) {
        for (int x = LENGTH - 1; x >= LENGTH - 3; --x) {
            for (int y = 0; y < WIDTH; ++y) {
                if (grid[x][y] == nullptr) return {x, y};
            }
        }
    }
    return {-1, -1};
}

bool Board::placeNewFidele(Fidele* fidele, Position pos, Player player) {
    if (!fidele || !isWithinBounds(pos) || isOccupied(pos)) {
        return false;
    }

    if (player == Player::Player1 && pos.x > 2) {
        return false; // Must be in first 3 rows
    } else if (player == Player::Player2 && pos.x < LENGTH - 3) {
        return false; // Must be in first 3 rows
    }

    fidele->setOwner(player);
    grid[pos.x][pos.y] = fidele;
    fidele->setPosition(pos);
    fidele->setAlive(true);
    return true;
}

bool Board::placeInitialFidele(Fidele* fidele, Position pos, Player player) {
    if (!fidele || !isWithinBounds(pos) || isOccupied(pos)) {
        return false;
    }

    if (player == Player::Player1) {
        if (initialPlacementsP1 >= 3 || pos.x > 2) {
            std::cout << "Invalid placement: Player 1 can only place up to 3 units on rows 0-2." << std::endl;
            return false;
        }
        initialPlacementsP1++;
    } else if (player == Player::Player2) {
        if (initialPlacementsP2 >= 3 || pos.x < LENGTH - 3) {
            std::cout << "Invalid placement: Player 2 can only place up to 3 units on rows " << LENGTH - 3 << "-" << LENGTH - 1 << "." << std::endl;
            return false;
        }
        initialPlacementsP2++;
    } else {
        return false;
    }

    fidele->setOwner(player);
    grid[pos.x][pos.y] = fidele;
    fidele->setPosition(pos);
    fidele->setAlive(true);
    return true;
}

void Board::displayBoard() const {
    std::cout << "===========================================" << std::endl;
    std::cout << "               WAR DOLLS                   " << std::endl;
    std::cout << "===========================================" << std::endl;
    std::cout << " Olympe (P1) HP: " << domaine1.getHP() << " / 20" << std::endl;
    std::cout << " Panthéon (P2) HP: " << domaine2.getHP() << " / 20" << std::endl;
    std::cout << "===========================================" << std::endl;

    // Header Coordinates 1-9
    std::cout << "    1   2   3   4   5   6   7   8   9" << std::endl;

    for (int i = 0; i < LENGTH; ++i) {
        // Row letter A-L
        char rowLabel = 'A' + i;
        std::cout << " " << rowLabel << " ";

        for (int j = 0; j < WIDTH; ++j) {
            Fidele* f = grid[i][j];
            if (f == nullptr) {
                std::cout << "[.] ";
            } else {
                if (!f->isAlive()) {
                    std::cout << "[X] "; // Dead token
                } else {
                    if (f->getFaction() == Faction::Greek) {
                        std::cout << "\033[34m[G]\033[0m "; // Blue for Greek
                    } else if (f->getFaction() == Faction::Roman) {
                        std::cout << "\033[31m[R]\033[0m "; // Red for Roman
                    } else {
                        // Fallback if no faction
                        std::cout << (f->getOwner() == Player::Player1 ? "\033[34m[1]\033[0m " : "\033[31m[2]\033[0m ");
                    }
                }
            }
        }
        std::cout << std::endl;
    }
    // Footer Coordinates 1-9
    std::cout << "    1   2   3   4   5   6   7   8   9" << std::endl;

    std::cout << "-------------------------------------------" << std::endl;
}
