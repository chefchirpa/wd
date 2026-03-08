#include "GodPowerManager.h"
#include <iostream>

bool GodPowerManager::playGod(God* god, PlayerState& owner, Board& board, Player activePlayer, const std::string& targetName, std::vector<Fidele>& allFideles) {
    if (!god) return false;

    if (god->hasBeenPlayed()) {
        std::cout << "God power " << god->getName() << " has already been played this game!\n";
        return false;
    }

    // Power_Type check: Green can only be played during owner's turn
    if (god->getPowerType() == PowerType::Green && owner.playerId != activePlayer) {
        std::cout << "Cannot play " << god->getName() << " right now: Green God cards can only be played during your own turn.\n";
        return false;
    }

    std::cout << "\n>>> PLAYING GOD CARD: " << god->getName() << " <<<\n";

    // Switch to French just to print the French description as requested
    Language prevLang = God::currentLanguage;
    God::currentLanguage = Language::French;
    std::cout << "Effet: " << god->getDescription() << "\n\n";
    God::currentLanguage = prevLang; // Restore

    bool success = false;
    std::string baseAbility = god->getAbility(); // E.g., "Lightning" or "Eclairs"

    // Example - Zeus (Éclairs)
    if (god->getName() == "Zeus" || baseAbility.find("clair") != std::string::npos || baseAbility.find("Lightning") != std::string::npos) {
        // Target an enemy unit and subtract 5 HP
        Fidele* target = nullptr;
        for (auto& f : allFideles) {
            if (f.getName() == targetName && f.isAlive() && f.getPosition().x != -1) {
                target = &f;
                break;
            }
        }

        if (target) {
            std::cout << "Zeus strikes " << target->getName() << " for 5 damage!\n";
            target->takeDamage(5);
            if (target->getCurrentHP() <= 0) {
                std::cout << target->getName() << " was obliterated by Zeus!\n";
                board.killFidele(target);

                Domaine* enemyDom = board.getDomaine(target->getOwner());
                if (enemyDom) {
                    enemyDom->takeDamage(1);
                    std::cout << "Enemy Domain loses 1 HP. Current HP: " << enemyDom->getHP() << "\n";
                }
            }
            success = true;
        } else {
            std::cout << "Zeus's strike failed: Target '" << targetName << "' not found or not alive on the board.\n";
        }
    }
    // Example - Hades (Rappel / Underworld)
    else if (god->getName() == "Hades" || god->getName() == "Hadès" || baseAbility.find("Rappel") != std::string::npos || baseAbility.find("Recall") != std::string::npos) {
        // Take a dead unit from the player's deck and resurrect it immediately
        Fidele* target = nullptr;

        // Search owner's deck for a dead unit matching the name
        auto it = owner.deck.begin();
        while (it != owner.deck.end()) {
            if ((*it)->getName() == targetName && !(*it)->isAlive()) {
                target = *it;
                owner.deck.erase(it); // Remove from deck
                break;
            }
            ++it;
        }

        if (target) {
            std::cout << "Hades summons " << target->getName() << " from the underworld!\n";
            bool resurrected = board.resurrectFidele(target);
            if (resurrected) {
                std::cout << target->getName() << " successfully resurrected at its death position.\n";
                success = true;
            } else {
                std::cout << "Failed to resurrect " << target->getName() << ". Position might be blocked.\n";
                // Put back in deck if failed
                owner.deck.push_back(target);
            }
        } else {
            std::cout << "Hades's power failed: Dead target '" << targetName << "' not found in deck.\n";
        }
    } else {
        std::cout << "This God's specific power logic is not yet hardcoded. Marking as played.\n";
        success = true;
    }

    if (success) {
        god->setPlayed(true);
    }

    return success;
}
