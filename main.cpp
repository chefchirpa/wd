#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "Fidele.h"
#include "Domaine.h"

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

    return 0;
}
