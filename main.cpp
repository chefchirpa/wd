#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <curl/curl.h>

#include "Fidele.h"
#include "Domaine.h"
#include "Board.h"

// Define the static Language variable from Fidele
Language Fidele::currentLanguage = Language::English;

// Callback function for libcurl to write received data into a std::string
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    size_t totalSize = size * nmemb;
    userp->append((char*)contents, totalSize);
    return totalSize;
}

std::string downloadCSV(const std::string& url) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Google Sheets redirects, so we must follow them
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Execute the request
        res = curl_easy_perform(curl);

        if(res != CURLE_OK) {
            std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();
    return readBuffer;
}

FideleType parseType(const std::string& typeStr) {
    if (typeStr == "Hero" || typeStr == "Héros") return FideleType::Heros;
    if (typeStr == "Creature" || typeStr == "Créature") return FideleType::Creature;
    if (typeStr == "Divinity" || typeStr == "Divinité") return FideleType::Divinite;
    return FideleType::Heros; // Default
}

Faction parseFaction(const std::string& factionStr) {
    if (factionStr == "Greek") return Faction::Greek;
    if (factionStr == "Roman") return Faction::Roman;
    return Faction::None;
}

std::string typeToString(FideleType type) {
    switch (type) {
        case FideleType::Heros: return "Hero";
        case FideleType::Creature: return "Creature";
        case FideleType::Divinite: return "Divinity";
    }
    return "Unknown";
}

// Simple CSV parser capable of handling quoted strings containing commas
std::vector<std::string> parseCSVLine(const std::string& line) {
    std::vector<std::string> result;
    std::string current;
    bool inQuotes = false;
    for (size_t i = 0; i < line.length(); ++i) {
        char c = line[i];
        if (c == '\"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            result.push_back(current);
            current.clear();
        } else {
            current += c;
        }
    }
    result.push_back(current);

    // Clean up carriage returns (\r) often left at the end of CSV lines
    if (!result.empty() && !result.back().empty() && result.back().back() == '\r') {
        result.back().pop_back();
    }

    return result;
}

