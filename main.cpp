#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <curl/curl.h>
#include <deque>
#include <algorithm>
#include <random>

#include "Fidele.h"
#include "Domaine.h"
#include "Board.h"

#include "God.h"
#include "GodPowerManager.h"

// Define the static Language variable from Fidele and God
Language Fidele::currentLanguage = Language::English;
Language God::currentLanguage = Language::English;

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

PowerType parsePowerType(const std::string& typeStr) {
    if (typeStr == "Green" || typeStr == "Vert") return PowerType::Green;
    if (typeStr == "Red" || typeStr == "Rouge") return PowerType::Red;
    return PowerType::Unknown;
}

void setupGame(std::vector<Fidele>& allFideles, std::vector<God>& allGods, PlayerState& p1, PlayerState& p2) {
    p1.playerId = Player::Player1;
    p2.playerId = Player::Player2;

    std::vector<Fidele*> greekUnits;
    std::vector<Fidele*> romanUnits;

    for (auto& f : allFideles) {
        if (f.getFaction() == Faction::Greek) greekUnits.push_back(&f);
        else if (f.getFaction() == Faction::Roman) romanUnits.push_back(&f);
    }

    // Shuffle the lists
    auto rng = std::default_random_engine{};
    std::shuffle(std::begin(greekUnits), std::end(greekUnits), rng);
    std::shuffle(std::begin(romanUnits), std::end(romanUnits), rng);

    // Pick up to 10 for each
    for (size_t i = 0; i < 10 && i < greekUnits.size(); ++i) {
        greekUnits[i]->setOwner(Player::Player1);
        p1.deck.push_back(greekUnits[i]);
    }
    for (size_t i = 0; i < 10 && i < romanUnits.size(); ++i) {
        romanUnits[i]->setOwner(Player::Player2);
        p2.deck.push_back(romanUnits[i]);
    }

    // Deal 5 cards initially
    for (int i = 0; i < 5; ++i) {
        if (!p1.deck.empty()) {
            p1.hand.push_back(p1.deck.front());
            p1.deck.pop_front();
        }
        if (!p2.deck.empty()) {
            p2.hand.push_back(p2.deck.front());
            p2.deck.pop_front();
        }
    }

    // Allocate 3 God cards to each player based on their faction
    std::vector<God*> greekGods;
    std::vector<God*> romanGods;
    for (auto& g : allGods) {
        if (g.getFaction() == Faction::Greek) greekGods.push_back(&g);
        else if (g.getFaction() == Faction::Roman) romanGods.push_back(&g);
    }

    std::shuffle(std::begin(greekGods), std::end(greekGods), rng);
    std::shuffle(std::begin(romanGods), std::end(romanGods), rng);

    for (int i = 0; i < 3; ++i) {
        if (i < greekGods.size()) p1.gods.push_back(greekGods[i]);
        if (i < romanGods.size()) p2.gods.push_back(romanGods[i]);
    }
}

struct GameState {
    PlayerState* p1;
    PlayerState* p2;
};

void checkAndProcessDeaths(GameState& state, const std::vector<Fidele>& allFideles) {
    for (auto& f : const_cast<std::vector<Fidele>&>(allFideles)) {
        if (!f.isAlive() && f.getOwner() != Player::None && f.getDeathPosition().x != -1) {
            PlayerState* ownerState = (f.getOwner() == Player::Player1) ? state.p1 : state.p2;

            // Is it already in the deck?
            bool inDeck = false;
            for (auto* deckUnit : ownerState->deck) {
                if (deckUnit == &f) { inDeck = true; break; }
            }
            // Is it in the hand?
            bool inHand = false;
            for (auto* handUnit : ownerState->hand) {
                if (handUnit == &f) { inHand = true; break; }
            }

            if (!inDeck && !inHand) {
                // It just died. Put it at the bottom of the deck.
                std::cout << f.getName() << "'s card is returned to the bottom of the deck.\n";
                ownerState->deck.push_back(&f);
            }
        }
    }
}

void printHand(const PlayerState& p) {
    std::cout << "Current Hand (" << (p.playerId == Player::Player1 ? "Player 1" : "Player 2") << "): [";
    for (size_t i = 0; i < p.hand.size(); ++i) {
        std::cout << p.hand[i]->getName();
        if (i < p.hand.size() - 1) std::cout << ", ";
    }
    std::cout << "]\n";
}

