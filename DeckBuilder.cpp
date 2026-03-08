#include "DeckBuilder.h"
#include <ncurses.h>
#include <iostream>
#include <fstream>
#include <algorithm>

// Define box dimensions
const int CARD_WIDTH = 25;
const int CARD_HEIGHT = 6;
const int MARGIN_X = 2;
const int MARGIN_Y = 1;
const int CARDS_PER_ROW = 4;

void drawAsciiCard(int startY, int startX, const std::string& name, const std::string& nature, const std::string& stats, bool isSelected, bool isHighlighted) {
    if (isHighlighted) {
        attron(COLOR_PAIR(2)); // Highlight color
    } else if (isSelected) {
        attron(COLOR_PAIR(3)); // Selected color
    } else {
        attron(COLOR_PAIR(1)); // Normal border
    }

    // Top border
    mvprintw(startY, startX, "+-----------------------+");

    // Empty side borders
    for (int i = 1; i < CARD_HEIGHT - 1; ++i) {
        mvprintw(startY + i, startX, "|                       |");
    }

    // Bottom border
    mvprintw(startY + CARD_HEIGHT - 1, startX, "+-----------------------+");

    // Print contents
    if (isSelected) {
        mvprintw(startY + 1, startX + 2, "[X] %s", name.substr(0, CARD_WIDTH - 6).c_str());
    } else {
        mvprintw(startY + 1, startX + 2, "[ ] %s", name.substr(0, CARD_WIDTH - 6).c_str());
    }

    mvprintw(startY + 3, startX + 2, "Type: %s", nature.substr(0, CARD_WIDTH - 8).c_str());
    mvprintw(startY + 4, startX + 2, "%s", stats.substr(0, CARD_WIDTH - 4).c_str());

    if (isHighlighted) attroff(COLOR_PAIR(2));
    else if (isSelected) attroff(COLOR_PAIR(3));
    else attroff(COLOR_PAIR(1));
}

std::string fideleTypeToStringDB(FideleType t) {
    if (t == FideleType::Heros) return "Heros";
    if (t == FideleType::Creature) return "Creature";
    return "Divinite";
}

