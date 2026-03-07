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
    Domaine myDomaine;
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

    // Test a valid move: 2 squares forward, 1 right (Cyclope has 3 Forward, 2 Right)
    // Note: Player 1 moves "forward" in the +x direction
    std::vector<Position> validPath = {
        {3, 4}, // Forward 1
        {4, 4}, // Forward 2
        {4, 5}  // Right 1
    };

    if (board.moveFidele(&myCyclope, validPath)) {
        std::cout << "Valid move successful! New position: ("
                  << myCyclope.getPosition().x << "," << myCyclope.getPosition().y << ")\n";
    } else {
        std::cout << "Valid move failed.\n";
    }

    // Test an invalid move: moving diagonally
    std::vector<Position> invalidPath = {
        {5, 6} // Diagonal move from {4,5}
    };

    if (board.moveFidele(&myCyclope, invalidPath)) {
        std::cout << "Invalid move succeeded (Error!).\n";
    } else {
        std::cout << "Invalid move rejected successfully (No diagonals).\n";
    }

    // Test killing and resurrecting
    std::cout << "\nKilling " << myCyclope.getName() << "...\n";
    board.killFidele(&myCyclope);

    if (!myCyclope.isAlive()) {
        std::cout << myCyclope.getName() << " is dead. Death position recorded at ("
                  << myCyclope.getDeathPosition().x << "," << myCyclope.getDeathPosition().y << ")\n";
    }

    std::cout << "Resurrecting " << myCyclope.getName() << "...\n";
    board.resurrectFidele(&myCyclope);

    if (myCyclope.isAlive() && myCyclope.getPosition() == myCyclope.getDeathPosition()) {
        std::cout << myCyclope.getName() << " resurrected successfully at ("
                  << myCyclope.getPosition().x << "," << myCyclope.getPosition().y << ")\n";
    } else {
        std::cout << "Resurrection failed.\n";
    }

    std::cout << "\nFinal Board State:\n";
    board.printBoard();

    return 0;
}