int main() {
    Domaine myDomaine(Player::Player1);
    std::cout << "Domaine initialized with HP: " << myDomaine.getHP() << std::endl;

    std::vector<Fidele> fideles;

    std::string url = "https://docs.google.com/spreadsheets/d/e/2PACX-1vTcCb5RRtnClh6-InKLA8U9ppoosYrBBIWi-xjrs7vK8FG0mUm4XLIRAc0OJNlOo1fQ84lZA9BM3mU0/pub?gid=0&single=true&output=csv";
    std::cout << "Fetching data from Google Sheets..." << std::endl;
    std::string csvData = downloadCSV(url);

    if (csvData.empty()) {
        std::cerr << "Failed to fetch CSV data." << std::endl;
        return 1;
    }

    std::stringstream ss(csvData);
    std::string line;

    // Skip the header
    std::getline(ss, line);

    while (std::getline(ss, line)) {
        if (line.empty() || line == "\r") continue;

        std::vector<std::string> cols = parseCSVLine(line);
        // Expected columns from Google Sheets:
        // 0: ID
        // 1: Faction
        // 2: Nature
        // 3: Name
        // 4: Ability
        // 5: Description
        // 6: Name FR
        // 7: Ability FR
        // 8: Description FR
        // 9: Image
        // 10: Has Token
        // 11: Range
        // 12: Health
        // 13: Up
        // 14: Down
        // 15: Left
        // 16: Right
        // 17: Has Die

        // Let's dynamically find the indices or just hardcode based on the actual sheet format we see failing
        // Given the error output, a line looks like:
        // 16,Greek,Hero,Theseus,Ariadne's tread,"...",Thésée,Fil d'Ariane,"...",Theseus.png,x,3,3,3,2,2,2,X,Effect,,X,
        // So:
        // 0: ID (16)
        // 1: Faction (Greek)
        // 2: Nature (Hero)
        // 3: Name En (Theseus)
        // 4: Ability En (Ariadne's tread)
        // 5: Desc En ("...")
        // 6: Name Fr (Thésée)
        // 7: Ability Fr (Fil d'Ariane)
        // 8: Desc Fr ("...")
        // 9: Image (Theseus.png)
        // 10: Has Token (x)
        // 11: Range (3)
        // 12: Health (3)
        // 13: Up (3)
        // 14: Down (2)
        // 15: Left (2)
        // 16: Right (2)
        // ...

        if (cols.size() < 17) {
            std::cerr << "Skipping malformed line (insufficient columns): " << line << std::endl;
            continue;
        }

        try {
            Faction faction = parseFaction(cols[1]);
            FideleType type = parseType(cols[2]);
            std::string nameEn = cols[3];
            std::string abilityEn = cols[4];
            std::string descEn = cols[5];
            std::string nameFr = cols[6];
            std::string abilityFr = cols[7];
            std::string descFr = cols[8];

            // Skip headers or empty fields safely
            if (cols[11].empty() || cols[12].empty() || cols[11] == "Range") continue;

            int range = std::stoi(cols[11]);
            int hp = std::stoi(cols[12]);

            Movement movement = {
                std::stoi(cols[13]), // Up/Forward
                std::stoi(cols[14]), // Down/Backward
                std::stoi(cols[15]), // Left
                std::stoi(cols[16])  // Right
            };

            fideles.push_back(Fidele(nameEn, nameFr, abilityEn, abilityFr, descEn, descFr,
                                     faction, type, hp, range, movement));
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line: " << line << " - " << e.what() << std::endl;
        }
    }

    std::cout << "Loaded " << fideles.size() << " fideles." << std::endl;

    // Toggling language to show both
    Fidele::currentLanguage = Language::English;
    std::cout << "\n[English Language Set]\n";
    for (const auto& fidele : fideles) {
        std::cout << "Fidele: " << fidele.getName() << " | Ability: " << fidele.getAbility() << "\n";
        std::cout << "  Desc: " << fidele.getDescription() << "\n";
    }

    Fidele::currentLanguage = Language::French;
    std::cout << "\n[French Language Set]\n";
    for (const auto& fidele : fideles) {
        std::cout << "Fidèle: " << fidele.getName() << " | Pouvoir: " << fidele.getAbility() << "\n";
        std::cout << "  Desc: " << fidele.getDescription() << "\n";
    }

    // --- Visual Simulation Demo ---
    std::cout << "\n--- Visual Simulation Test ---\n";

    // Use French Language
    Fidele::currentLanguage = Language::French;

    Board board;

    if (fideles.empty()) {
        std::cerr << "No fideles loaded to test the board." << std::endl;
        return 1;
    }

    // Find "Cyclope" in the loaded fideles
    Fidele* cyclopePtr = nullptr;
    for (auto& f : fideles) {
        if (f.getName() == "Cyclope") {
            cyclopePtr = &f;
            break;
        }
    }

    if (!cyclopePtr) {
        std::cerr << "Could not find Cyclope in the CSV." << std::endl;
        return 1;
    }

    // Cyclops is Roman. Let's place him for Player 2 (Pantheon)
    Position p2StartPos = {10, 4}; // Row 10 is within the first 3 rows from P2's side (9, 10, 11)

    if (board.placeInitialFidele(cyclopePtr, p2StartPos, Player::Player2)) {
        std::cout << "Successfully placed " << cyclopePtr->getName() << " at (" << p2StartPos.x << "," << p2StartPos.y << ")" << std::endl;
    } else {
        std::cout << "Failed to place " << cyclopePtr->getName() << std::endl;
    }

    // Let's add an enemy Greek unit just for fun to see both tags
    Fidele* heroPtr = nullptr;
    for (auto& f : fideles) {
        if (f.getFaction() == Faction::Greek && f.getType() == FideleType::Heros) {
            heroPtr = &f;
            break;
        }
    }

    if (heroPtr) {
        Position p1StartPos = {2, 4}; // Row 2 is within the first 3 rows from P1's side (0, 1, 2)
        board.placeInitialFidele(heroPtr, p1StartPos, Player::Player1);
    }

    // Display the simulated visual board
    std::cout << "\n";
    board.displayBoard();

    return 0;
}
