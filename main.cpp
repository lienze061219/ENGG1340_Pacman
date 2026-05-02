#include "Map.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iostream>
#include <iomanip>
#include <limits>
#include <queue>
#include <random>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <termios.h>
#include <unistd.h>
#endif

#ifndef _WIN32
class TerminalRawMode {
public:
    TerminalRawMode() : enabled(false) {
        if (tcgetattr(STDIN_FILENO, &original) == 0) {
            termios raw = original;
            raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));
            raw.c_cc[VMIN] = 0;
            raw.c_cc[VTIME] = 0;
            if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) enabled = true;
        }
    }
    ~TerminalRawMode() {
        if (enabled) tcsetattr(STDIN_FILENO, TCSANOW, &original);
    }

private:
    termios original{};
    bool enabled;
};
#endif

enum class Difficulty { Easy = 1, Medium = 2, Hard = 3 };
enum class GhostMode { Random, Chase };

struct GhostState {
    int x;
    int y;
    int spawnX;
    int spawnY;
    GhostMode mode;
    std::chrono::milliseconds moveInterval;
    std::chrono::steady_clock::time_point lastMoveTime;
};

struct MenuConfig {
    int mapChoice = 1;
    Difficulty difficulty = Difficulty::Easy;
    bool startGame = true;
};

enum class SessionEndAction { Restart, BackToMenu, Quit };

namespace {
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string DIM = "\033[2m";
const std::string RED = "\033[31m";
const std::string GREEN = "\033[32m";
const std::string YELLOW = "\033[33m";
const std::string BLUE = "\033[34m";
const std::string MAGENTA = "\033[35m";
const std::string CYAN = "\033[36m";
const std::string WHITE = "\033[37m";
const std::string BRIGHT_RED = "\033[91m";
const std::string BRIGHT_GREEN = "\033[92m";
const std::string BRIGHT_YELLOW = "\033[93m";
const std::string BRIGHT_BLUE = "\033[94m";
const std::string BRIGHT_MAGENTA = "\033[95m";
const std::string BRIGHT_CYAN = "\033[96m";
const std::string BRIGHT_WHITE = "\033[97m";

char pollInput() {
#ifdef _WIN32
    if (_kbhit()) return static_cast<char>(std::tolower(_getch()));
    return '\0';
#else
    char ch = '\0';
    ssize_t bytesRead = read(STDIN_FILENO, &ch, 1);
    if (bytesRead > 0) return static_cast<char>(std::tolower(ch));
    return '\0';
#endif
}

void clearAndHomeCursor() {
    std::cout << "\033[2J\033[H";
}

void hideCursor() {
    std::cout << "\033[?25l";
}

void showCursor() {
    std::cout << "\033[?25h";
}

int randBetween(int lo, int hi, std::mt19937& rng) {
    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(rng);
}

std::pair<int, int> findPlayerSpawn(Map& map) {
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            if (map.getTile(x, y) == 'P') {
                map.setTile(x, y, ' ');
                return {x, y};
            }
        }
    }
    return {1, 1};
}

std::vector<std::pair<int, int>> collectWalkableTiles(const Map& map) {
    std::vector<std::pair<int, int>> cells;
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            if (map.isSafe(x, y)) cells.push_back({x, y});
        }
    }
    return cells;
}

std::pair<int, int> bfsNextStep(const Map& map, int sx, int sy, int tx, int ty) {
    if (sx == tx && sy == ty) return {sx, sy};
    const int w = map.getWidth();
    const int h = map.getHeight();
    if (sx < 0 || sx >= w || sy < 0 || sy >= h) return {sx, sy};
    if (tx < 0 || tx >= w || ty < 0 || ty >= h) return {sx, sy};

    std::vector<std::vector<bool>> vis(h, std::vector<bool>(w, false));
    std::vector<std::vector<std::pair<int, int>>> parent(h, std::vector<std::pair<int, int>>(w, {-1, -1}));

    std::queue<std::pair<int, int>> q;
    q.push({sx, sy});
    vis[sy][sx] = true;
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};

    while (!q.empty()) {
        auto cur = q.front();
        q.pop();
        if (cur.first == tx && cur.second == ty) break;

        for (int i = 0; i < 4; ++i) {
            int nx = cur.first + dx[i];
            int ny = cur.second + dy[i];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h && map.isSafe(nx, ny) && !vis[ny][nx]) {
                vis[ny][nx] = true;
                parent[ny][nx] = cur;
                q.push({nx, ny});
            }
        }
    }

    if (!vis[ty][tx]) return {sx, sy};
    int cx = tx;
    int cy = ty;
    while (parent[cy][cx].first != -1) {
        auto p = parent[cy][cx];
        if (p.first == sx && p.second == sy) return {cx, cy};
        cx = p.first;
        cy = p.second;
    }
    return {sx, sy};
}

