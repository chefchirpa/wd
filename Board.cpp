#include "Board.h"
#include "AbilityController.h"
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

Fidele* Board::getFideleAt(Position pos) const {
    if (!isWithinBounds(pos)) return nullptr;
    return grid[pos.x][pos.y];
}

bool Board::isSameTile(Position p1, Position p2) const {
    // The board is made of 3x3 tiles.
    // Row 0, 1, 2 are Tile Row 0 (x/3 == 0).
    // Col 0, 1, 2 are Tile Col 0 (y/3 == 0).
    return (p1.x / 3 == p2.x / 3) && (p1.y / 3 == p2.y / 3);
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

void Board::resetTurnModifiers() {
    for (auto& pair : turnMods.dionysusMindControlled) {
        if (pair.first) {
            pair.first->setOwner(pair.second);
        }
    }
    turnMods.reset();
}

bool Board::moveUnit(Fidele* fidele, const std::string& direction, int distance) {
    if (!fidele || !fidele->isAlive() || distance <= 0) {
        return false;
    }

    if (fidele->getIsAsleep()) {
        std::cout << "Action impossible : " << fidele->getName() << " est endormi(e) et passe son tour.\n";
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
    } else {
        // Trigger: Mermaid Song Check "At the end of a MOVE action"
        if (fidele->getAbility() == "Song" || fidele->getAbility() == "Chant") {
            // Check if any enemy is in range
            Fidele* validTarget = nullptr;
            for (int i = 0; i < LENGTH; ++i) {
                for (int j = 0; j < WIDTH; ++j) {
                    Fidele* occ = grid[i][j];
                    if (occ && occ->isAlive() && occ->getOwner() != fidele->getOwner() && isInRange(fidele->getPosition(), occ->getPosition(), fidele->getRange())) {
                        validTarget = occ;
                        break;
                    }
                }
                if (validTarget) break;
            }

            if (validTarget) {
                // Interactive prompt logic. For automated testing, we simulate picking 'Y'.
                std::cout << "\n[PROMPT] " << fidele->getName() << " a fini son déplacement. Un ennemi (" << validTarget->getName() << ") est à portée.\n";
                std::cout << "Utiliser le Chant de la Sirène au lieu d'attaquer ? (O/N) : O (Simulé)\n";

                // Trigger the ability
                useMermaidSong(fidele, validTarget);
            }
        }

        // Trigger: Check for Interrupts triggered by an enemy moving (Theseus, Odysseus)
        AbilityController::handlePostMoveInterrupts(this, fidele);
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

    // Scan for Romulus & Remus fortification globally (since checking path requires tile logic)
    std::vector<Position> fortificationTiles;
    for (int i = 0; i < LENGTH; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            Fidele* occ = grid[i][j];
            if (occ && occ->isAlive() && occ->getOwner() != fidele->getOwner()) {
                if (occ->getAbility() == "Fortification" || occ->getAbility() == "Muraille") {
                    fortificationTiles.push_back(occ->getPosition());
                }
            }
        }
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

        // Romulus & Remus: check if nextPos enters their tile
        for (auto& rPos : fortificationTiles) {
            if (isSameTile(nextPos, rPos)) {
                std::cout << "Action impossible : la Muraille de Romulus & Rémus bloque l'accès à cette tuile.\n";
                return false;
            }
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

bool isAmazonTargetingValid(Position attacker, Position target) {
    // Amazon can attack anyone on her tile or the 4 directly adjacent 3x3 tiles
    int aTileX = attacker.x / 3;
    int aTileY = attacker.y / 3;
    int tTileX = target.x / 3;
    int tTileY = target.y / 3;

    if (aTileX == tTileX && aTileY == tTileY) return true; // Same tile
    if (aTileX == tTileX && std::abs(aTileY - tTileY) == 1) return true; // Adjacent horizontally
    if (aTileY == tTileY && std::abs(aTileX - tTileX) == 1) return true; // Adjacent vertically

    return false;
}

void Board::attackFidele(Fidele* attacker, Fidele* defender) {
    if (!attacker || !defender || !attacker->isAlive() || !defender->isAlive()) return;
    if (attacker->getOwner() == defender->getOwner()) return; // Can't attack own

    if (attacker->getSkipNextAttack()) {
        std::cout << attacker->getName() << " a consommé son action en interceptant (Cheval de Troie) et ne peut pas attaquer ce tour-ci.\n";
        attacker->setSkipNextAttack(false); // consume flag
        return;
    }

    if (attacker->getIsAsleep()) {
        std::cout << attacker->getName() << " est endormi et ne peut pas attaquer.\n";
        return;
    }

    if (AbilityController::isProtectedByHector(this, defender)) {
        return; // Hector blocks the attack execution entirely
    }

    bool attackInRange = false;
    // Giant already handled above
    if (defender->getAbility() == "Cuirassier") {
        attackInRange = isInRange(attacker->getPosition(), defender->getPosition(), attacker->getRange());
    } else {
        if (attacker->getAbility() == "Archery" || attacker->getAbility() == "Tir à l'arc") {
            attackInRange = isAmazonTargetingValid(attacker->getPosition(), defender->getPosition());
        } else if (attacker->getAbility() == "Imperial sword" || attacker->getAbility() == "Glaive impérial") {
            // Roman Soldier: Can attack diagonally (Distance = 1)
            int dx = std::abs(attacker->getPosition().x - defender->getPosition().x);
            int dy = std::abs(attacker->getPosition().y - defender->getPosition().y);
            if ((dx == 1 && dy == 1) || (dx + dy == 1)) {
                attackInRange = true;
            }
        } else {
            attackInRange = isInRange(attacker->getPosition(), defender->getPosition(), attacker->getRange());
        }
    }

    if (!attackInRange) {
        std::cout << "L'attaque échoue : cible hors de portée." << std::endl;
        return;
    }

    // Minerva Immunity Check
    for (Fidele* f : turnMods.minervaImmuneFideles) {
        if (f == defender) {
            std::cout << "-> " << defender->getName() << " est protégé par l'Égide de Minerve et ne peut être blessé ce tour !\n";
            return;
        }
    }

    int attackRoll = rollDice();
    if ((attacker->getAbility() == "Overpower" || attacker->getAbility() == "Surpuissance") &&
        !(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != attacker->getOwner())) {
        int roll2 = rollDice();
        attackRoll = std::max(attackRoll, roll2);
        AbilityController::printAbilityTrigger(attacker);
        std::cout << "-> Hercule garde le meilleur jet : " << attackRoll << " !\n";
    }

    if (turnMods.vulcanDoubleDice && turnMods.activeGodPlayer == attacker->getOwner()) {
        int roll2 = rollDice();
        attackRoll = std::max(attackRoll, roll2);
        std::cout << "-> Vulcain offre un 2e jet d'attaque ! (" << attackRoll << " retenu)\n";
    }

    int defenseRoll = rollDice();
    if (turnMods.vulcanDoubleDice && turnMods.activeGodPlayer == defender->getOwner()) {
        int roll2 = rollDice();
        defenseRoll = std::max(defenseRoll, roll2);
        std::cout << "-> Vulcain offre un 2e jet de défense ! (" << defenseRoll << " retenu)\n";
    }

    int additionalAttackBonus = 0;
    if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != attacker->getOwner())) {
        attacker->applyOffensiveAbility(defender, additionalAttackBonus);
    }

    int additionalDefenseBonus = 0;
    if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != defender->getOwner())) {
        defender->applyDefensiveAbility(this, attacker, additionalDefenseBonus);
    }

    // Heracles Check: Resistant
    int oppDefBonus = defender->getDefenseBonus();
    int oppAtkBonus = attacker->getAttackBonus();
    if (attacker->getAbility() == "Resistant" || attacker->getAbility() == "Résistant") {
        oppDefBonus = 0;
    }
    if (defender->getAbility() == "Resistant" || defender->getAbility() == "Résistant") {
        oppAtkBonus = 0;
    }

    int totalAttack = attackRoll + oppAtkBonus + additionalAttackBonus;
    int totalDefense = defenseRoll + oppDefBonus + additionalDefenseBonus;

    // Cupid Check: Charm (-1 to opponent's die result, simulated by deducting 1 from total before damage if possible, or modifying raw stat)
    if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != attacker->getOwner()) && (attacker->getAbility() == "Charm" || attacker->getAbility() == "Charme")) {
        totalDefense -= 1;
        if (totalDefense < 0) totalDefense = 0;
        AbilityController::printAbilityTrigger(attacker);
        std::cout << "-> Charme actif : Défense réduite à " << totalDefense << " !\n";
    }
    if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != defender->getOwner()) && (defender->getAbility() == "Charm" || defender->getAbility() == "Charme")) {
        totalAttack -= 1;
        if (totalAttack < 0) totalAttack = 0;
        AbilityController::printAbilityTrigger(defender);
        std::cout << "-> Charme actif : Attaque réduite à " << totalAttack << " !\n";
    }

    int damage = totalAttack - totalDefense;
    if (damage < 0) damage = 0;

    if (turnMods.aresDoubleDamage && turnMods.activeGodPlayer == attacker->getOwner()) {
        damage *= 2;
        std::cout << "-> Arès double les dégâts infligés (" << damage << ") !\n";
    }

    // Lemures Check: Coup de grace (Immune to exactly 1 damage)
    if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != defender->getOwner()) && (defender->getAbility() == "Coup de grace" || defender->getAbility() == "Coup de grâce")) {
        if (damage == 1) {
            AbilityController::printAbilityTrigger(defender);
            std::cout << "-> Les Lémures ignorent l'attaque de 1 dégât !\n";
            damage = 0;
        }
    }

    std::cout << attacker->getName() << " lance l'assaut ! (Attaque totale: " << totalAttack << ", Défense totale: " << totalDefense << ")" << std::endl;

    if (damage > 0) {
        // The Wolf Check: Mother instinct
        Fidele* wolf = nullptr;
        if (defender->getType() == FideleType::Heros) {
            for (int i = 0; i < LENGTH; ++i) {
                for (int j = 0; j < WIDTH; ++j) {
                    Fidele* occ = grid[i][j];
                    if (occ && occ->isAlive() && occ->getOwner() == defender->getOwner() &&
                        (occ->getAbility() == "Mother instinct" || occ->getAbility() == "Instinct maternel")) {
                        wolf = occ;
                        break;
                    }
                }
                if (wolf) break;
            }
        }

        if (wolf) {
            AbilityController::printAbilityTrigger(wolf);
            std::cout << "[PROMPT] La Louve veut prendre les dégâts à la place de " << defender->getName() << ". (O/N) : O (Simulé)\n";
            wolf->takeDamage(damage);
            std::cout << "Dégâts infligés à La Louve : " << damage << ". PV restants : " << wolf->getCurrentHP() << ".\n";

            if (wolf->getCurrentHP() <= 0) {
                killFidele(wolf);
                Domaine* wolfDom = getDomaine(wolf->getOwner());
                std::string domainName = (wolf->getOwner() == Player::Player1) ? "Olympe" : "Panthéon";
                std::cout << wolf->getName() << " est tombé au combat ! Le domaine " << domainName << " perd 1 PV." << std::endl;
                if (wolfDom) {
                    wolfDom->takeDamage(1);
                    if (wolfDom->getHP() <= 0) {
                        std::cout << "\n*** VICTOIRE ! ***\nLe domaine " << domainName << " a été détruit. Fin de la guerre !\n";
                    }
                }
            }
        } else {
            int preHitHp = defender->getCurrentHP();
            defender->takeDamage(damage);

            // Gladiator Check: Emperor's Grace
            if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != defender->getOwner()) &&
                defender->getCurrentHP() <= 0 && !defender->getEmperorsGraceUsed() &&
                (defender->getAbility() == "Emperor's Grace" || defender->getAbility() == "Grâce de l'Empereur")) {

                AbilityController::printAbilityTrigger(defender);
                std::cout << "-> Le Gladiateur survit miraculeusement avec 1 PV !\n";
                defender->setHP(1);
                defender->setEmperorsGraceUsed(true);
            }

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
        }


        // Ceryneian Hind Elusiveness Check
        if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != defender->getOwner()) &&
            defender->isAlive() && (defender->getAbility() == "Elusive" || defender->getAbility() == "Insaisissable")) {
            Language prevLang = Fidele::currentLanguage;
            Fidele::currentLanguage = Language::French;
            std::cout << "\n>>> POUVOIR ACTIVÉ : " << defender->getName() << " - " << defender->getAbility() << " <<<\n";
            std::cout << "Effet : " << defender->getDescription() << "\n";
            std::cout << "-> Action accordée : Déplacement gratuit pour " << defender->getName() << " !\n";
            Fidele::currentLanguage = prevLang;

            // Execute the free move immediately. In an interactive console game, this would loop via std::cin.
            // For testing, we will simulate a backward dash of 1 if permitted by its stats.
            std::string escapeDir = (defender->getOwner() == Player::Player1) ? "Down" : "Up"; // Move away
            std::cout << "[PROMPT] Entrez la direction de fuite (Up, Down, Left, Right) et la distance : " << escapeDir << " 1 (Simulé)\n";
            bool escapeSuccess = moveUnit(defender, escapeDir, 1);
            if (!escapeSuccess) {
                // If backward is blocked, try side
                escapeSuccess = moveUnit(defender, "Left", 1);
            }
            if (escapeSuccess) {
                std::cout << "-> " << defender->getName() << " a fui vers la case (" << defender->getPosition().x << "," << defender->getPosition().y << ") !\n\n";
            } else {
                std::cout << "-> La fuite a échoué (chemin bloqué).\n\n";
            }
        }

        // Chimera Deflagration Check
        if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != attacker->getOwner()) &&
            (attacker->getAbility() == "Deflagration" || attacker->getAbility() == "Déflagration")) {
            Language prevLang = Fidele::currentLanguage;
            Fidele::currentLanguage = Language::French;
            std::cout << "\n>>> POUVOIR ACTIVÉ : " << attacker->getName() << " - " << attacker->getAbility() << " <<<\n";
            std::cout << "Effet : " << attacker->getDescription() << "\n";
            Fidele::currentLanguage = prevLang;

            Position targetPos = defender->getPosition();

            // Apply 1 damage to all OTHER enemies on the SAME TILE
            for (int i = 0; i < LENGTH; ++i) {
                for (int j = 0; j < WIDTH; ++j) {
                    Fidele* occ = grid[i][j];
                    if (occ && occ->isAlive() && occ != defender && occ->getOwner() == defender->getOwner()) {
                        if (isSameTile(occ->getPosition(), targetPos)) {
                            if (occ->isImmuneToAbilities()) {
                                std::cout << "-> " << occ->getName() << " est immunisé contre les capacités spéciales (Cuirassier) et ignore les brûlures !\n";
                            } else {
                                std::cout << "-> " << occ->getName() << " subit 1 dégât de brûlure !\n";
                                occ->takeDamage(1);

                                if (occ->getCurrentHP() <= 0) {
                                    killFidele(occ);
                                    std::string dName = (occ->getOwner() == Player::Player1) ? "Olympe" : "Panthéon";
                                    std::cout << "-> " << occ->getName() << " a succombé aux brûlures ! Le domaine " << dName << " perd 1 PV.\n";
                                    Domaine* dom = getDomaine(occ->getOwner());
                                    if (dom) {
                                        dom->takeDamage(1);
                                        if (dom->getHP() <= 0) {
                                            std::cout << "\n*** VICTOIRE ! ***\nLe domaine " << dName << " a été détruit. Fin de la guerre !\n";
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
            std::cout << "\n";
        }
    } else {
        std::cout << "L'attaque échoue ! " << defender->getName() << " bloque le coup sans subir de dégâts." << std::endl;

        // Morpheus Check: Sandman
        if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != defender->getOwner()) &&
            (defender->getAbility() == "Sandman" || defender->getAbility() == "Marchand de sable")) {
            AbilityController::printAbilityTrigger(defender);
            std::cout << "-> " << attacker->getName() << " s'endort !\n";
            attacker->setAsleep(true);
        }

        // Ceryneian Hind Elusiveness Check (still triggers if attack misses, as per "after being targeted")
        if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != defender->getOwner()) &&
            defender->isAlive() && (defender->getAbility() == "Elusive" || defender->getAbility() == "Insaisissable")) {
            Language prevLang = Fidele::currentLanguage;
            Fidele::currentLanguage = Language::French;
            std::cout << "\n>>> POUVOIR ACTIVÉ : " << defender->getName() << " - " << defender->getAbility() << " <<<\n";
            std::cout << "Effet : " << defender->getDescription() << "\n";
            std::cout << "-> Action accordée : Déplacement gratuit pour " << defender->getName() << " !\n";
            Fidele::currentLanguage = prevLang;

            std::string escapeDir = (defender->getOwner() == Player::Player1) ? "Down" : "Up"; // Move away
            std::cout << "[PROMPT] Entrez la direction de fuite (Up, Down, Left, Right) et la distance : " << escapeDir << " 1 (Simulé)\n";
            bool escapeSuccess = moveUnit(defender, escapeDir, 1);
            if (!escapeSuccess) {
                escapeSuccess = moveUnit(defender, "Left", 1);
            }
            if (escapeSuccess) {
                std::cout << "-> " << defender->getName() << " a fui vers la case (" << defender->getPosition().x << "," << defender->getPosition().y << ") !\n\n";
            } else {
                std::cout << "-> La fuite a échoué (chemin bloqué).\n\n";
            }
        }
    }

    if (damage > 0 && attacker->isAlive() && (attacker->getAbility() == "Strategic retreat" || attacker->getAbility() == "Repli stratégique")) {
        AbilityController::printAbilityTrigger(attacker);
        std::cout << "-> Action accordée : Repli stratégique gratuit pour " << attacker->getName() << " !\n";

        std::string retreatDir = (attacker->getOwner() == Player::Player1) ? "Down" : "Up";
        std::cout << "[PROMPT] Entrez la direction de fuite (Up, Down, Left, Right) et la distance : " << retreatDir << " 1 (Simulé)\n";
        bool retreatSuccess = moveUnit(attacker, retreatDir, 1);
        if (!retreatSuccess) {
            retreatSuccess = moveUnit(attacker, "Right", 1);
        }
        if (retreatSuccess) {
            std::cout << "-> " << attacker->getName() << " s'est replié vers la case (" << attacker->getPosition().x << "," << attacker->getPosition().y << ") !\n\n";
        } else {
            std::cout << "-> Le repli a échoué (chemin bloqué).\n\n";
        }
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

    for (Domaine* d : turnMods.minervaImmuneDomaines) {
        if (d == targetDomaine) {
            std::cout << "-> Le domaine est protégé par l'Égide de Minerve et ne peut être endommagé !\n";
            return;
        }
    }

    int attackRoll = rollDice();

    if (turnMods.vulcanDoubleDice && turnMods.activeGodPlayer == attacker->getOwner()) {
        int roll2 = rollDice();
        attackRoll = std::max(attackRoll, roll2);
        std::cout << "-> Vulcain offre un 2e jet d'attaque ! (" << attackRoll << " retenu)\n";
    }

    int additionalAttackBonus = 0;
    if (!(turnMods.neptuneNullifyEnemyAbilities && turnMods.activeGodPlayer != attacker->getOwner())) {
        attacker->applyOffensiveAbility(nullptr, additionalAttackBonus);
    }

    int totalAttack = attackRoll + attacker->getAttackBonus() + additionalAttackBonus;

    if (turnMods.aresDoubleDamage && turnMods.activeGodPlayer == attacker->getOwner()) {
        totalAttack *= 2;
        std::cout << "-> Arès double les dégâts infligés au domaine (" << totalAttack << ") !\n";
    }

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

void Board::useMermaidSong(Fidele* mermaid, Fidele* target) {
    if (!mermaid || !target || !mermaid->isAlive() || !target->isAlive()) return;
    if (mermaid->getAbility() != "Song" && mermaid->getAbility() != "Chant") return;

    if (target->isImmuneToAbilities()) {
        std::cout << target->getName() << " (Géant) est immunisé contre le Chant de la Sirène.\n";
        return;
    }

    if (!isInRange(mermaid->getPosition(), target->getPosition(), mermaid->getRange())) {
        std::cout << "Le chant de la Sirène échoue : cible hors de portée." << std::endl;
        return;
    }

    Language prevLang = Fidele::currentLanguage;
    Fidele::currentLanguage = Language::French;
    std::cout << "\n>>> POUVOIR ACTIVÉ : " << mermaid->getName() << " - " << mermaid->getAbility() << " <<<\n";
    std::cout << "Effet : " << mermaid->getDescription() << "\n";
    Fidele::currentLanguage = prevLang;

    Position mPos = mermaid->getPosition();
    Position tPos = target->getPosition();
    Position newPos = tPos;

    // Pull 1 square closer
    if (mPos.x == tPos.x) {
        if (tPos.y > mPos.y) newPos.y -= 1;
        else if (tPos.y < mPos.y) newPos.y += 1;
    } else if (mPos.y == tPos.y) {
        if (tPos.x > mPos.x) newPos.x -= 1;
        else if (tPos.x < mPos.x) newPos.x += 1;
    }

    if (!isOccupied(newPos)) {
        // Move the target
        grid[tPos.x][tPos.y] = nullptr;
        grid[newPos.x][newPos.y] = target;
        target->setPosition(newPos);
        std::cout << "-> " << target->getName() << " est attiré(e) en (" << newPos.x << "," << newPos.y << ").\n";

        // Check if now adjacent (distance == 1)
        int dist = std::abs(mPos.x - newPos.x) + std::abs(mPos.y - newPos.y);
        if (dist == 1) {
            std::cout << "-> " << target->getName() << " finit adjacent(e) à la Sirène et subit 2 points de dégâts purs !\n";
            target->takeDamage(2);

            if (target->getCurrentHP() <= 0) {
                killFidele(target);
                Domaine* defenderDomaine = getDomaine(target->getOwner());
                std::string domainName = (target->getOwner() == Player::Player1) ? "Olympe" : "Panthéon";
                std::cout << target->getName() << " est tombé au combat ! Le domaine " << domainName << " perd 1 PV." << std::endl;
                if (defenderDomaine) {
                    defenderDomaine->takeDamage(1);
                    if (defenderDomaine->getHP() <= 0) {
                        std::cout << "\n*** VICTOIRE ! ***\nLe domaine " << domainName << " a été détruit. Fin de la guerre !\n";
                    }
                }
            } else {
                std::cout << "PV restants de " << target->getName() << " : " << target->getCurrentHP() << ".\n";
            }
        }
    } else {
        std::cout << "-> L'attraction échoue car la case de destination est bloquée.\n";
    }
}

void Board::executeCyclopsAoEAttack(Fidele* cyclope) {
    if (!cyclope || !cyclope->isAlive()) return;

    // Find all valid targets in the same column strictly in front of the Cyclops within his Range
    std::vector<Fidele*> targets;
    Position cPos = cyclope->getPosition();
    Player owner = cyclope->getOwner();
    int range = cyclope->getRange();

    for (int i = 1; i <= range; ++i) {
        Position checkPos = cPos;
        if (owner == Player::Player1) {
            checkPos.x += i;
        } else if (owner == Player::Player2) {
            checkPos.x -= i;
        }

        if (isWithinBounds(checkPos)) {
            Fidele* occ = grid[checkPos.x][checkPos.y];
            if (occ && occ->isAlive() && occ->getOwner() != owner) {
                targets.push_back(occ);
            }
        }
    }

    if (targets.empty()) {
        std::cout << "Aucune cible dans la ligne de mire du Cyclope." << std::endl;
        return;
    }

    Language prevLang = Fidele::currentLanguage;
    Fidele::currentLanguage = Language::French;
    std::cout << "\n>>> POUVOIR ACTIVÉ : " << cyclope->getName() << " - " << cyclope->getAbility() << " <<<\n";
    std::cout << "Effet : " << cyclope->getDescription() << "\n";
    Fidele::currentLanguage = prevLang;

    // Roll attack ONCE for the Cyclops
    int attackRoll = rollDice();
    int additionalAttackBonus = 0;
    cyclope->applyOffensiveAbility(targets[0], additionalAttackBonus); // Just dummy check for the +1 if we kept it for Roman

    int totalAttack = attackRoll + cyclope->getAttackBonus() + additionalAttackBonus;
    std::cout << cyclope->getName() << " déclenche son Rayon Optique de zone ! (Attaque globale: " << totalAttack << ")\n";

    // Resolve against all targets
    for (Fidele* target : targets) {
        if (target->getAbility() == "Cuirassier") {
            std::cout << "-> " << target->getName() << " (Géant) est immunisé contre le rayon optique !\n";
            continue;
        }

        int defenseRoll = rollDice();
        int additionalDefenseBonus = 0;
        target->applyDefensiveAbility(this, cyclope, additionalDefenseBonus);

        int oppDefBonus = target->getDefenseBonus();
        int totalDefense = defenseRoll + oppDefBonus + additionalDefenseBonus;

        int damage = totalAttack - totalDefense;
        if (damage < 0) damage = 0;

        std::cout << "-> Cible: " << target->getName() << " | Défense totale: " << totalDefense;

        if (damage > 0) {
            target->takeDamage(damage);
            std::cout << " | Dégâts reçus : " << damage << ". PV restants : " << target->getCurrentHP() << ".\n";

            if (target->getCurrentHP() <= 0) {
                killFidele(target);
                Domaine* defenderDomaine = getDomaine(target->getOwner());
                std::string domainName = (target->getOwner() == Player::Player1) ? "Olympe" : "Panthéon";
                std::cout << "   " << target->getName() << " a été pulvérisé ! Le domaine " << domainName << " perd 1 PV.\n";
                if (defenderDomaine) {
                    defenderDomaine->takeDamage(1);
                    if (defenderDomaine->getHP() <= 0) {
                        std::cout << "\n*** VICTOIRE ! ***\nLe domaine " << domainName << " a été détruit. Fin de la guerre !\n";
                    }
                }
            }
        } else {
            std::cout << " | Bloqué ! Aucun dégât.\n";
        }
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

    // Blockers check: Cerberus & Proserpina
    for (int i = 0; i < LENGTH; ++i) {
        for (int j = 0; j < WIDTH; ++j) {
            Fidele* occ = grid[i][j];
            if (occ && occ->isAlive() && occ->getOwner() != fidele->getOwner()) {

                // Cerberus
                if (occ->getAbility() == "Guardian of the underworld" || occ->getAbility() == "Gardien des enfers") {
                    if (isSameTile(occ->getPosition(), deathPos)) {
                        AbilityController::printAbilityTrigger(occ);
                        std::cout << "-> Résurrection de " << fidele->getName() << " annulée !\n\n";
                        return false;
                    }
                }

                // Proserpina
                if (occ->getAbility() == "Queen of Hell" || occ->getAbility() == "Reine des enfers") {
                    if (occ->getPosition().y == deathPos.y) {
                        AbilityController::printAbilityTrigger(occ);
                        std::cout << "-> Proserpine bloque la résurrection de " << fidele->getName() << " sur cette colonne !\n\n";
                        return false;
                    }
                }
            }
        }
    }

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
