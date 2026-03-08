#include "Fidele.h"
#include "Board.h"
#include <iostream>

void Fidele::applyDefensiveAbility(Board* board, int& additionalDefenseBonus) const {
    if (!board) return;

    if (abilityEn == "Phalanx" || abilityEn == "Support" || abilityFr == "Phalange" || abilityFr == "Soutien") {
        Position myPos = this->getPosition();

        // Check Up, Down, Left, Right
        Position adj[4] = {
            {myPos.x + 1, myPos.y},
            {myPos.x - 1, myPos.y},
            {myPos.x, myPos.y + 1},
            {myPos.x, myPos.y - 1}
        };

        int allyCount = 0;
        for (int i = 0; i < 4; ++i) {
            Fidele* occ = board->getFideleAt(adj[i]);
            if (occ != nullptr && occ->isAlive() && occ->getOwner() == this->getOwner() && occ != this) {
                allyCount++;
            }
        }

        if (allyCount > 0) {
            additionalDefenseBonus += allyCount;

            // Output string using the French Name constraint from user
            std::cout << "Pouvoir " << abilityFr << " activé : +" << allyCount << " bonus de défense grâce aux alliés adjacents.\n";
        }
    }
}

void Fidele::applyOffensiveAbility(Fidele* target, int& additionalAttackBonus) const {
    if (!target) return;

    // Use abilityEn or abilityFr to map logic (Google sheet uses "Eye" or "Œil")
    if (abilityEn == "Eye" || abilityFr == "Œil" || abilityFr.find("Rayon") != std::string::npos) {
        Position myPos = this->getPosition();
        Position targetPos = target->getPosition();

        // Target must be on the same column (y matches)
        if (myPos.y == targetPos.y) {
            // Target must be directly "in front".
            // Player 1 (Olympe) starts at top (x=0) and faces down (+x).
            // Player 2 (Pantheon) starts at bottom (x=11) and faces up (-x).
            bool inFront = false;
            if (this->getOwner() == Player::Player1 && targetPos.x > myPos.x) {
                inFront = true;
            } else if (this->getOwner() == Player::Player2 && targetPos.x < myPos.x) {
                inFront = true;
            }

            if (inFront) {
                additionalAttackBonus += 1;

                // Display feedback in French as requested
                Language prevLang = currentLanguage;
                currentLanguage = Language::French;
                std::cout << "\n>>> POUVOIR ACTIVÉ : " << this->getName() << " - " << this->getAbility() << " <<<\n";
                std::cout << "Effet : " << this->getDescription() << "\n\n";
                currentLanguage = prevLang;
            }
        }
    }
}