void moveGhostRandom(GhostState& ghost, const Map& map, std::mt19937& rng) {
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};
    std::vector<int> order = {0, 1, 2, 3};
    std::shuffle(order.begin(), order.end(), rng);

    for (int idx : order) {
        int nx = ghost.x + dx[idx];
        int ny = ghost.y + dy[idx];
        if (map.isSafe(nx, ny)) {
            ghost.x = nx;
            ghost.y = ny;
            break;
        }
    }
    // 随机鬼移动速度波动
    ghost.moveInterval = std::chrono::milliseconds(randBetween(140, 360, rng));
}

void moveGhostChase(GhostState& ghost, const Map& map, int playerX, int playerY) {
    auto nxt = bfsNextStep(map, ghost.x, ghost.y, playerX, playerY);
    ghost.x = nxt.first;
    ghost.y = nxt.second;
}

void drawRainbowFrame(int width, int height) {
    const std::string palette[6] = {BRIGHT_RED, BRIGHT_YELLOW, BRIGHT_GREEN, BRIGHT_CYAN, BRIGHT_BLUE, BRIGHT_MAGENTA};
    clearAndHomeCursor();
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            bool border = (y == 0 || y == height - 1 || x == 0 || x == width - 1);
            if (border) {
                std::cout << palette[(x + y) % 6] << "#" << RESET;
            } else {
                std::cout << " ";
            }
        }
        std::cout << "\n";
    }
}

void playChestAnimation(int mapWidth, int mapHeight) {
    int fx = mapWidth + 10;
    int fy = mapHeight + 10;
    for (int i = 0; i < 2; ++i) {
        clearAndHomeCursor();
        std::cout << "\033[1;107m";
        for (int y = 0; y < fy; ++y) {
            for (int x = 0; x < fx; ++x) std::cout << " ";
            std::cout << "\n";
        }
        std::cout << RESET;
        std::this_thread::sleep_for(std::chrono::milliseconds(70));
        drawRainbowFrame(fx, fy);
        std::cout << "\033[" << (fy / 2) << ";5H" << BOLD << BRIGHT_YELLOW << "TREASURE! HP +1" << RESET;
        std::cout.flush();
        std::this_thread::sleep_for(std::chrono::milliseconds(90));
    }
}

std::string hpBar(int hp, int maxHp) {
    std::string s;
    for (int i = 0; i < maxHp; ++i) s += (i < hp ? "H" : "-");
    return s;
}

std::string difficultyText(Difficulty difficulty) {
    if (difficulty == Difficulty::Easy) return "Easy";
    if (difficulty == Difficulty::Medium) return "Medium";
    return "Hard";
}

std::string mapText(int mapChoice) {
    if (mapChoice == 1) return "Small";
    if (mapChoice == 2) return "Medium";
    return "Large";
}

void drawTimeBar(const std::string& label, int remainMs, int totalMs, const std::string& color) {
    const int width = 24;
    int clamped = std::max(0, std::min(remainMs, totalMs));
    int fill = (totalMs > 0) ? (clamped * width / totalMs) : 0;
    std::cout << BOLD << BRIGHT_WHITE << std::setw(12) << std::left << label << RESET << " ";
    std::cout << color << "[";
    for (int i = 0; i < width; ++i) std::cout << (i < fill ? "=" : " ");
    std::cout << "]" << RESET << " ";
    std::cout << BRIGHT_WHITE << std::setw(4) << (clamped / 1000.0) << "s" << RESET << "\n";
}

void waitForEnter() {
    std::cout << DIM << "Press Enter to go back..." << RESET;
    std::cout.flush();
    while (true) {
        char c = pollInput();
        if (c == '\n' || c == '\r') return;
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }
}