void executeInvocationPhase(PlayerState& p, Board& board) {
    std::cout << "\n===========================================\n";
    std::cout << "--- TURN INVOCATION PHASE: " << (p.playerId == Player::Player1 ? "PLAYER 1 (Greek)" : "PLAYER 2 (Roman)") << " ---\n";

    // 1. Draw 2 cards
    for (int i = 0; i < 2; ++i) {
        if (!p.deck.empty()) {
            p.hand.push_back(p.deck.front());
            p.deck.pop_front();
        }
    }
    std::cout << "Drew 2 cards from the deck.\n";

    // 2. Discard 1 card to the bottom of the deck
    if (!p.hand.empty()) {
        Fidele* discarded = p.hand.back();
        p.hand.pop_back();
        p.deck.push_back(discarded);
        std::cout << "Discarded " << discarded->getName() << " to the bottom of the deck.\n";
    }

    // 3. Play 1 unit on the board
    if (!p.hand.empty()) {
        Fidele* played = p.hand.front();
        p.hand.erase(p.hand.begin());

        bool placed = false;

        // Check for Resurrection
        Position deathPos = played->getDeathPosition();
        if (deathPos.x != -1 && deathPos.y != -1) {
            std::cout << "Attempting to resurrect " << played->getName() << " at its death position...\n";
            placed = board.resurrectFidele(played);
        }

        // Standard placement
        if (!placed) {
            Position startPos = board.getFirstEmptyStartRow(p.playerId);
            if (startPos.x != -1) {
                placed = board.placeNewFidele(played, startPos, p.playerId);
            }
        }

        if (placed) {
            std::cout << "Played " << played->getName() << " onto the board.\n";
        } else {
            std::cout << "Failed to play " << played->getName() << ". Putting back in hand.\n";
            p.hand.push_back(played);
        }
    }

    // Print Hand
    printHand(p);

    // End of Turn Maintenance: Refill hand to 2 if needed
    while (p.hand.size() < 2 && !p.deck.empty()) {
        p.hand.push_back(p.deck.front());
        p.deck.pop_front();
    }

    // Display the updated board state
    std::cout << "\n[Plateau après Invocation]\n";
    board.displayBoard();
}

void printStateOfTheWar(const GameState& state, Board& board, const std::vector<Fidele>& allFideles) {
    std::cout << "\n===========================================\n";
    std::cout << "          STATE OF THE WAR                 \n";
    std::cout << "===========================================\n";

    std::cout << " [Domaines]\n";
    std::cout << "  - Olympe (P1)   : " << board.getDomaine(Player::Player1)->getHP() << " PV\n";
    std::cout << "  - Panthéon (P2) : " << board.getDomaine(Player::Player2)->getHP() << " PV\n\n";

    std::cout << " [Cartes Dieux restantes]\n";
    std::cout << "  - P1 (Grec)   : " << state.p1->gods.size() << "\n";
    std::cout << "  - P2 (Romain) : " << state.p2->gods.size() << "\n\n";

    std::cout << " [Unités en vie sur le plateau]\n";
    bool anyAlive = false;
    for (auto& f : allFideles) {
        if (f.isAlive() && f.getPosition().x != -1) {
            std::cout << "  - " << f.getName()
                      << " (" << (f.getOwner() == Player::Player1 ? "P1" : "P2") << ") | "
                      << "PV: " << f.getCurrentHP() << "/" << f.getHP() << " | "
                      << "Portée: " << f.getRange() << "\n";
            anyAlive = true;
        }
    }
    if (!anyAlive) {
        std::cout << "  (Aucune unité en vie)\n";
    }
    std::cout << "===========================================\n\n";
}