void DeckBuilder::launchInteractiveBuilder(const std::vector<Fidele>& allFideles, const std::vector<God>& allGods, Faction targetFaction) {
    std::vector<Fidele> factionFideles;
    std::vector<God> factionGods;

    for (const auto& f : allFideles) {
        if (f.getFaction() == targetFaction) factionFideles.push_back(f);
    }
    for (const auto& g : allGods) {
        if (g.getFaction() == targetFaction) factionGods.push_back(g);
    }

    initscr();
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);

    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLACK);
        init_pair(2, COLOR_BLACK, COLOR_CYAN);  // Highlighted
        init_pair(3, COLOR_GREEN, COLOR_BLACK); // Selected
        init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Headers
    }

    int currentPhase = 0; // 0 = Gods, 1 = Fideles
    int cursorIndex = 0;

    std::vector<int> selectedGods;
    std::vector<int> selectedFideles;

    bool running = true;

    while (running) {
        clear();

        attron(COLOR_PAIR(4) | A_BOLD);
        mvprintw(1, 2, "=== WAR DOLLS DECK BUILDER ===");
        mvprintw(2, 2, "Faction: %s", targetFaction == Faction::Greek ? "Greek" : "Roman");
        attroff(COLOR_PAIR(4) | A_BOLD);

        mvprintw(4, 2, "Selected Gods: %lu/3   |   Selected Fideles: %lu/10", selectedGods.size(), selectedFideles.size());
        mvprintw(5, 2, "Press [SPACE] to select/deselect. Press [ENTER] when ready to advance.");

        int totalItems = (currentPhase == 0) ? factionGods.size() : factionFideles.size();

        if (currentPhase == 0) {
            mvprintw(7, 2, "--- SELECT GOD CARDS ---");
            for (size_t i = 0; i < factionGods.size(); ++i) {
                int row = i / CARDS_PER_ROW;
                int col = i % CARDS_PER_ROW;
                int startY = 9 + (row * (CARD_HEIGHT + MARGIN_Y));
                int startX = 2 + (col * (CARD_WIDTH + MARGIN_X));

                bool highlighted = (i == cursorIndex);
                bool selected = (std::find(selectedGods.begin(), selectedGods.end(), i) != selectedGods.end());

                std::string typeStr = (factionGods[i].getPowerType() == PowerType::Red) ? "Red" : "Green";
                std::string stats = "Power: " + typeStr;

                drawAsciiCard(startY, startX, factionGods[i].getName(), "God", stats, selected, highlighted);
            }
        } else {
            mvprintw(7, 2, "--- SELECT FIDELE CARDS ---");
            for (size_t i = 0; i < factionFideles.size(); ++i) {
                int row = i / CARDS_PER_ROW;
                int col = i % CARDS_PER_ROW;
                int startY = 9 + (row * (CARD_HEIGHT + MARGIN_Y));
                int startX = 2 + (col * (CARD_WIDTH + MARGIN_X));

                bool highlighted = (i == cursorIndex);
                bool selected = (std::find(selectedFideles.begin(), selectedFideles.end(), i) != selectedFideles.end());

                std::string typeStr = fideleTypeToStringDB(factionFideles[i].getType());
                std::string stats = "HP:" + std::to_string(factionFideles[i].getHP()) + " Rng:" + std::to_string(factionFideles[i].getRange());

                drawAsciiCard(startY, startX, factionFideles[i].getName(), typeStr, stats, selected, highlighted);
            }
        }

        refresh();

        int ch = getch();
        switch (ch) {
            case KEY_LEFT:
                if (cursorIndex > 0) cursorIndex--;
                break;
            case KEY_RIGHT:
                if (cursorIndex < totalItems - 1) cursorIndex++;
                break;
            case KEY_UP:
                if (cursorIndex >= CARDS_PER_ROW) cursorIndex -= CARDS_PER_ROW;
                break;
            case KEY_DOWN:
                if (cursorIndex + CARDS_PER_ROW < totalItems) cursorIndex += CARDS_PER_ROW;
                break;
            case ' ':
                if (currentPhase == 0) {
                    auto it = std::find(selectedGods.begin(), selectedGods.end(), cursorIndex);
                    if (it != selectedGods.end()) {
                        selectedGods.erase(it);
                    } else if (selectedGods.size() < 3) {
                        selectedGods.push_back(cursorIndex);
                    }
                } else {
                    auto it = std::find(selectedFideles.begin(), selectedFideles.end(), cursorIndex);
                    if (it != selectedFideles.end()) {
                        selectedFideles.erase(it);
                    } else if (selectedFideles.size() < 10) {
                        selectedFideles.push_back(cursorIndex);
                    }
                }
                break;
            case '\n': // Enter key
                if (currentPhase == 0) {
                    if (selectedGods.size() == 3) {
                        currentPhase = 1;
                        cursorIndex = 0;
                    }
                } else {
                    if (selectedFideles.size() == 10 && selectedGods.size() == 3) {
                        running = false;
                    }
                }
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
        }
    }

    clear();

    if (selectedFideles.size() == 10 && selectedGods.size() == 3) {
        echo();
        curs_set(1);
        char deckName[50];
        mvprintw(2, 2, "Enter a name for this deck: ");
        getnstr(deckName, 49);

        char makeDefaultStr[10];
        mvprintw(4, 2, "Set as Default for this Faction? (y/n): ");
        getnstr(makeDefaultStr, 9);
        bool isDefault = (makeDefaultStr[0] == 'y' || makeDefaultStr[0] == 'Y');

        endwin(); // Exit ncurses mode

        // Save to JSON (Simple raw write for demo)
        std::ofstream out("decks.json", std::ios::app);
        if (out.is_open()) {
            out << "{\n  \"name\": \"" << deckName << "\",\n  \"faction\": \"" << (targetFaction == Faction::Greek ? "Greek" : "Roman") << "\",\n";
            out << "  \"default\": " << (isDefault ? "true" : "false") << ",\n";
            out << "  \"gods\": [\n";
            for (size_t i = 0; i < selectedGods.size(); ++i) {
                out << "    \"" << factionGods[selectedGods[i]].getName() << "\"" << (i < 2 ? "," : "") << "\n";
            }
            out << "  ],\n  \"fideles\": [\n";
            for (size_t i = 0; i < selectedFideles.size(); ++i) {
                out << "    \"" << factionFideles[selectedFideles[i]].getName() << "\"" << (i < 9 ? "," : "") << "\n";
            }
            out << "  ]\n}\n";
            std::cout << "Successfully saved deck '" << deckName << "' to decks.json!\n";
        }
    } else {
        endwin();
        std::cout << "Deck building aborted.\n";
    }
}