char pollInputWithTimeout(int timeoutMs) {
    const int step = 5;
    int waited = 0;
    while (waited < timeoutMs) {
        char c = pollInput();
        if (c != '\0') return c;
        std::this_thread::sleep_for(std::chrono::milliseconds(step));
        waited += step;
    }
    return '\0';
}

char readMenuKeyBlocking() {
    while (true) {
        char c = pollInput();
        if (c == '\0') {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        if (c == '\n' || c == '\r') return 'e'; // enter
        if (c == 'w' || c == 's' || c == 'q') return c;

        // Handle ANSI arrow keys: ESC [ A/B
        if (static_cast<unsigned char>(c) == 27) {
            char c1 = pollInputWithTimeout(25);
            char c2 = pollInputWithTimeout(25);
            if (c1 == '[' && (c2 == 'a' || c2 == 'A')) return 'u';
            if (c1 == '[' && (c2 == 'b' || c2 == 'B')) return 'd';
        }
    }
}

void showDifficultyHelpScreen() {
    clearAndHomeCursor();
    std::cout << BOLD << BRIGHT_CYAN << "================ Difficulty Help ================\n" << RESET;
    std::cout << BRIGHT_GREEN << "[Easy]   " << RESET << "All ghosts move randomly.\n";
    std::cout << BRIGHT_YELLOW << "[Medium] " << RESET << "Half random, half chase player.\n";
    std::cout << BRIGHT_RED << "[Hard]   " << RESET << "All ghosts chase player with BFS.\n\n";
    std::cout << BRIGHT_WHITE << "Tips:\n";
    std::cout << "- Super bean makes you faster and lets you eat ghosts.\n";
    std::cout << "- Chest restores 1 HP and triggers a visual effect.\n";
    std::cout << "- After being hit, you gain a short invincibility window.\n\n" << RESET;
    waitForEnter();
}

void showLegendScreen() {
    clearAndHomeCursor();
    std::cout << BOLD << BRIGHT_CYAN << "=================== Legend ===================\n" << RESET;
    std::cout << BOLD << BRIGHT_YELLOW << "P" << RESET << " : Player\n";
    std::cout << BOLD << BRIGHT_RED << "G" << RESET << " : Ghost (dangerous)\n";
    std::cout << BOLD << BRIGHT_CYAN << "g" << RESET << " : Frightened ghost (edible during SuperMode)\n";
    std::cout << WHITE << "." << RESET << " : Bean (+10)\n";
    std::cout << BRIGHT_MAGENTA << "o" << RESET << " : Big bean (+25)\n";
    std::cout << BOLD << BRIGHT_YELLOW << "C" << RESET << " : Treasure chest (+1 HP, +80)\n";
    std::cout << BOLD << BRIGHT_GREEN << "S" << RESET << " : Super bean (+120, speed boost)\n";
    std::cout << BRIGHT_BLUE << "#" << RESET << " : Wall\n\n";
    std::cout << BRIGHT_WHITE << "Controls: W/A/S/D move, Q quit\n\n" << RESET;
    waitForEnter();
}

MenuConfig runStartMenu() {
    MenuConfig cfg;
#ifndef _WIN32
    TerminalRawMode rawMode;
#endif
    hideCursor();
    int selected = 0;
    const int itemCount = 6;
    while (true) {
        clearAndHomeCursor();
        std::cout << BOLD << BRIGHT_CYAN << "+======================================================+\n";
        std::cout << "|               PACMAN CONSOLE EDITION                 |\n";
        std::cout << "+======================================================+\n" << RESET;
        std::cout << BRIGHT_WHITE << "Current Map: " << BRIGHT_YELLOW << mapText(cfg.mapChoice)
                  << BRIGHT_WHITE << "   Current Difficulty: " << BRIGHT_MAGENTA << difficultyText(cfg.difficulty) << RESET << "\n\n";
        std::string items[itemCount] = {
            "Start Game",
            "Difficulty Explanation",
            "Symbol Legend",
            "Change Map Size",
            "Change Difficulty",
            "Quit"};
        for (int i = 0; i < itemCount; ++i) {
            bool highlight = (i == selected);
            std::string prefix = highlight ? (BOLD + BRIGHT_YELLOW + "> ") : (DIM + "  ");
            std::string textColor = highlight ? BRIGHT_WHITE : BRIGHT_GREEN;
            std::cout << prefix << textColor << items[i];
            if (i == 3) std::cout << BRIGHT_CYAN << "  [" << mapText(cfg.mapChoice) << "]";
            if (i == 4) std::cout << BRIGHT_CYAN << "  [" << difficultyText(cfg.difficulty) << "]";
            std::cout << RESET << "\n";
        }
        std::cout << "\n" << DIM << "Use Arrow Up/Down (or W/S), Enter select, Q quit" << RESET;
        std::cout.flush();

        char key = readMenuKeyBlocking();
        if (key == 'u' || key == 'w') {
            selected = (selected - 1 + itemCount) % itemCount;
            continue;
        }
        if (key == 'd' || key == 's') {
            selected = (selected + 1) % itemCount;
            continue;
        }
        if (key == 'q') {
            cfg.startGame = false;
            showCursor();
            return cfg;
        }
        if (key != 'e') continue;

        if (selected == 0) {
            showCursor();
            return cfg;
        } else if (selected == 1) {
            showCursor();
            showDifficultyHelpScreen();
            hideCursor();
        } else if (selected == 2) {
            showCursor();
            showLegendScreen();
            hideCursor();
        } else if (selected == 3) {
            cfg.mapChoice = (cfg.mapChoice % 3) + 1;
        } else if (selected == 4) {
            int d = static_cast<int>(cfg.difficulty);
            d = (d % 3) + 1;
            cfg.difficulty = static_cast<Difficulty>(d);
        } else {
            cfg.startGame = false;
            showCursor();
            return cfg;
        }
    }
}

std::vector<int> loadScores(const std::string& path) {
    std::vector<int> scores;
    std::ifstream in(path);
    int value = 0;
    while (in >> value) scores.push_back(value);
    return scores;
}

void saveScores(const std::string& path, const std::vector<int>& scores) {
    std::ofstream out(path, std::ios::trunc);
    for (int s : scores) out << s << "\n";
}

void showEndAnimation(bool won) {
    std::string text = won ? "YOU WIN!" : "GAME OVER";
    std::string color = won ? BRIGHT_GREEN : BRIGHT_RED;
    for (int i = 0; i < 4; ++i) {
        clearAndHomeCursor();
        std::cout << "\n\n";
        std::cout << BOLD << color << "                " << text << RESET << "\n";
        std::cout << (i % 2 == 0 ? BRIGHT_YELLOW : BRIGHT_MAGENTA) << "        ******************************" << RESET << "\n";
        std::this_thread::sleep_for(std::chrono::milliseconds(150));
    }
}

SessionEndAction showGameOverScreen(bool won, int score) {
    showEndAnimation(won);
    std::vector<int> scores = loadScores("scores.txt");
    scores.push_back(score);
    std::sort(scores.begin(), scores.end(), std::greater<int>());
    if (scores.size() > 10) scores.resize(10);
    saveScores("scores.txt", scores);

    clearAndHomeCursor();
    std::cout << BOLD << BRIGHT_CYAN << "+======================================================+\n" << RESET;
    std::cout << BOLD << (won ? BRIGHT_GREEN : BRIGHT_RED) << (won ? "|                     YOU WIN!                         |\n"
                                                                    : "|                    GAME OVER                         |\n")
              << RESET;
    std::cout << BOLD << BRIGHT_CYAN << "+======================================================+\n" << RESET;
    std::cout << BRIGHT_WHITE << "Final Score: " << BOLD << BRIGHT_YELLOW << score << RESET << "\n\n";
    std::cout << BOLD << BRIGHT_MAGENTA << "Top Scores\n" << RESET;
    for (size_t i = 0; i < scores.size() && i < 5; ++i) {
        std::cout << BRIGHT_WHITE << std::setw(2) << (i + 1) << ". "
                  << (i == 0 ? BRIGHT_YELLOW : BRIGHT_CYAN) << scores[i] << RESET << "\n";
    }
    std::cout << "\n" << BOLD << BRIGHT_GREEN << "[R]" << RESET << " Restart";
    std::cout << "   " << BOLD << BRIGHT_CYAN << "[M]" << RESET << " Main Menu";
    std::cout << "   " << BOLD << BRIGHT_RED << "[Q]" << RESET << " Quit\n";
    std::cout << BRIGHT_WHITE << "Press R/M/Q: " << RESET;
    std::cout.flush();

#ifndef _WIN32
    TerminalRawMode rawMode;
#endif
    while (true) {
        char c = pollInput();
        if (c == '\0') {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
            continue;
        }
        c = static_cast<char>(std::tolower(c));
        if (c == 'r') return SessionEndAction::Restart;
        if (c == 'm') return SessionEndAction::BackToMenu;
        if (c == 'q') return SessionEndAction::Quit;
    }
}

void drawGame(const Map& map, int playerX, int playerY, const std::vector<GhostState>& ghosts, int hp, int maxHp,
              int chestLeft, int superBeanLeft, Difficulty difficulty, bool speedBoostOn, bool playerInvincible, int score,
              int superRemainMs, int superTotalMs, int invRemainMs, int invTotalMs) {
    clearAndHomeCursor();
    std::string diffText = difficultyText(difficulty);

    std::cout << BOLD << BRIGHT_CYAN << "+==============================================================+\n" << RESET;
    std::cout << BOLD << BRIGHT_CYAN << "| " << BRIGHT_YELLOW << "PACMAN REALTIME" << BRIGHT_CYAN
              << " | " << BRIGHT_GREEN << "Score: " << score
              << BRIGHT_CYAN << " | " << BRIGHT_RED << "HP: " << hpBar(hp, maxHp)
              << BRIGHT_CYAN << " | " << BRIGHT_MAGENTA << "Diff: " << diffText << BRIGHT_CYAN << " |\n" << RESET;
    std::cout << BOLD << BRIGHT_CYAN << "+==============================================================+\n" << RESET;

    for (int y = 0; y < map.getHeight(); ++y) {
        std::cout << BRIGHT_BLUE << "|" << RESET;
        for (int x = 0; x < map.getWidth(); ++x) {
            bool painted = false;
            if (x == playerX && y == playerY) {
                if (playerInvincible && (std::rand() % 2 == 0)) {
                    std::cout << DIM << BRIGHT_YELLOW << "P" << RESET;
                } else {
                    std::cout << BOLD << BRIGHT_YELLOW << "P" << RESET;
                }
                continue;
            }
            for (const auto& g : ghosts) {
                if (g.x == x && g.y == y) {
                    std::cout << (speedBoostOn ? BOLD + BRIGHT_CYAN + "g" + RESET : BOLD + BRIGHT_RED + "G" + RESET);
                    painted = true;
                    break;
                }
            }
            if (painted) continue;
            char tile = map.getTile(x, y);
            if (tile == '#') std::cout << BRIGHT_BLUE << "#" << RESET;
            else if (tile == '.') std::cout << WHITE << "." << RESET;
            else if (tile == 'o') std::cout << BRIGHT_MAGENTA << "o" << RESET;
            else if (tile == 'C') std::cout << BOLD << BRIGHT_YELLOW << "C" << RESET;
            else if (tile == 'S') std::cout << BOLD << BRIGHT_GREEN << "S" << RESET;
            else std::cout << " ";
        }
        std::cout << BRIGHT_BLUE << "|" << RESET << '\n';
    }
    std::cout << BOLD << BRIGHT_CYAN << "+==============================================================+\n" << RESET;

    std::cout << BOLD << BRIGHT_WHITE << "Beans: " << map.countBeans()
              << " | Chests: " << chestLeft
              << " | SuperBeans: " << superBeanLeft
              << " | SuperMode: " << (speedBoostOn ? BRIGHT_GREEN + std::string("ON") + RESET : BRIGHT_RED + std::string("OFF") + RESET)
              << " | Invincible: " << (playerInvincible ? BRIGHT_GREEN + std::string("ON") + RESET : BRIGHT_RED + std::string("OFF") + RESET)
              << "\n" << RESET;
    drawTimeBar("SuperMode", superRemainMs, superTotalMs, BRIGHT_GREEN);
    drawTimeBar("Invincible", invRemainMs, invTotalMs, BRIGHT_YELLOW);
    std::cout << DIM << "WASD move | Q quit | During SuperMode, collide to eat ghosts" << RESET << "\n";
}
} // namespace

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    bool needMenu = true;
    int mapChoice = 1;
    Difficulty difficulty = Difficulty::Easy;

    while (true) {
        std::mt19937 rng(static_cast<unsigned int>(std::time(nullptr)));

        if (needMenu) {
            MenuConfig menuConfig = runStartMenu();
            if (!menuConfig.startGame) {
                clearAndHomeCursor();
                std::cout << BRIGHT_CYAN << "Bye!\n" << RESET;
                break;
            }
            mapChoice = menuConfig.mapChoice;
            difficulty = menuConfig.difficulty;
        }

        std::string mapFile = "map_small.txt";
        int ghostCount = 2;
        int chestCount = 2;
        int superBeanCount = 1;
        if (mapChoice == 2) {
            mapFile = "map_medium.txt";
            ghostCount = 3;
            chestCount = 3;
            superBeanCount = 2;
        } else if (mapChoice == 3) {
            mapFile = "map_large.txt";
            ghostCount = 4;
            chestCount = 5;
            superBeanCount = 3;
        }

        Map gameMap;
        if (!gameMap.loadFromFile(mapFile)) {
            std::cerr << "Failed to load " << mapFile << '\n';
            return 1;
        }

        auto spawn = findPlayerSpawn(gameMap);
        int playerX = spawn.first;
        int playerY = spawn.second;

        auto walkable = collectWalkableTiles(gameMap);
        std::shuffle(walkable.begin(), walkable.end(), rng);

        int cursor = 0;
        for (int i = 0; i < chestCount && cursor < static_cast<int>(walkable.size()); ++i) {
            auto pos = walkable[cursor++];
            if (pos.first == playerX && pos.second == playerY) {
                --i;
                continue;
            }
            gameMap.setTile(pos.first, pos.second, 'C');
        }
        for (int i = 0; i < superBeanCount && cursor < static_cast<int>(walkable.size()); ++i) {
            auto pos = walkable[cursor++];
            if (gameMap.getTile(pos.first, pos.second) == 'C' || (pos.first == playerX && pos.second == playerY)) {
                --i;
                continue;
            }
            gameMap.setTile(pos.first, pos.second, 'S');
        }

        std::vector<GhostState> ghosts;
        ghosts.reserve(static_cast<size_t>(ghostCount));
        auto now = std::chrono::steady_clock::now();
        for (int i = 0; i < ghostCount && cursor < static_cast<int>(walkable.size()); ++i) {
            auto pos = walkable[cursor++];
            if (!gameMap.isSafe(pos.first, pos.second) || (pos.first == playerX && pos.second == playerY)) {
                --i;
                continue;
            }

            GhostMode mode = GhostMode::Random;
            if (difficulty == Difficulty::Hard) {
                mode = GhostMode::Chase;
            } else if (difficulty == Difficulty::Medium) {
                mode = (i % 2 == 0) ? GhostMode::Chase : GhostMode::Random;
            }

            int intervalMs = (mode == GhostMode::Chase) ? 180 : randBetween(140, 360, rng);
            ghosts.push_back({pos.first, pos.second, pos.first, pos.second, mode, std::chrono::milliseconds(intervalMs), now});
        }

        int hp = 3;
        int score = 0;
        const int maxHp = 3;
        bool running = true;
        char direction = '\0';

        const auto frameTime = std::chrono::milliseconds(33);
        auto playerMoveInterval = std::chrono::milliseconds(130);
        auto normalMoveInterval = std::chrono::milliseconds(130);
        auto boostedMoveInterval = std::chrono::milliseconds(80);
        auto lastPlayerMove = now;
        auto boostEndTime = now;
        const auto superDuration = std::chrono::seconds(6);
        auto invincibleEndTime = now;
        const auto hitInvincibleDuration = std::chrono::milliseconds(1200);

#ifndef _WIN32
        {
            TerminalRawMode rawMode;
#endif
            hideCursor();

            while (running) {
                auto frameStart = std::chrono::steady_clock::now();
                bool speedBoostOn = frameStart < boostEndTime;
                bool playerInvincible = frameStart < invincibleEndTime;
                playerMoveInterval = speedBoostOn ? boostedMoveInterval : normalMoveInterval;

                char input = pollInput();
                while (input != '\0') {
                    if (input == 'q') {
                        running = false;
                        break;
                    }
                    if (input == 'w' || input == 'a' || input == 's' || input == 'd') direction = input;
                    input = pollInput();
                }
                if (!running) break;

                if (direction != '\0' && frameStart - lastPlayerMove >= playerMoveInterval) {
                    int nx = playerX;
                    int ny = playerY;
                    if (direction == 'w') --ny;
                    if (direction == 's') ++ny;
                    if (direction == 'a') --nx;
                    if (direction == 'd') ++nx;

                    if (gameMap.isSafe(nx, ny)) {
                        playerX = nx;
                        playerY = ny;
                        char tile = gameMap.getTile(playerX, playerY);
                        if (tile == '.' || tile == 'o') {
                            gameMap.eatBean(playerX, playerY);
                            score += (tile == 'o') ? 25 : 10;
                        } else if (tile == 'C') {
                            hp = std::min(maxHp, hp + 1);
                            score += 80;
                            gameMap.setTile(playerX, playerY, ' ');
                            playChestAnimation(gameMap.getWidth(), gameMap.getHeight());
                        } else if (tile == 'S') {
                            gameMap.setTile(playerX, playerY, ' ');
                            boostEndTime = frameStart + superDuration;
                            score += 120;
                        }
                    }
                    lastPlayerMove = frameStart;
                }

                for (auto& ghost : ghosts) {
                    if (frameStart - ghost.lastMoveTime >= ghost.moveInterval) {
                        if (ghost.mode == GhostMode::Random) moveGhostRandom(ghost, gameMap, rng);
                        else moveGhostChase(ghost, gameMap, playerX, playerY);
                        ghost.lastMoveTime = frameStart;
                    }
                }

                for (auto& ghost : ghosts) {
                    if (ghost.x == playerX && ghost.y == playerY) {
                        if (speedBoostOn) {
                            score += 200;
                            ghost.x = ghost.spawnX;
                            ghost.y = ghost.spawnY;
                            ghost.lastMoveTime = frameStart + std::chrono::milliseconds(300);
                        } else if (!playerInvincible) {
                            --hp;
                            invincibleEndTime = frameStart + hitInvincibleDuration;
                            playerX = spawn.first;
                            playerY = spawn.second;
                            std::this_thread::sleep_for(std::chrono::milliseconds(180));
                        }
                        break;
                    }
                }

                int chestLeft = 0;
                int superBeanLeft = 0;
                for (int y = 0; y < gameMap.getHeight(); ++y) {
                    for (int x = 0; x < gameMap.getWidth(); ++x) {
                        if (gameMap.getTile(x, y) == 'C') ++chestLeft;
                        if (gameMap.getTile(x, y) == 'S') ++superBeanLeft;
                    }
                }

                int superRemainMs = std::max(0, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(boostEndTime - frameStart).count()));
                int invRemainMs = std::max(0, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(invincibleEndTime - frameStart).count()));
                int superTotalMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(superDuration).count());
                int invTotalMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(hitInvincibleDuration).count());

                drawGame(gameMap, playerX, playerY, ghosts, hp, maxHp, chestLeft, superBeanLeft, difficulty,
                         speedBoostOn, playerInvincible, score, superRemainMs, superTotalMs, invRemainMs, invTotalMs);

                if (hp <= 0) {
                    running = false;
                    break;
                }
                if (!gameMap.hasBeans()) {
                    running = false;
                    break;
                }

                auto frameEnd = std::chrono::steady_clock::now();
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
                if (elapsed < frameTime) std::this_thread::sleep_for(frameTime - elapsed);
            }
            showCursor();
#ifndef _WIN32
        }
#endif

        bool won = (hp > 0 && !gameMap.hasBeans());
        std::cout << "\033[0m\n";
        SessionEndAction next = showGameOverScreen(won, score);
        if (next == SessionEndAction::Restart) {
            needMenu = false;
            continue;
        }
        if (next == SessionEndAction::BackToMenu) {
            needMenu = true;
            continue;
        }
        break;
    }
    return 0;
}