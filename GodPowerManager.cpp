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

    std::string gName = god->getName();

    // GREEK GODS
    if (gName == "Zeus" || baseAbility.find("clair") != std::string::npos || baseAbility.find("Lightning") != std::string::npos) {
        // Target format for Zeus in simulation: "TargetName" (simulated dealing all 4 to one target for simplicity)
        Fidele* target = nullptr;
        for (auto& f : allFideles) {
            if (f.getName() == targetName && f.isAlive() && f.getPosition().x != -1) { target = &f; break; }
        }

        if (target && target->isImmuneToAbilities()) {
            std::cout << target->getName() << " (Géant) est immunisé contre les pouvoirs des Dieux !\n";
            success = false;
        } else if (target) {
            std::cout << "Zeus foudroie " << target->getName() << " et inflige 4 dégâts purs !\n";
            target->takeDamage(4);
            if (target->getCurrentHP() <= 0) {
                std::cout << target->getName() << " a été pulvérisé par Zeus !\n";
                board.killFidele(target);
                Domaine* enemyDom = board.getDomaine(target->getOwner());
                if (enemyDom) {
                    enemyDom->takeDamage(1);
                    std::cout << "Le domaine ennemi perd 1 PV. HP restants: " << enemyDom->getHP() << "\n";
                }
            }
            success = true;
        }
    }
    else if (gName == "Hermès" || gName == "Hermes") {
        std::cout << "-> Hermès accorde un déplacement gratuit à TOUS les alliés !\n";
        for (auto& f : allFideles) {
            if (f.isAlive() && f.getOwner() == owner.playerId && f.getPosition().x != -1) {
                std::cout << "[PROMPT] Déplacement gratuit pour " << f.getName() << " (Simulé : Left 1)\n";
                board.moveUnit(&f, "Left", 1);
            }
        }
        success = true;
    }
    else if (gName == "Hadès" || gName == "Hades") {
        std::cout << "-> Hadès autorise une seconde invocation (simulée ici depuis la main)\n";
        if (!owner.hand.empty()) {
            Fidele* extra = owner.hand.front();
            owner.hand.erase(owner.hand.begin());

            Position startPos = board.getFirstEmptyStartRow(owner.playerId);
            if (startPos.x != -1) {
                board.placeNewFidele(extra, startPos, owner.playerId);
                std::cout << "-> " << extra->getName() << " invoqué via Hadès en (" << startPos.x << "," << startPos.y << ")\n";
                success = true;
            }

            // Draw 1 card to maintain hand size
            if (!owner.deck.empty()) {
                owner.hand.push_back(owner.deck.front());
                owner.deck.pop_front();
                std::cout << "-> Pioche 1 carte pour compenser l'invocation d'Hadès.\n";
            }
        }
    }
    else if (gName == "Dyonysos" || gName == "Dionysus") {
        std::cout << "-> Dionysos rend ivres les ennemis ! (Simulation: On prend le contrôle d'une cible ennemie pour ce tour)\n";
        Fidele* target = nullptr;
        for (auto& f : allFideles) {
            if (f.getName() == targetName && f.isAlive() && f.getOwner() != owner.playerId && f.getPosition().x != -1) {
                target = &f; break;
            }
        }
        if (target && !target->isImmuneToAbilities()) {
            board.turnMods.dionysusMindControlled.push_back({target, target->getOwner()});
            target->setOwner(owner.playerId); // Temporarily steal control
            std::cout << "-> Vous contrôlez " << target->getName() << " jusqu'à la fin du tour !\n";
            success = true;
        } else if (target && target->isImmuneToAbilities()) {
            std::cout << "-> " << target->getName() << " (Géant) est immunisé à l'ivresse !\n";
        }
    }
    else if (gName == "Arès" || gName == "Ares") {
        std::cout << "-> L'aura d'Arès double vos dégâts pour ce tour !\n";
        board.turnMods.aresDoubleDamage = true;
        board.turnMods.activeGodPlayer = owner.playerId;
        success = true;
    }

    // ROMAN GODS
    else if (gName == "Diane" || gName == "Diana") {
        std::cout << "-> Les flèches d'argent de Diane repoussent tous les ennemis de 4 cases !\n";
        std::string pushDir = (owner.playerId == Player::Player1) ? "Down" : "Up"; // Push them away
        for (auto& f : allFideles) {
            if (f.isAlive() && f.getOwner() != owner.playerId && f.getPosition().x != -1) {
                if (f.isImmuneToAbilities()) {
                    std::cout << "-> " << f.getName() << " (Géant) encaisse la flèche sans bouger.\n";
                    continue;
                }
                std::cout << "-> " << f.getName() << " est repoussé de 4 cases !\n";
                // Simulate pushing
                bool pushed = board.moveUnit(&f, pushDir, 4);
                if (!pushed) {
                    std::cout << "   Chemin bloqué, esquive latérale...\n";
                    board.moveUnit(&f, "Left", 1);
                }
            }
        }
        success = true;
    }
    else if (gName == "Vulcain" || gName == "Vulcan") {
        std::cout << "-> Vulcain fournit des munitions lourdes (2 dés, garde le meilleur) pour ce tour !\n";
        board.turnMods.vulcanDoubleDice = true;
        board.turnMods.activeGodPlayer = owner.playerId;
        success = true;
    }
    else if (gName == "Pluton" || gName == "Pluto") {
        std::cout << "-> Pluton envoie une âme dans le Tartare pour l'éternité !\n";
        Fidele* target = nullptr;
        for (auto& f : allFideles) {
            if (f.getName() == targetName && f.getOwner() != owner.playerId && (!f.isAlive() || f.getCurrentHP() < f.getHP())) {
                target = &f; break;
            }
        }
        if (target) {
            std::cout << "-> " << target->getName() << " est banni définitivement du jeu !\n";
            board.removeFideleFromGrid(target);
            target->setAlive(false);
            target->setHP(0);
            target->setDeathPosition({-1,-1}); // Erase death pos so cannot be resurrected
            success = true;
        } else {
            std::cout << "Cible introuvable ou n'est pas blessée/morte.\n";
        }
    }
    else if (gName == "Minerve" || gName == "Minerva") {
        std::cout << "-> L'Égide de Minerve protège 3 entités pour le tour !\n";
        // Simulate protecting the Domain and 2 specific allies
        board.turnMods.minervaImmuneDomaines.push_back(board.getDomaine(owner.playerId));
        int count = 0;
        for (auto& f : allFideles) {
            if (f.isAlive() && f.getOwner() == owner.playerId && f.getPosition().x != -1) {
                board.turnMods.minervaImmuneFideles.push_back(&f);
                std::cout << "   " << f.getName() << " est protégé par l'Égide.\n";
                count++;
                if (count >= 2) break; // 1 Domain + 2 Units = 3 targets
            }
        }
        success = true;
    }
    else if (gName == "Neptune") {
        std::cout << "-> Le Typhon de Neptune désactive tous les pouvoirs ennemis pour ce tour !\n";
        board.turnMods.neptuneNullifyEnemyAbilities = true;
        board.turnMods.activeGodPlayer = owner.playerId;
        success = true;
    }

    if (success) {
        god->setPlayed(true);
    }

    return success;
}