int main() {
    std::cout << "===========================================\n";
    std::cout << "          W A R   D O L L S                \n";
    std::cout << "===========================================\n";
    std::cout << "Choisissez votre langue / Choose your language (FR/EN) : \n";

    // In a real environment, we would use std::cin. For automated testing, we hardcode to French as requested.
    std::string langChoice = "FR"; // Hardcoded for test. Simulate: std::cin >> langChoice;

    if (langChoice == "FR") {
        std::cout << "> Langue choisie : Français\n\n";
        Fidele::currentLanguage = Language::French;
        God::currentLanguage = Language::French;
    } else {
        std::cout << "Language chosen : English\n\n";
        Fidele::currentLanguage = Language::English;
        God::currentLanguage = Language::English;
    }

    Domaine myDomaine(Player::Player1);

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

    // --- Load Gods CSV ---
    std::vector<God> gods;
    std::string godsUrl = "https://docs.google.com/spreadsheets/d/e/2PACX-1vTcCb5RRtnClh6-InKLA8U9ppoosYrBBIWi-xjrs7vK8FG0mUm4XLIRAc0OJNlOo1fQ84lZA9BM3mU0/pub?gid=1190568154&single=true&output=csv";
    std::cout << "Fetching Gods data from Google Sheets..." << std::endl;
    std::string godsCsvData = downloadCSV(godsUrl);

    if (godsCsvData.empty() || godsCsvData.find("<html") != std::string::npos) {
        std::cerr << "Failed to fetch Gods CSV data or URL is broken. Falling back to hardcoded mock data for testing." << std::endl;
        gods.push_back(God("Zeus", "Zeus", "Lightning", "Éclairs", "Sends a shower of lightning and inflicts 5 points of damage.", "Envoie une pluie d'éclairs et inflige 5 points de dégâts.", Faction::Greek, PowerType::Red));
        gods.push_back(God("Hades", "Hadès", "Recall", "Rappel", "Resurrects a unit.", "Ressuscite une unité.", Faction::Greek, PowerType::Red));
    } else {
        std::stringstream gss(godsCsvData);
        std::string gline;
        std::getline(gss, gline); // Skip header

        while (std::getline(gss, gline)) {
            if (gline.empty() || gline == "\r") continue;

            std::vector<std::string> cols = parseCSVLine(gline);
            // Expected columns from the new Google Sheets link:
            // 0: Name
            // 1: Name FR
            // 2: Faction
            // 3: Power_Type
            // 4: Ability
            // 5: Ability FR
            // 6: Description
            // 7: Description FR

            if (cols.size() < 8) {
                std::cerr << "Skipping malformed God line: " << gline << std::endl;
                continue;
            }

            try {
                std::string nameEn = cols[0];
                std::string nameFr = cols[1];
                Faction faction = parseFaction(cols[2]);
                PowerType pType = parsePowerType(cols[3]);
                std::string abilityEn = cols[4];
                std::string abilityFr = cols[5];
                std::string descEn = cols[6];
                std::string descFr = cols[7];

                gods.push_back(God(nameEn, nameFr, abilityEn, abilityFr, descEn, descFr, faction, pType));
            } catch (const std::exception& e) {
                std::cerr << "Error parsing God line: " << gline << " - " << e.what() << std::endl;
            }
        }
    }
    std::cout << "Loaded " << gods.size() << " gods." << std::endl;

    // Toggling language to show both
    Fidele::currentLanguage = Language::English;
    God::currentLanguage = Language::English;
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

    // Find "Cyclope" and "Achille" in the loaded fideles
    Fidele* cyclopePtr = nullptr;
    Fidele* achillePtr = nullptr;
    for (auto& f : fideles) {
        if (f.getName() == "Cyclope") cyclopePtr = &f;
        if (f.getName() == "Achille") achillePtr = &f;
    }

    if (!cyclopePtr || !achillePtr) {
        std::cerr << "Could not find Cyclope or Achille in the CSV." << std::endl;
        return 1;
    }

    // Cyclops is Roman. Let's place him for Player 2 (Pantheon)
    Position p2StartPos = {10, 4};
    board.placeInitialFidele(cyclopePtr, p2StartPos, Player::Player2);

    // Achille is Greek. Let's place him for Player 1 (Olympe)
    Position p1StartPos = {2, 4};
    board.placeInitialFidele(achillePtr, p1StartPos, Player::Player1);

    // Display the initial board
    std::cout << "\n[Initial Board State]\n";
    board.displayBoard();

    // Test interactive movement commands
    std::cout << "\n--- Testing Interactive Movement ---\n";
    std::cout << "Command: Move Achille Down 2\n";
    if (board.moveUnit(achillePtr, "Down", 2)) {
        std::cout << "-> Achille moved successfully to (" << achillePtr->getPosition().x << "," << achillePtr->getPosition().y << ")!\n";
    }

    // --- Testing PLAY_GOD command ---
    std::cout << "\n--- Testing PLAY_GOD command ---\n";

    // We will simulate P1 (Greek) playing Zeus on Cyclope
    God* zeusPtr = nullptr;
    for (auto& g : gods) {
        // Find by name in either language
        if (g.getName() == "Zeus" || g.getName() == "Zeus (FR)") zeusPtr = &g;
    }

    // Since currentLanguage is French by the time we get here, the name might be 'Zeus' depending on the sheet. Let's just search Ability "Eclairs" to be safe.
    if (!zeusPtr) {
        for (auto& g : gods) {
            if (g.getAbility() == "Eclairs" || g.getAbility() == "Lightning") zeusPtr = &g;
        }
    }

    if (zeusPtr) {
        // We need a dummy player state for this quick test before the main loop resets things
        PlayerState dummyP1;
        dummyP1.playerId = Player::Player1;
        dummyP1.gods.push_back(zeusPtr);

        std::string command = "PLAY_GOD Zeus Cyclope";
        std::cout << "Command received: " << command << "\n";

        if (command.rfind("PLAY_GOD ", 0) == 0) {
            std::string args = command.substr(9);
            size_t spacePos = args.find(' ');
            if (spacePos != std::string::npos) {
                std::string godName = args.substr(0, spacePos);
                std::string targetName = args.substr(spacePos + 1);

                GodPowerManager::playGod(zeusPtr, dummyP1, board, Player::Player1, targetName, fideles);
            }
        }
    } else {
        std::cout << "Zeus God card not found in CSV.\n";
    }

    std::cout << "\n[Board State After God Card]\n";
    board.displayBoard();

    // --- FULL GAME LOOP ---
    std::cout << "\n--- Setting up standard game loop ---\n";
    board = Board(); // Reset board to clean state
    // Reset Fidele objects states
    for (auto& f : fideles) {
        f.setAlive(true);
        f.setPosition({-1, -1});
        f.setDeathPosition({-1, -1});
        f.setOwner(Player::None);
    }

    PlayerState player1, player2;
    setupGame(fideles, gods, player1, player2);
    GameState state = { &player1, &player2 };

    std::cout << "Game setup complete.\n";
    std::cout << "Player 1 Deck: " << player1.deck.size() << " cards, Hand: " << player1.hand.size() << " cards\n";
    std::cout << "Player 2 Deck: " << player2.deck.size() << " cards, Hand: " << player2.hand.size() << " cards\n";

    bool gameIsRunning = true;
    int turnCounter = 1;

    // Simulation loop. We will run max 4 turns to avoid infinite loops in test
    while (gameIsRunning && turnCounter <= 4) {
        std::cout << "\n===========================================\n";
        std::cout << "               TURN " << turnCounter << "\n";
        std::cout << "===========================================\n";

        // --- Player 1 Turn ---
        checkAndProcessDeaths(state, fideles); // Check if cards need returning to deck
        executeInvocationPhase(player1, board);

        // Simulated P1 Actions (Randomly attack something to trigger death logic)
        for (auto& p1Unit : fideles) {
            if (p1Unit.getOwner() == Player::Player1 && p1Unit.isAlive() && p1Unit.getPosition().x != -1) {
                // Just attack P2 domain if possible, else attack any enemy in range
                board.attackDomaine(&p1Unit, board.getDomaine(Player::Player2));

                for (auto& p2Unit : fideles) {
                    if (p2Unit.getOwner() == Player::Player2 && p2Unit.isAlive() && p2Unit.getPosition().x != -1) {
                        board.attackFidele(&p1Unit, &p2Unit);
                    }
                }
            }
        }

        if (board.getDomaine(Player::Player2)->getHP() <= 0) { gameIsRunning = false; break; }

        // --- Player 2 Turn ---
        checkAndProcessDeaths(state, fideles); // Check if cards need returning to deck
        executeInvocationPhase(player2, board);

        // Simulated P2 Actions
        for (auto& p2Unit : fideles) {
            if (p2Unit.getOwner() == Player::Player2 && p2Unit.isAlive() && p2Unit.getPosition().x != -1) {
                board.attackDomaine(&p2Unit, board.getDomaine(Player::Player1));

                for (auto& p1Unit : fideles) {
                    if (p1Unit.getOwner() == Player::Player1 && p1Unit.isAlive() && p1Unit.getPosition().x != -1) {
                        board.attackFidele(&p2Unit, &p1Unit);
                    }
                }
            }
        }

        if (board.getDomaine(Player::Player1)->getHP() <= 0) { gameIsRunning = false; break; }

        printStateOfTheWar(state, board, fideles);

        turnCounter++;
    }

    if (turnCounter > 4) {
        std::cout << "\n[Test simulation finished successfully after 4 turns]\n";
    }

    return 0;
}
