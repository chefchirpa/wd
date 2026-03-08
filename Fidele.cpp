#include "Fidele.h"
#include <iostream>

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
