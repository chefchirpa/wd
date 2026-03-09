#include "AbilityController.h"
#include <iostream>
#include <cmath>

void AbilityController::printAbilityTrigger(const Fidele* f) {
    Language prevLang = Fidele::currentLanguage;
    Fidele::currentLanguage = Language::French;
    std::cout << "\n>>> POUVOIR ACTIVÉ : " << f->getName() << " - " << f->getAbility() << " <<<\n";
    std::cout << "Effet : " << f->getDescription() << "\n";
    Fidele::currentLanguage = prevLang;
}

bool AbilityController::isProtectedByHector(Board* board, Fidele* target) {
    if (target && (target->getAbility() == "Cuirassier" || target->getAbility() == "Cuirassier")) return false; // Giant immune to abilities
    if (!target || !target->isAlive()) return false;

    // Scan for Hector
    for (int i = 0; i < board->LENGTH; ++i) {
        for (int j = 0; j < board->WIDTH; ++j) {
            Fidele* occ = board->getFideleAt({i, j});
            if (occ && occ->isAlive() && occ->getOwner() == target->getOwner() && occ != target) {
                if (occ->getAbility() == "Human shield" || occ->getAbility() == "Bouclier humain") {
                    // Check cross-shape distance <= 2
                    Position hPos = occ->getPosition();
                    Position tPos = target->getPosition();

                    if ((hPos.x == tPos.x && std::abs(hPos.y - tPos.y) <= 2) ||
                        (hPos.y == tPos.y && std::abs(hPos.x - tPos.x) <= 2)) {

                        printAbilityTrigger(occ);
                        std::cout << "-> " << target->getName() << " est protégé par Hector et ne peut pas être attaqué !\n\n";
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

bool AbilityController::useMoiraiThreadOfDeath(Board* board, Fidele* moirai, Fidele* target) {
    if (!moirai || !target || !moirai->isAlive() || !target->isAlive()) return false;
    if (moirai->getAbility() != "Thread of life" && moirai->getAbility() != "Fil de mort" && moirai->getAbility() != "Thread of death") return false;

    if (target->isImmuneToAbilities()) {
        std::cout << target->getName() << " (Géant) est immunisé contre le Fil de mort !\n";
        return false;
    }

    if (target->getCurrentHP() == 1) {
        Position mPos = moirai->getPosition();
        Position tPos = target->getPosition();
        if (mPos.x == tPos.x || mPos.y == tPos.y) {
            printAbilityTrigger(moirai);
            std::cout << "-> Les Moires coupent le fil de vie de " << target->getName() << " !\n";
            target->takeDamage(1); // Execute

            board->killFidele(target);
            Domaine* dom = board->getDomaine(target->getOwner());
            std::string domName = (target->getOwner() == Player::Player1) ? "Olympe" : "Panthéon";
            std::cout << target->getName() << " est tombé au combat ! Le domaine " << domName << " perd 1 PV.\n";
            if (dom) {
                dom->takeDamage(1);
                if (dom->getHP() <= 0) {
                    std::cout << "\n*** VICTOIRE ! ***\nLe domaine " << domName << " a été détruit. Fin de la guerre !\n";
                }
            }
            return true;
        }
    }
    std::cout << "Action impossible : La cible n'est pas valide pour le fil de mort.\n";
    return false;
}

bool AbilityController::useTitanEarthquake(Board* board, Fidele* titan, Position tileTargetPos, std::vector<Position>& newPositions) {
    if (!titan || !titan->isAlive()) return false;
    if (titan->getAbility() != "Earthquake" && titan->getAbility() != "Séisme") return false;

    printAbilityTrigger(titan);

    // Find all living units on the target 3x3 tile (excluding immune Giants)
    std::vector<Fidele*> targets;
    for (int i = 0; i < board->LENGTH; ++i) {
        for (int j = 0; j < board->WIDTH; ++j) {
            Fidele* occ = board->getFideleAt({i, j});
            if (occ && occ->isAlive() && board->isSameTile(tileTargetPos, occ->getPosition())) {
                if (!occ->isImmuneToAbilities()) {
                    targets.push_back(occ);
                }
            }
        }
    }

    if (targets.size() != newPositions.size()) {
        std::cout << "Erreur: Le nombre de nouvelles positions ne correspond pas au nombre d'unités sur la tuile.\n";
        return false;
    }

    // Process movements
    for (size_t i = 0; i < targets.size(); ++i) {
        Fidele* t = targets[i];
        Position nPos = newPositions[i];

        // Ensure new position is on the same tile and empty (or moving to its own spot)
        if (board->isSameTile(tileTargetPos, nPos) && (!board->isOccupied(nPos) || t->getPosition() == nPos)) {
            // Unlink old
            board->removeFideleFromGrid(t);
            // Relink new
            board->placeFidele(t, nPos);
            std::cout << "-> " << t->getName() << " a été secoué et déplacé vers (" << nPos.x << "," << nPos.y << ")\n";
        }
    }
    return true;
}

bool AbilityController::usePrometheusFire(Board* board, Fidele* prometheus, Fidele* allyTarget, Fidele* enemyTarget) {
    if (!prometheus || !allyTarget || !enemyTarget || !prometheus->isAlive() || !allyTarget->isAlive() || !enemyTarget->isAlive()) return false;
    if (prometheus->getAbility() != "Fire Bringer" && prometheus->getAbility() != "Transmetteur de feu") return false;

    if (prometheus->getOwner() == allyTarget->getOwner() && board->isInRange(prometheus->getPosition(), allyTarget->getPosition(), prometheus->getRange())) {
        printAbilityTrigger(prometheus);
        std::cout << "-> " << prometheus->getName() << " transmet son tour d'attaque à " << allyTarget->getName() << " !\n";

        // Execute the attack on behalf of the ally
        board->attackFidele(allyTarget, enemyTarget);
        return true;
    }
    std::cout << "Action impossible : Cible invalide pour le don de feu.\n";
    return false;
}

bool AbilityController::useCelerisAirSupport(Board* board, Fidele* celeris, Fidele* allyTarget) {
    if (!celeris || !allyTarget || !celeris->isAlive() || !allyTarget->isAlive()) return false;

    printAbilityTrigger(celeris);
    Position cPos = celeris->getPosition();
    Position spawnOpts[4] = { {cPos.x+1, cPos.y}, {cPos.x-1, cPos.y}, {cPos.x, cPos.y+1}, {cPos.x, cPos.y-1} };

    for (int i = 0; i < 4; ++i) {
        if (!board->isOccupied(spawnOpts[i]) && spawnOpts[i].x >= 0 && spawnOpts[i].x < Board::LENGTH && spawnOpts[i].y >= 0 && spawnOpts[i].y < Board::WIDTH) {
            board->removeFideleFromGrid(allyTarget);
            board->placeFidele(allyTarget, spawnOpts[i]);
            std::cout << "-> " << allyTarget->getName() << " a été transporté en (" << spawnOpts[i].x << "," << spawnOpts[i].y << ").\n";
            return true;
        }
    }
    std::cout << "-> Échec du support aérien : aucune case adjacente libre.\n";
    return false;
}

void AbilityController::useAmphisbaenaAttack(Board* board, Fidele* amphisbaena, Fidele* target1, Fidele* target2) {
    if (!amphisbaena || !amphisbaena->isAlive()) return;
    printAbilityTrigger(amphisbaena);

    if (target1 && target1->isAlive() && board->isInRange(amphisbaena->getPosition(), target1->getPosition(), 1)) {
        std::cout << "-> Première tête attaque " << target1->getName() << " :\n";
        board->attackFidele(amphisbaena, target1);
    }
    if (target2 && target2->isAlive() && target2 != target1 && board->isInRange(amphisbaena->getPosition(), target2->getPosition(), 1)) {
        std::cout << "-> Seconde tête attaque " << target2->getName() << " :\n";
        board->attackFidele(amphisbaena, target2);
    }
}

bool AbilityController::useParcaeThreadOfLife(Board* board, Fidele* parcae, Fidele* deadAlly) {
    if (!parcae || !parcae->isAlive() || !deadAlly || deadAlly->isAlive()) return false;

    if (board->isSameTile(parcae->getPosition(), deadAlly->getDeathPosition())) {
        printAbilityTrigger(parcae);
        if (board->resurrectFidele(deadAlly)) {
            // Force HP to 1 as per Parcae rules
            deadAlly->takeDamage(deadAlly->getCurrentHP() - 1);
            std::cout << "-> " << deadAlly->getName() << " est ressuscité avec 1 PV !\n";
            return true;
        }
    }
    return false;
}

bool AbilityController::useCaladriusBloodDonation(Fidele* caladrius, Fidele* allyTarget) {
    if (!caladrius || !allyTarget || !caladrius->isAlive() || !allyTarget->isAlive()) return false;

    if (caladrius->getCurrentHP() > 1) {
        printAbilityTrigger(caladrius);
        int donation = std::min(2, caladrius->getCurrentHP() - 1); // Max 2, must leave Caladrius with at least 1 HP to not die
        caladrius->takeDamage(donation);

        // Heal allyTarget
        int current = allyTarget->getCurrentHP();
        int max = allyTarget->getHP();
        int newHp = std::min(max, current + donation);
        allyTarget->setHP(newHp); // We'll need to add setHP or similar, or just manipulate currentHp if we expose it

        std::cout << "-> Caladrius sacrifie " << donation << " PV pour soigner " << allyTarget->getName() << ".\n";
        return true;
    }
    return false;
}

void AbilityController::handlePostMoveInterrupts(Board* board, Fidele* movedUnit) {
    if (!movedUnit || !movedUnit->isAlive()) return;

    for (int i = 0; i < board->LENGTH; ++i) {
        for (int j = 0; j < board->WIDTH; ++j) {
            Fidele* occ = board->getFideleAt({i, j});
            if (occ && occ->isAlive()) {

                // Wake up allies sleeping next to moved unit
                if (occ->getIsAsleep() && occ->getOwner() == movedUnit->getOwner()) {
                    Position mPos = movedUnit->getPosition();
                    Position occPos = occ->getPosition();
                    if (std::abs(mPos.x - occPos.x) + std::abs(mPos.y - occPos.y) == 1) {
                        occ->setAsleep(false);
                        std::cout << "-> " << movedUnit->getName() << " réveille " << occ->getName() << " !\n";
                    }
                }

                if (occ->getOwner() != movedUnit->getOwner()) {

                // Theseus: Ariadne's Thread
                if (occ->getAbility() == "Ariadne's tread" || occ->getAbility() == "Fil d'Ariane") {
                    if (board->isSameTile(occ->getPosition(), movedUnit->getPosition())) {
                        printAbilityTrigger(occ);

                        // Use his actual stats to move. Thésée needs to leave the tile.
                        // We will simulate 1 step at a time dynamically using max possible speed.
                        Movement m = occ->getMovement();
                        int maxBwd = m.backward;

                        std::string escapeDir = (occ->getOwner() == Player::Player1) ? "Down" : "Up"; // Usually backward
                        std::cout << "[PROMPT] Thésée fuit la tuile. Tente un déplacement de " << maxBwd << " cases vers " << escapeDir << " (Simulé)\n";

                        bool escapeSuccess = board->moveUnit(occ, escapeDir, maxBwd);
                        if (!escapeSuccess && m.left > 0) {
                            std::cout << "[PROMPT] Fuite bloquée, tente de fuir sur le côté.\n";
                            board->moveUnit(occ, "Left", m.left);
                        }
                    }
                }

                // Cubs: Predators
                if (occ->getAbility() == "Predators" || occ->getAbility() == "Prédateurs") {
                    if (board->isInRange(occ->getPosition(), movedUnit->getPosition(), occ->getRange()) || board->isSameTile(occ->getPosition(), movedUnit->getPosition())) {
                        printAbilityTrigger(occ);
                        std::cout << "-> Les Louveteaux bondissent sur la proie qui approche !\n";
                        board->attackFidele(occ, movedUnit);
                    }
                }

                // Odysseus: Trojan Horse
                if (occ->getAbility() == "Trojan horse" || occ->getAbility() == "Cheval de Troie") {
                    if (board->isInRange(occ->getPosition(), movedUnit->getPosition(), occ->getRange())) {
                        printAbilityTrigger(occ);
                        std::cout << "[PROMPT] Ulysse peut attaquer hors-tour. Lancer l'attaque ? (O/N) : O (Simulé)\n";

                        // Attack first, then apply penalty to his *next* normal turn
                        board->attackFidele(occ, movedUnit);
                        occ->setSkipNextAttack(true);
                    }
                }
                } // end if owner !=
            }
        }
    }
}
