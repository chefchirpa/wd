#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "Fidele.h"
#include "Domaine.h"
#include "Board.h"

FideleType parseType(const std::string& typeStr) {
    if (typeStr == "Héros") return FideleType::Heros;
    if (typeStr == "Créature") return FideleType::Creature;
    if (typeStr == "Divinité") return FideleType::Divinite;
    return FideleType::Heros; // Default, maybe should throw an exception instead
}

std::string typeToString(FideleType type) {
    switch (type) {
        case FideleType::Heros: return "Héros";
        case FideleType::Creature: return "Créature";
        case FideleType::Divinite: return "Divinité";
    }
    return "Inconnu";
}

int main() {
    Domaine myDomaine(Player::Player1);
    std::cout << "Domaine initialized with HP: " << myDomaine.getHP() << std::endl;

    std::vector<Fidele> fideles;

    std::ifstream file("cards.csv");
    if (!file.is_open()) {
        std::cerr << "Failed to open cards.csv" << std::endl;
        return 1;
    }

    std::string line;
    // Skip the header
    std::getline(file, line);

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        std::string name, typeStr, hpStr, rangeStr, forwardStr, backwardStr, leftStr, rightStr;

        std::getline(ss, name, ',');
        std::getline(ss, typeStr, ',');
        std::getline(ss, hpStr, ',');
        std::getline(ss, rangeStr, ',');
        std::getline(ss, forwardStr, ',');
        std::getline(ss, backwardStr, ',');
        std::getline(ss, leftStr, ',');
        std::getline(ss, rightStr, ',');

        try {
            int hp = std::stoi(hpStr);
            int range = std::stoi(rangeStr);
            Movement movement = {
                std::stoi(forwardStr),
                std::stoi(backwardStr),
                std::stoi(leftStr),
                std::stoi(rightStr)
            };
            FideleType type = parseType(typeStr);

            fideles.push_back(Fidele(name, type, hp, range, movement));
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
        }
    }

    std::cout << "Loaded " << fideles.size() << " fideles." << std::endl;

    for (const auto& fidele : fideles) {
        std::cout << "Fidele: " << fidele.getName() << std::endl;
        std::cout << "  Type: " << typeToString(fidele.getType()) << std::endl;
        std::cout << "  HP: " << fidele.getHP() << std::endl;
        std::cout << "  Range: " << fidele.getRange() << std::endl;
        Movement mov = fidele.getMovement();
        std::cout << "  Movement (Forward: " << mov.forward
                  << ", Backward: " << mov.backward
                  << ", Left: " << mov.left
                  << ", Right: " << mov.right << ")" << std::endl;
    }

    std::cout << "\n--- Testing Board Logic ---\n";
    Board board;

    if (fideles.empty()) {
        std::cerr << "No fideles loaded to test the board." << std::endl;
        return 1;
    }

    // Give the first Fidele to Player 1 and place it on the board
    Fidele& myCyclope = fideles[0];
    myCyclope.setOwner(Player::Player1);

    Position startPos = {2, 4};
    board.placeFidele(&myCyclope, startPos);
    std::cout << "Placed " << myCyclope.getName() << " at (" << startPos.x << "," << startPos.y << ")\n";

    // Create an opponent Fidele
    Fidele enemyFidele = fideles[0]; // clone Cyclope
    enemyFidele.setOwner(Player::Player2);

    Position enemyStartPos = {2, 5}; // Within range 2 of (2,4)
    board.placeFidele(&enemyFidele, enemyStartPos);
    std::cout << "Placed Enemy " << enemyFidele.getName() << " at (" << enemyStartPos.x << "," << enemyStartPos.y << ")\n";

    std::cout << "\n--- Testing Combat: Fidele vs Fidele ---\n";

    // Player 1's Cyclope attacks Player 2's Cyclope
    board.attackFidele(&myCyclope, &enemyFidele);

    // Give enemy 1 HP so next attack likely kills
    if (enemyFidele.isAlive()) {
        enemyFidele.takeDamage(enemyFidele.getCurrentHP() - 1);
        std::cout << "\n[Setting Enemy HP to 1 to force a kill on next hit]\n";
        board.attackFidele(&myCyclope, &enemyFidele);
    }

    std::cout << "\n--- Testing Combat: Fidele vs Domaine ---\n";

    // Player 2's Domain is at the right edge (x >= 12).
    // MyCyclope has Range 2. If we put him at x=10, distance is 12 - 10 = 2.
    Position attackDomainPos = {10, 4};
    // Force place him there (for testing)
    board.killFidele(&myCyclope); // Remove from old spot to avoid grid conflict
    myCyclope.setAlive(true);
    board.placeFidele(&myCyclope, attackDomainPos);

    board.attackDomaine(&myCyclope, board.getDomaine(Player::Player2));

    std::cout << "\nFinal Board State:\n";
    board.printBoard();

    return 0;
}
