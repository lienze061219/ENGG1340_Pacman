#include "Map.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <functional>
#include <iostream>
#include <iomanip>
#include <limits>
#include <queue>
#include <random>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#ifdef _WIN32
#include <conio.h>
#else
#include <sys/ioctl.h>
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

enum class GhostBulletOutcome { None, Stunned, Eliminated };

struct GhostState {
    int x;
    int y;
    int spawnX;
    int spawnY;
    GhostMode mode;
    std::chrono::milliseconds moveInterval;
    std::chrono::steady_clock::time_point lastMoveTime;
    std::chrono::steady_clock::time_point stunnedUntil{};
    std::chrono::steady_clock::time_point eliminatedUntil{};
};

struct Bullet {
    int x;
    int y;
    char dir;
    std::chrono::steady_clock::time_point lastMove;
};

struct PendingBean {
    int x;
    int y;
    char type;
    std::chrono::steady_clock::time_point respawnAt;
};

struct MenuConfig {
    int mapChoice = 1;
    Difficulty difficulty = Difficulty::Easy;
    bool endlessMode = false;
    bool startGame = true;
};

enum class SessionEndAction { Restart, BackToMenu, Quit };

struct LeaderEntry {
    int score = 0;
    int timeSec = 0;
    std::string name;
};

namespace {
const char* kLeaderboardPath = "leaderboard.txt";
const char* kLegacyScoresPath = "scores.txt";

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

// 只读查找玩家出生格（不修改地图）
std::pair<int, int> findPlayerSpawnReadOnly(const Map& map) {
    for (int y = 0; y < map.getHeight(); ++y) {
        for (int x = 0; x < map.getWidth(); ++x) {
            if (map.getTile(x, y) == 'P') return {x, y};
        }
    }
    return {1, 1};
}

// 从 (sx,sy) 沿可行走格子 BFS，判断是否可达地图上所有豆子（. 与 o）
bool allBeansReachableFrom(const Map& map, int sx, int sy) {
    const int w = map.getWidth();
    const int h = map.getHeight();
    if (!map.isSafe(sx, sy)) return false;

    int beanTotal = 0;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            char t = map.getTile(x, y);
            if (t == '.' || t == 'o') ++beanTotal;
        }
    }
    if (beanTotal == 0) return true;

    std::vector<std::vector<bool>> vis(h, std::vector<bool>(w, false));
    std::queue<std::pair<int, int>> q;
    q.push({sx, sy});
    vis[sy][sx] = true;
    int reachableBeans = 0;
    const int dx[4] = {0, 0, -1, 1};
    const int dy[4] = {-1, 1, 0, 0};

    while (!q.empty()) {
        auto cur = q.front();
        q.pop();
        char t = map.getTile(cur.first, cur.second);
        if (t == '.' || t == 'o') ++reachableBeans;

        for (int i = 0; i < 4; ++i) {
            int nx = cur.first + dx[i];
            int ny = cur.second + dy[i];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h && map.isSafe(nx, ny) && !vis[ny][nx]) {
                vis[ny][nx] = true;
                q.push({nx, ny});
            }
        }
    }
    return reachableBeans == beanTotal;
}

// 递归回溯挖迷宫；返回的网格外围一圈为墙，内部连通
std::vector<std::vector<char>> carveMazeRecursiveBacktracker(int W, int H, std::mt19937& rng) {
    std::vector<std::vector<char>> g(H, std::vector<char>(W, '#'));
    if (W < 7 || H < 7) return g;

    const int cellW = (W - 3) / 2 + 1;
    const int cellH = (H - 3) / 2 + 1;
    std::vector<std::vector<bool>> visited(static_cast<size_t>(cellH), std::vector<bool>(static_cast<size_t>(cellW), false));

    std::function<void(int, int)> dfs = [&](int ci, int cj) {
        visited[static_cast<size_t>(cj)][static_cast<size_t>(ci)] = true;
        int mx = 1 + 2 * ci;
        int my = 1 + 2 * cj;
        g[my][mx] = ' ';

        std::vector<std::pair<int, int>> dirs = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
        std::shuffle(dirs.begin(), dirs.end(), rng);

        for (const auto& d : dirs) {
            int ni = ci + d.first;
            int nj = cj + d.second;
            if (ni < 0 || ni >= cellW || nj < 0 || nj >= cellH) continue;
            if (visited[static_cast<size_t>(nj)][static_cast<size_t>(ni)]) continue;

            if (d.first == 1) g[my][mx + 1] = ' ';
            else if (d.first == -1) g[my][mx - 1] = ' ';
            else if (d.second == 1) g[my + 1][mx] = ' ';
            else if (d.second == -1) g[my - 1][mx] = ' ';

            dfs(ni, nj);
        }
    };

    dfs(0, 0);
    return g;
}

// 在迷宫中放置 P、普通豆与少量大豆；保证从 P 可到达所有豆子
bool generateRandomPacmanMap(Map& map, int sizeTier, std::mt19937& rng) {
    int W = 21;
    int H = 11;
    if (sizeTier == 2) {
        W = 31;
        H = 17;
    } else if (sizeTier == 3) {
        W = 41;
        H = 21;
    }

    const int maxAttempts = 80;
    for (int attempt = 0; attempt < maxAttempts; ++attempt) {
        auto g = carveMazeRecursiveBacktracker(W, H, rng);

        std::vector<std::pair<int, int>> walkable;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                if (g[y][x] == ' ') walkable.push_back({x, y});
            }
        }
        if (walkable.empty()) continue;

        std::shuffle(walkable.begin(), walkable.end(), rng);
        int px = walkable[0].first;
        int py = walkable[0].second;

        for (const auto& p : walkable) {
            g[p.second][p.first] = '.';
        }
        g[py][px] = 'P';

        int bigBeanTarget = 1;
        if (sizeTier == 2) bigBeanTarget = 2;
        else if (sizeTier == 3) bigBeanTarget = 3;

        std::vector<std::pair<int, int>> candidates;
        for (const auto& p : walkable) {
            if (!(p.first == px && p.second == py)) candidates.push_back(p);
        }
        std::shuffle(candidates.begin(), candidates.end(), rng);
        for (int i = 0; i < bigBeanTarget && i < static_cast<int>(candidates.size()); ++i) {
            g[candidates[i].second][candidates[i].first] = 'o';
        }

        if (!map.loadFromGrid(std::move(g))) continue;

        auto spawn = findPlayerSpawnReadOnly(map);
        if (!allBeansReachableFrom(map, spawn.first, spawn.second)) continue;

        return true;
    }
    return false;
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

// 幽灵相对玩家参考步频的比：Easy 50%、Medium 70%、Hard 90%（数值越大幽灵越快）
double ghostSpeedRatio(Difficulty d) {
    if (d == Difficulty::Medium) return 0.7;
    if (d == Difficulty::Hard) return 0.9;
    return 0.5;
}

int ghostChaseIntervalMs(Difficulty d) {
    const int kPlayerRefStepMs = 200;
    return static_cast<int>(kPlayerRefStepMs / ghostSpeedRatio(d) + 0.5);
}

// 无尽模式：幽灵移动间隔在原有基础上再延长 20%（再慢 20%）
int ghostChaseIntervalMsEndless(Difficulty d) {
    return ghostChaseIntervalMs(d) * 120 / 100;
}

void refreshGhostRandomInterval(GhostState& ghost, Difficulty speedTier, bool endlessMode, std::mt19937& rng) {
    int base = endlessMode ? ghostChaseIntervalMsEndless(speedTier) : ghostChaseIntervalMs(speedTier);
    ghost.moveInterval = std::chrono::milliseconds(randBetween(base * 55 / 100, base * 175 / 100, rng));
}

void moveGhostRandom(GhostState& ghost, const Map& map, Difficulty speedTier, bool endlessMode, std::mt19937& rng) {
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
    refreshGhostRandomInterval(ghost, speedTier, endlessMode, rng);
}

void moveGhostChase(GhostState& ghost, const Map& map, int playerX, int playerY) {
    auto nxt = bfsNextStep(map, ghost.x, ghost.y, playerX, playerY);
    ghost.x = nxt.first;
    ghost.y = nxt.second;
}

void waitForEnter(const std::string& prompt = "Press Enter to go back...");

void getAnimationFrameSize(int mapCellW, int mapCellH, int& rows, int& cols) {
    rows = 28;
    cols = 100;
#ifndef _WIN32
    struct winsize w {};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &w) == 0 && w.ws_row >= 12 && w.ws_col >= 50) {
        rows = w.ws_row;
        cols = w.ws_col;
        return;
    }
#endif
    rows = std::max(rows, mapCellH * 3 + 16);
    cols = std::max(cols, mapCellW * 3 + 28);
}

void drawSolidBackgroundScreen(int rows, int cols, const std::string& ansiBg) {
    clearAndHomeCursor();
    for (int y = 0; y < rows; ++y) {
        std::cout << ansiBg << std::string(static_cast<size_t>(cols), ' ') << RESET << "\n";
    }
}

void drawCenteredLineBlock(int rows, int cols, int startRow, const std::vector<std::string>& lines, const std::string& color) {
    for (size_t i = 0; i < lines.size(); ++i) {
        int r = startRow + static_cast<int>(i);
        if (r < 1 || r > rows) continue;
        int c = std::max(1, cols / 2 - static_cast<int>(lines[i].size() / 2));
        std::cout << "\033[" << r << ";" << c << "H" << color << lines[i] << RESET;
    }
}

// 纯线条宝箱：关盖为梯形盖 + 箱体 + 包角；开盖为后掀盖 + 露膛与金币轮廓（统一行宽便于居中）
void drawLineArtChestOnly(int rows, int cols) {
    const std::vector<std::string> closed = {
<<<<<<< HEAD
       "             ******       ******             ",
    "           **      **   **      **           ",
    "         **          **           **         ",
    "        **                         **        ",
    "        **          +5 PTS         **        ",
    "         **                       **         ",
    "           **                   **           ",
    "             **               **             ",
    "               **           **               ",
    "                 **       **                 ",
    "                   *******                   "
=======
        "            _________________________            ",
        "           /                         \\           ",
        "          /___________________________\\          ",
        "         |_____________________________|         ",
        "         |         +---------+         |         ",
        "         |         |   $     |         |         ",
        "         |         +---------+         |         ",
        "         |                             |         ",
        "         |_____________________________|         ",
        "              \\___/               \\___/              ",
>>>>>>> c0fd6f4293ca85bf4ce8c1c77d26e9922d30b5c1
    };
    int startRow = std::max(1, rows / 2 - static_cast<int>(closed.size() / 2));
    drawCenteredLineBlock(rows, cols, startRow, closed, BOLD + BRIGHT_YELLOW);
}

void drawLineArtChestOpen(int rows, int cols) {
    const std::vector<std::string> open = {
<<<<<<< HEAD
       "             ******       ******             ",
    "           **      **   **      **           ",
    "         **          **           **         ",
    "        **                         **        ",
    "        **          +5 PTS         **        ",
    "         **                       **         ",
    "           **                   **           ",
    "             **               **             ",
    "               **           **               ",
    "                 **       **                 ",
    "                   *******                   "
=======
        "              ___________________              ",
        "             /                   \\             ",
        "            /_____________________\\            ",
        "           /                       \\           ",
        "          /_______________________\\_______     ",
        "         |                             |    ",
        "         |        (   $ $ $   )        |    ",
        "         |       /~~~~~~~~~~~~~\\       |    ",
        "         |       \\_____________/       |    ",
        "         |_______________________________|    ",
>>>>>>> c0fd6f4293ca85bf4ce8c1c77d26e9922d30b5c1
    };
    int startRow = std::max(1, rows / 2 - static_cast<int>(open.size() / 2));
    drawCenteredLineBlock(rows, cols, startRow, open, BOLD + BRIGHT_YELLOW);
}

std::string padCenterInWidth(const std::string& text, int innerW) {
    if (static_cast<int>(text.size()) >= innerW) return text.substr(0, static_cast<size_t>(innerW));
    int pad = innerW - static_cast<int>(text.size());
    int left = pad / 2;
    int right = pad - left;
    return std::string(static_cast<size_t>(left), ' ') + text + std::string(static_cast<size_t>(right), ' ');
}

// 奖状式外框（角花 + 双线感）+ 文案绝对居中
void drawCertificateAtCenter(int rows, int cols, const std::string& primary, const std::string& secondary,
                               const std::string& prompt) {
    const int innerW =
        std::min(cols - 8, std::max(38, std::max(static_cast<int>(primary.size()), static_cast<int>(secondary.size())) + 16));
    const int boxInner = innerW + 4;
    const std::string bc = BOLD + BRIGHT_YELLOW;
    const std::string tc = BOLD + BRIGHT_WHITE;
    const std::string sc = BRIGHT_CYAN;
    const std::string dc = DIM + BRIGHT_WHITE;

    const std::string top = "." + std::string(static_cast<size_t>(boxInner), '=') + ".";
    const std::string bot = "'" + std::string(static_cast<size_t>(boxInner), '=') + "'";
    const std::string side = "|" + std::string(static_cast<size_t>(boxInner), ' ') + "|";
    const std::string rule = "|" + std::string(static_cast<size_t>(boxInner), '-') + "|";
    const std::string cornerDeco = "| ." + std::string(static_cast<size_t>(boxInner - 4), '~') + ". |";

    std::vector<std::pair<std::string, int>> rowsOut;
    rowsOut.push_back({top, 0});
    rowsOut.push_back({cornerDeco, 0});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({"|" + padCenterInWidth("* . * . * . * . * . * . * . *", boxInner) + "|", 0});
    rowsOut.push_back({rule, 0});
    rowsOut.push_back({"|" + padCenterInWidth("C  E  R  T  I  F  I  C  A  T  E", boxInner) + "|", 0});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({rule, 0});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({"|" + padCenterInWidth(primary, boxInner) + "|", 1});
    rowsOut.push_back({side, 0});
    if (!secondary.empty()) {
        rowsOut.push_back({"|" + padCenterInWidth(secondary, boxInner) + "|", 2});
        rowsOut.push_back({side, 0});
    }
    rowsOut.push_back({rule, 0});
    rowsOut.push_back({"|" + padCenterInWidth("* . * . * . * . * . * . * . *", boxInner) + "|", 0});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({cornerDeco, 0});
    rowsOut.push_back({bot, 0});

    if (!prompt.empty()) rowsOut.push_back({padCenterInWidth(prompt, boxInner + 2), 3});

    const int certRowWidth = boxInner + 2;
    int startRow = std::max(1, rows / 2 - static_cast<int>(rowsOut.size() / 2));
    for (size_t i = 0; i < rowsOut.size(); ++i) {
        int r = startRow + static_cast<int>(i);
        if (r < 1 || r > rows) continue;
        const std::string& line = rowsOut[i].first;
        int mode = rowsOut[i].second;
        int c = (mode == 3) ? std::max(1, cols / 2 - static_cast<int>(line.size() / 2))
                            : std::max(1, cols / 2 - certRowWidth / 2);
        std::cout << "\033[" << r << ";" << c << "H";
        if (mode == 0)
            std::cout << bc << line << RESET;
        else if (mode == 1)
            std::cout << bc << "|" << tc << padCenterInWidth(primary, boxInner) << RESET << bc << "|" << RESET;
        else if (mode == 2)
            std::cout << bc << "|" << sc << padCenterInWidth(secondary, boxInner) << RESET << bc << "|" << RESET;
        else
            std::cout << dc << line << RESET;
    }
}

// 海报式画框（与奖状同结构，可换配色与条幅文案）mode: 0 框 4 条幅 1 主标题 2 副标题 3 底部提示
void drawPosterAtCenter(int rows, int cols, const std::string& ribbon, const std::string& headline, const std::string& subline,
                        const std::string& prompt, const std::string& frameCol, const std::string& ribbonCol,
                        const std::string& headlineCol, const std::string& sublineCol) {
    const int innerW = std::min(
        cols - 8, std::max(42, std::max(static_cast<int>(ribbon.size()),
                                        std::max(static_cast<int>(headline.size()), static_cast<int>(subline.size()))) +
                           20));
    const int boxInner = innerW + 4;
    const std::string bc = frameCol;
    const std::string dc = DIM + BRIGHT_WHITE;

    const std::string top = "." + std::string(static_cast<size_t>(boxInner), '=') + ".";
    const std::string bot = "'" + std::string(static_cast<size_t>(boxInner), '=') + "'";
    const std::string side = "|" + std::string(static_cast<size_t>(boxInner), ' ') + "|";
    const std::string rule = "|" + std::string(static_cast<size_t>(boxInner), '-') + "|";
    const std::string cornerDeco = "| ." + std::string(static_cast<size_t>(boxInner - 4), '~') + ". |";

    std::vector<std::pair<std::string, int>> rowsOut;
    rowsOut.push_back({top, 0});
    rowsOut.push_back({cornerDeco, 0});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({"|" + padCenterInWidth("* ~ * ~ * ~ * ~ * ~ * ~ * ~ *", boxInner) + "|", 0});
    rowsOut.push_back({rule, 0});
    rowsOut.push_back({"|" + padCenterInWidth(ribbon, boxInner) + "|", 4});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({rule, 0});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({"|" + padCenterInWidth(headline, boxInner) + "|", 1});
    rowsOut.push_back({side, 0});
    if (!subline.empty()) {
        rowsOut.push_back({"|" + padCenterInWidth(subline, boxInner) + "|", 2});
        rowsOut.push_back({side, 0});
    }
    rowsOut.push_back({rule, 0});
    rowsOut.push_back({"|" + padCenterInWidth("* ~ * ~ * ~ * ~ * ~ * ~ * ~ *", boxInner) + "|", 0});
    rowsOut.push_back({side, 0});
    rowsOut.push_back({cornerDeco, 0});
    rowsOut.push_back({bot, 0});
    if (!prompt.empty()) rowsOut.push_back({padCenterInWidth(prompt, boxInner + 2), 3});

    const int certRowWidth = boxInner + 2;
    int startRow = std::max(1, rows / 2 - static_cast<int>(rowsOut.size() / 2));
    for (size_t i = 0; i < rowsOut.size(); ++i) {
        int r = startRow + static_cast<int>(i);
        if (r < 1 || r > rows) continue;
        const std::string& line = rowsOut[i].first;
        int mode = rowsOut[i].second;
        int c = (mode == 3) ? std::max(1, cols / 2 - static_cast<int>(line.size() / 2))
                            : std::max(1, cols / 2 - certRowWidth / 2);
        std::cout << "\033[" << r << ";" << c << "H";
        if (mode == 0)
            std::cout << bc << line << RESET;
        else if (mode == 1)
            std::cout << bc << "|" << headlineCol << padCenterInWidth(headline, boxInner) << RESET << bc << "|" << RESET;
        else if (mode == 2)
            std::cout << bc << "|" << sublineCol << padCenterInWidth(subline, boxInner) << RESET << bc << "|" << RESET;
        else if (mode == 4)
            std::cout << bc << "|" << ribbonCol << padCenterInWidth(ribbon, boxInner) << RESET << bc << "|" << RESET;
        else
            std::cout << dc << line << RESET;
    }
}

// 分阶段：仅线条宝箱（关 → 开）→ 奖状式居中文案 → Enter
void playTreasureRevealAnimation(const std::string& primary, const std::string& secondary, int mapCellW, int mapCellH) {
    int rows = 28, cols = 100;
    getAnimationFrameSize(mapCellW, mapCellH, rows, cols);
    const std::string bg = "\033[40m";

    drawSolidBackgroundScreen(rows, cols, bg);
    drawLineArtChestOnly(rows, cols);
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(650));

    drawSolidBackgroundScreen(rows, cols, bg);
    drawLineArtChestOpen(rows, cols);
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(650));

    drawSolidBackgroundScreen(rows, cols, bg);
    drawCertificateAtCenter(rows, cols, primary, secondary, "");
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    drawSolidBackgroundScreen(rows, cols, bg);
    drawCertificateAtCenter(rows, cols, primary, secondary, "Press Enter to continue...");
    std::cout.flush();
    showCursor();
    waitForEnter("");
    hideCursor();
}

void playChestAnimation(int mapCellW, int mapCellH) {
    playTreasureRevealAnimation("TREASURE!", "HP +1", mapCellW, mapCellH);
}

void playLevelUpAnimation(int newTier, const std::string& perkLine, int mapCellW, int mapCellH) {
    playTreasureRevealAnimation("LEVEL UP!  Tier " + std::to_string(newTier), perkLine, mapCellW, mapCellH);
}

void playDifficultyWarningAnimation(int newTier, int mapCellW, int mapCellH) {
    int rows = 28, cols = 100;
    getAnimationFrameSize(mapCellW, mapCellH, rows, cols);
    std::string sub =
        (newTier == 2) ? "Ghosts: half chase (Medium). Stay sharp!"
                       : "All ghosts chase (Hard). Maximum pressure!";
    drawSolidBackgroundScreen(rows, cols, "\033[40m");
    drawPosterAtCenter(rows, cols, "! !  W A R N I N G  ! !", "DIFFICULTY  ESCALATED", sub, "", BOLD + BRIGHT_RED, BOLD + BRIGHT_YELLOW,
                       BOLD + BRIGHT_RED, BRIGHT_WHITE);
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(550));
    drawSolidBackgroundScreen(rows, cols, "\033[40m");
    drawPosterAtCenter(rows, cols, "! !  W A R N I N G  ! !", "DIFFICULTY  ESCALATED", sub, "Press Enter to continue...", BOLD + BRIGHT_RED,
                       BOLD + BRIGHT_YELLOW, BOLD + BRIGHT_RED, BRIGHT_WHITE);
    std::cout.flush();
    showCursor();
    waitForEnter("");
    hideCursor();
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
    if (mapChoice == 3) return "Large";
    return "Random";
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

// 无尽三层经验条（每层约占 1/3 总量）
void drawEndlessTripleXpBar(int xpIntoTier, int xpPerTier) {
    const int segW = 8;
    const int perSeg = xpPerTier / 3;
    std::cout << BOLD << BRIGHT_WHITE << std::setw(12) << std::left << std::string("XP (x3)") << RESET << " ";
    for (int layer = 0; layer < 3; ++layer) {
        int lo = layer * perSeg;
        int inSeg = std::max(0, std::min(perSeg, xpIntoTier - lo));
        int chars = perSeg > 0 ? (inSeg * segW + perSeg - 1) / perSeg : 0;
        std::cout << BRIGHT_YELLOW << "[";
        for (int i = 0; i < segW; ++i) std::cout << (i < chars ? "*" : " ");
        std::cout << "]" << RESET;
        if (layer < 2) std::cout << " ";
    }
    std::cout << " " << BRIGHT_WHITE << xpIntoTier << "/" << xpPerTier << RESET << "\n";
}

void drawAmmoBar(int ammo, int ammoMax, bool infiniteAmmo, const std::string& color) {
    const int width = 24;
    std::cout << BOLD << BRIGHT_WHITE << std::setw(12) << std::left << std::string("Ammo") << RESET << " ";
    if (infiniteAmmo) {
        std::cout << BRIGHT_MAGENTA << "[";
        for (int i = 0; i < width; ++i) std::cout << "=";
        std::cout << "] " << BOLD << "INF" << RESET << "\n";
        return;
    }
    int clamped = std::max(0, std::min(ammo, ammoMax));
    int fill = (ammoMax > 0) ? (clamped * width / ammoMax) : 0;
    std::cout << color << "[";
    for (int i = 0; i < width; ++i) std::cout << (i < fill ? "=" : " ");
    std::cout << "]" << RESET << " ";
    std::cout << BRIGHT_WHITE << clamped << "/" << ammoMax << RESET << "\n";
}

std::string gameModeLabel(bool endless) { return endless ? "Endless" : "Normal"; }

void waitForEnter(const std::string& prompt) {
    std::cout << DIM << prompt << RESET;
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
    std::cout << "- Super bean: speed boost + infinite bullets for a short time.\n";
    std::cout << "- Space: shoot (uses ammo; ammo refills slowly over time).\n";
    std::cout << "- Bullet: 1st hit stuns ghost; 2nd hit temporarily removes it.\n";
    std::cout << "- Green * walls can be shot (each hit removes one *); border === cannot.\n";
    std::cout << "- Endless mode: pellets respawn after a delay.\n";
    std::cout << "- Chest restores 1 HP and triggers a visual effect.\n";
    std::cout << "- After being hit, short invincibility (respawn in place).\n\n" << RESET;
    waitForEnter();
}

void showLegendScreen() {
    clearAndHomeCursor();
    std::cout << BOLD << BRIGHT_CYAN << "=================== Legend ===================\n" << RESET;
    std::cout << DIM << "(Playfield: each map cell = 3x3 chars, Pac-Man style sprites.)\n" << RESET;
    std::cout << BOLD << BRIGHT_YELLOW << "ooo+mouth gap" << RESET << " : Player (pie slice, mouth by direction)\n";
    std::cout << BOLD << BRIGHT_RED << "/^\\ @@@ vVv" << RESET << " : Ghost   " << BOLD << BRIGHT_CYAN << "0 0 @@@ ~~~" << RESET << " : Frightened\n";
    std::cout << WHITE << " . " << RESET << " : Pellet   " << BRIGHT_MAGENTA << " O / OOO " << RESET << " : Power pellet\n";
    std::cout << BOLD << BRIGHT_YELLOW << "+-+ |$|" << RESET << " : Chest   " << BOLD << BRIGHT_GREEN << "@S@ energizer" << RESET << " : Super bean\n";
    std::cout << BOLD << BRIGHT_MAGENTA << "===" << RESET << " : Outer wall (unbreakable)\n";
    std::cout << BRIGHT_GREEN << "***" << RESET << " : Inner wall (3 hits to break, loses one * per hit)\n\n";
    std::cout << BRIGHT_WHITE << "Controls: W/A/S/D move, Space shoot, Q quit\n\n" << RESET;
    waitForEnter();
}

void showLeaderboardScreen();

MenuConfig runStartMenu() {
    MenuConfig cfg;
#ifndef _WIN32
    TerminalRawMode rawMode;
#endif
    hideCursor();
    int selected = 0;
    const int itemCount = 8;
    while (true) {
        clearAndHomeCursor();
        std::cout << BOLD << BRIGHT_CYAN << "+======================================================+\n";
        std::cout << "|               PACMAN CONSOLE EDITION                 |\n";
        std::cout << "+======================================================+\n" << RESET;
        std::cout << BRIGHT_WHITE << "Current Map: " << BRIGHT_YELLOW << mapText(cfg.mapChoice)
                  << BRIGHT_WHITE << "   Difficulty: " << BRIGHT_MAGENTA << difficultyText(cfg.difficulty)
                  << BRIGHT_WHITE << "   Mode: " << BRIGHT_GREEN << gameModeLabel(cfg.endlessMode) << RESET << "\n\n";
        std::string items[itemCount] = {
            "Start Game",
            "Leaderboard",
            "Difficulty Explanation",
            "Symbol Legend",
            "Change Map Size",
            "Change Difficulty",
            "Toggle Game Mode",
            "Quit"};
        for (int i = 0; i < itemCount; ++i) {
            bool highlight = (i == selected);
            std::string prefix = highlight ? (BOLD + BRIGHT_YELLOW + "> ") : (DIM + "  ");
            std::string textColor = highlight ? BRIGHT_WHITE : BRIGHT_GREEN;
            std::cout << prefix << textColor << items[i];
            if (i == 4) std::cout << BRIGHT_CYAN << "  [" << mapText(cfg.mapChoice) << "]";
            if (i == 5) std::cout << BRIGHT_CYAN << "  [" << difficultyText(cfg.difficulty) << "]";
            if (i == 6) std::cout << BRIGHT_CYAN << "  [" << gameModeLabel(cfg.endlessMode) << "]";
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
            showLeaderboardScreen();
            hideCursor();
        } else if (selected == 2) {
            showCursor();
            showDifficultyHelpScreen();
            hideCursor();
        } else if (selected == 3) {
            showCursor();
            showLegendScreen();
            hideCursor();
        } else if (selected == 4) {
            cfg.mapChoice = (cfg.mapChoice % 4) + 1;
        } else if (selected == 5) {
            int d = static_cast<int>(cfg.difficulty);
            d = (d % 3) + 1;
            cfg.difficulty = static_cast<Difficulty>(d);
        } else if (selected == 6) {
            cfg.endlessMode = !cfg.endlessMode;
        } else {
            cfg.startGame = false;
            showCursor();
            return cfg;
        }
    }
}

bool leaderEntryBetter(const LeaderEntry& a, const LeaderEntry& b) {
    if (a.score != b.score) return a.score > b.score;
    if (a.timeSec != b.timeSec) return a.timeSec < b.timeSec;
    return a.name < b.name;
}

bool qualifiesTopFiveLeaderboard(int score, int timeSec, const std::vector<LeaderEntry>& board) {
    int strictlyBetter = 0;
    for (const auto& e : board) {
        if (e.score > score)
            ++strictlyBetter;
        else if (e.score == score && e.timeSec < timeSec)
            ++strictlyBetter;
    }
    return strictlyBetter < 5;
}

std::string sanitizeLeaderSignature(std::string s) {
    while (!s.empty() && (s.back() == ' ' || s.back() == '\r')) s.pop_back();
    size_t i = 0;
    while (i < s.size() && s[i] == ' ') ++i;
    if (i > 0) s.erase(0, i);
    std::string t;
    for (char c : s) {
        if (c == '\t' || c == '\n' || c == '\r') continue;
        if (static_cast<unsigned char>(c) < 32) continue;
        if (t.size() >= 16) break;
        t += c;
    }
    if (t.empty()) t = "Player";
    return t;
}

std::vector<LeaderEntry> loadLeaderboard() {
    std::vector<LeaderEntry> out;
    std::ifstream in(kLeaderboardPath);
    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) continue;
        size_t t1 = line.find('\t');
        size_t t2 = (t1 == std::string::npos) ? std::string::npos : line.find('\t', t1 + 1);
        if (t1 == std::string::npos || t2 == std::string::npos) continue;
        try {
            LeaderEntry e;
            e.score = std::stoi(line.substr(0, t1));
            e.timeSec = std::stoi(line.substr(t1 + 1, t2 - t1 - 1));
            e.name = line.substr(t2 + 1);
            if (e.name.empty()) e.name = "---";
            out.push_back(e);
        } catch (...) {
            continue;
        }
    }
    if (out.empty()) {
        std::ifstream leg(kLegacyScoresPath);
        int s = 0;
        while (leg >> s) {
            LeaderEntry e;
            e.score = s;
            e.timeSec = 0;
            e.name = "---";
            out.push_back(e);
        }
    }
    return out;
}

void saveLeaderboard(const std::vector<LeaderEntry>& entries) {
    std::ofstream out(kLeaderboardPath, std::ios::trunc);
    for (const auto& e : entries) {
        std::string name = e.name;
        for (char& c : name) {
            if (c == '\t' || c == '\n' || c == '\r') c = '_';
        }
        out << e.score << '\t' << e.timeSec << '\t' << name << '\n';
    }
}

std::string formatElapsedMmSs(int totalSec) {
    totalSec = std::max(0, totalSec);
    int m = totalSec / 60;
    int s = totalSec % 60;
    std::ostringstream os;
    os << m << ":" << std::setw(2) << std::setfill('0') << s;
    return os.str();
}

void showLeaderboardScreen() {
    clearAndHomeCursor();
    std::vector<LeaderEntry> v = loadLeaderboard();
    std::sort(v.begin(), v.end(), leaderEntryBetter);
    std::cout << BOLD << BRIGHT_CYAN << "==================== Leaderboard ====================\n" << RESET;
    std::cout << DIM << "(Sorted by score, then time)\n" << RESET;
    const size_t showN = std::min<size_t>(10, v.size());
    for (size_t i = 0; i < showN; ++i) {
        std::cout << BRIGHT_WHITE << std::setw(2) << (i + 1) << ". " << RESET << std::setw(16) << std::left
                  << v[i].name << RESET << "  " << BRIGHT_YELLOW << std::setw(8) << v[i].score << RESET << "  "
                  << BRIGHT_CYAN << formatElapsedMmSs(v[i].timeSec) << RESET << "\n";
    }
    if (v.empty()) std::cout << DIM << "(No entries yet. Play a game!)\n" << RESET;
    std::cout << "\n";
    showCursor();
    waitForEnter("Press Enter to return to menu...");
    hideCursor();
}

void showEndAnimation(bool won) {
    int rows = 28, cols = 100;
    getAnimationFrameSize(20, 14, rows, cols);
    hideCursor();
    drawSolidBackgroundScreen(rows, cols, "\033[40m");
    if (won) {
        drawPosterAtCenter(rows, cols, "*  V  I  C  T  O  R  Y  *", "Y O U   W I N !", "Pellets cleared — outstanding run!", "",
                           BOLD + BRIGHT_YELLOW, BOLD + BRIGHT_GREEN, BOLD + BRIGHT_GREEN, DIM + BRIGHT_WHITE);
    } else {
        drawPosterAtCenter(rows, cols, "G  A  M  E   O  V  E  R", "T H E   G H O S T S   W I N", "The maze will remember this run.", "",
                           BOLD + BRIGHT_RED, BOLD + BRIGHT_YELLOW, BOLD + BRIGHT_RED, DIM + BRIGHT_WHITE);
    }
    std::cout.flush();
    std::this_thread::sleep_for(std::chrono::milliseconds(600));
    drawSolidBackgroundScreen(rows, cols, "\033[40m");
    if (won) {
        drawPosterAtCenter(rows, cols, "*  V  I  C  T  O  R  Y  *", "Y O U   W I N !", "Pellets cleared — outstanding run!",
                           "Press Enter to see results...", BOLD + BRIGHT_YELLOW, BOLD + BRIGHT_GREEN, BOLD + BRIGHT_GREEN,
                           DIM + BRIGHT_WHITE);
    } else {
        drawPosterAtCenter(rows, cols, "G  A  M  E   O  V  E  R", "T H E   G H O S T S   W I N", "The maze will remember this run.",
                           "Press Enter to see results...", BOLD + BRIGHT_RED, BOLD + BRIGHT_YELLOW, BOLD + BRIGHT_RED,
                           DIM + BRIGHT_WHITE);
    }
    std::cout.flush();
    showCursor();
    waitForEnter("");
    hideCursor();
}

SessionEndAction showGameOverScreen(bool won, int score, int timeSec) {
    showEndAnimation(won);
    std::vector<LeaderEntry> board = loadLeaderboard();
    std::string signature = "---";
    if (qualifiesTopFiveLeaderboard(score, timeSec, board)) {
        clearAndHomeCursor();
        showCursor();
        std::cout << BOLD << BRIGHT_YELLOW << "You made the Top 5!\n" << RESET;
        std::cout << BRIGHT_WHITE << "Leave your signature (max 16 chars): " << RESET;
        std::cout.flush();
        std::string line;
        std::getline(std::cin, line);
        signature = sanitizeLeaderSignature(line);
        hideCursor();
    }
    LeaderEntry cur;
    cur.score = score;
    cur.timeSec = timeSec;
    cur.name = signature;
    board.push_back(cur);
    std::sort(board.begin(), board.end(), leaderEntryBetter);
    if (board.size() > 10) board.resize(10);
    saveLeaderboard(board);

    clearAndHomeCursor();
    std::cout << BOLD << BRIGHT_CYAN << "+======================================================+\n" << RESET;
    std::cout << BOLD << (won ? BRIGHT_GREEN : BRIGHT_RED) << (won ? "|                     YOU WIN!                         |\n"
                                                                    : "|                    GAME OVER                         |\n")
              << RESET;
    std::cout << BOLD << BRIGHT_CYAN << "+======================================================+\n" << RESET;
    std::cout << BRIGHT_WHITE << "Final Score: " << BOLD << BRIGHT_YELLOW << score << RESET;
    std::cout << BRIGHT_WHITE << "   Time: " << BOLD << BRIGHT_CYAN << formatElapsedMmSs(timeSec) << RESET << "\n\n";
    std::cout << BOLD << BRIGHT_MAGENTA << "Top 5\n" << RESET;
    for (size_t i = 0; i < board.size() && i < 5; ++i) {
        std::cout << BRIGHT_WHITE << std::setw(2) << (i + 1) << ". " << RESET << std::setw(14) << std::left << board[i].name
                  << RESET << "  " << BRIGHT_YELLOW << std::setw(8) << board[i].score << RESET << "  " << BRIGHT_CYAN
                  << formatElapsedMmSs(board[i].timeSec) << RESET << "\n";
    }
    std::cout << "\n" << BOLD << BRIGHT_GREEN << "[R]" << RESET << " Restart";
    std::cout << "   " << BOLD << BRIGHT_CYAN << "[M]" << RESET << " Main Menu";
    std::cout << "   " << BOLD << BRIGHT_RED << "[Q]" << RESET << " Quit\n";
    std::cout << BRIGHT_WHITE << "Press R/M/Q: " << RESET;
    std::cout.flush();

#ifndef _WIN32
    TerminalRawMode rawMode;
#endif
    showCursor();
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

// 每个地图格 = CELL_DIM x CELL_DIM；经典吃豆人风格（圆饼嘴、幽灵三角眉、波浪脚等）
static const int CELL_DIM = 3;

struct CellGlyph {
    const char* row[CELL_DIM];
};

const CellGlyph GLYPH_WALL = {{"###", "###", "###"}};
// 外围不可破坏：用 = 与配色区分；内部可破坏墙用 * 行表示耐久
const CellGlyph GLYPH_WALL_OUTER = {{"===", "===", "==="}};
const CellGlyph GLYPH_WALL_BREAK_3 = {{"***", "***", "***"}};
const CellGlyph GLYPH_WALL_BREAK_2 = {{"***", "** ", "***"}};
const CellGlyph GLYPH_WALL_BREAK_1 = {{"***", " * ", "***"}};
const CellGlyph GLYPH_EMPTY = {{"   ", "   ", "   "}};
const CellGlyph GLYPH_BEAN = {{"   ", " . ", "   "}};
const CellGlyph GLYPH_BIG = {{" O ", "OOO", " O "}};
<<<<<<< HEAD
const CellGlyph GLYPH_CHEST = {{"(V)", "\\ /", " v "}};
=======
const CellGlyph GLYPH_CHEST = {{"+-+", "|$|", "+-+"}};
>>>>>>> c0fd6f4293ca85bf4ce8c1c77d26e9922d30b5c1
const CellGlyph GLYPH_SUPER = {{"@@@", "@S@", "@@@"}};
// 幽灵：圆弧顶 + 身体 + 波浪底（类似原作剪影）
const CellGlyph GLYPH_GHOST = {{"/^\\", "@@@", "vVv"}};
const CellGlyph GLYPH_GHOST_F = {{"0 0", "@@@", "~~~"}};
const CellGlyph GLYPH_GHOST_STUN = {{"@ @", "@@@", "~~~"}}; // 眩晕
const CellGlyph GLYPH_BULLET = {{"   ", " * ", "   "}};

namespace GameCfg {
const std::chrono::milliseconds kGhostStunDur(2600);
const std::chrono::milliseconds kGhostElimDur(5000);
const std::chrono::milliseconds kBeanRespawnDelay(5500);
const int kAmmoMax = 100;
const int kAmmoFireCost = 24;
const int kAmmoRegenAmount = 3;
const std::chrono::milliseconds kAmmoRegenInterval(320);
const std::chrono::milliseconds kBulletMoveInterval(44);
const std::chrono::milliseconds kFireCooldown(260);
} // namespace GameCfg

namespace EndlessCfg {
// 升级所需经验：第 1→2 段为 kXpTierFirst，之后每升一段 +kXpTierIncrement
const int kXpTierFirst = 135;
const int kXpTierIncrement = 48;
const int kXpSmallBean = 5;
const int kXpBigBean = 14;
const int kXpStunGhost = 10;
const int kXpElimGhost = 28;
const std::chrono::milliseconds kPulseStunPeriod(8000);
const std::chrono::milliseconds kPulseStunDur(2200);

inline int xpToAdvanceFromTier(int tier) {
    return kXpTierFirst + (tier - 1) * kXpTierIncrement;
}
// 第 3 段时经验槽上限（与「若还有第 4 段」公式一致，用于显示与封顶）
inline int xpCapWhenMaxTier() { return xpToAdvanceFromTier(3); }
inline int xpMaxStoredWhenMaxTier() { return xpCapWhenMaxTier() - 1; }
} // namespace EndlessCfg

bool isDestructibleWallTile(char c) { return c >= '1' && c <= '5'; }

void normalizeMapWalls(Map& map) {
    const int w = map.getWidth();
    const int h = map.getHeight();
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            if (map.getTile(x, y) != '#') continue;
            const bool onBorder = (x == 0 || y == 0 || x == w - 1 || y == h - 1);
            map.setTile(x, y, onBorder ? '=' : '3');
        }
    }
}

void damageDestructibleWall(Map& map, int x, int y) {
    char t = map.getTile(x, y);
    if (!isDestructibleWallTile(t)) return;
    if (t == '1')
        map.setTile(x, y, ' ');
    else
        map.setTile(x, y, static_cast<char>(t - 1));
}

CellGlyph glyphBreakableWall(char tile) {
    if (tile == '3') return GLYPH_WALL_BREAK_3;
    if (tile == '2') return GLYPH_WALL_BREAK_2;
    return GLYPH_WALL_BREAK_1;
}

GhostBulletOutcome applyBulletHit(GhostState& ghost, std::chrono::steady_clock::time_point now, int& score) {
    if (now < ghost.eliminatedUntil) return GhostBulletOutcome::None;
    if (now < ghost.stunnedUntil) {
        ghost.stunnedUntil = std::chrono::steady_clock::time_point{};
        ghost.eliminatedUntil = now + GameCfg::kGhostElimDur;
        ghost.x = ghost.spawnX;
        ghost.y = ghost.spawnY;
        ghost.lastMoveTime = now;
        score += 120;
        return GhostBulletOutcome::Eliminated;
    }
    ghost.stunnedUntil = now + GameCfg::kGhostStunDur;
    score += 45;
    return GhostBulletOutcome::Stunned;
}

Difficulty endlessTierAsDifficulty(int tier) {
    if (tier >= 3) return Difficulty::Hard;
    if (tier == 2) return Difficulty::Medium;
    return Difficulty::Easy;
}

void applyEndlessTierToGhosts(std::vector<GhostState>& ghosts, int tier, std::mt19937& rng) {
    Difficulty spd = endlessTierAsDifficulty(tier);
    int chaseMs = ghostChaseIntervalMsEndless(spd);
    for (size_t i = 0; i < ghosts.size(); ++i) {
        GhostMode mode = GhostMode::Random;
        if (tier >= 3)
            mode = GhostMode::Chase;
        else if (tier == 2)
            mode = (i % 2 == 0) ? GhostMode::Chase : GhostMode::Random;
        ghosts[i].mode = mode;
        int intervalMs =
            (mode == GhostMode::Chase) ? chaseMs : randBetween(chaseMs * 55 / 100, chaseMs * 175 / 100, rng);
        ghosts[i].moveInterval = std::chrono::milliseconds(intervalMs);
    }
}

void endlessAddXp(int amount, bool endlessMode, int& xp, int& tier, std::chrono::steady_clock::time_point now, Map& gameMap,
                  std::vector<GhostState>& ghosts, std::mt19937& rng, std::chrono::steady_clock::time_point& multiShotBuffEnd,
                  std::chrono::steady_clock::time_point& pulseStunBuffEnd, std::chrono::steady_clock::time_point& nextPulseStunAt) {
    if (!endlessMode) return;
    if (tier >= 3) {
        xp = std::min(xp + amount, EndlessCfg::xpMaxStoredWhenMaxTier());
        return;
    }
    xp += amount;
    while (tier < 3) {
        const int cap = EndlessCfg::xpToAdvanceFromTier(tier);
        if (xp < cap) break;
        xp -= cap;
        ++tier;
        applyEndlessTierToGhosts(ghosts, tier, rng);
        int pick = rand() % 2;
        std::string perkLine;
        if (pick == 0) {
            multiShotBuffEnd = now + std::chrono::seconds(45);
            perkLine = "Reward: Triple shot 45s";
        } else {
            pulseStunBuffEnd = now + std::chrono::seconds(90);
            nextPulseStunAt = now + EndlessCfg::kPulseStunPeriod;
            perkLine = "Reward: Pulse stun every " + std::to_string(EndlessCfg::kPulseStunPeriod.count() / 1000) + "s (90s)";
        }
        playLevelUpAnimation(tier, perkLine, gameMap.getWidth(), gameMap.getHeight());
        playDifficultyWarningAnimation(tier, gameMap.getWidth(), gameMap.getHeight());
    }
    if (tier >= 3) xp = std::min(xp, EndlessCfg::xpMaxStoredWhenMaxTier());
}

void ghostBulletGrantXp(GhostBulletOutcome o, bool endlessMode, int& xp, int& tier, std::chrono::steady_clock::time_point now,
                        Map& gameMap, std::vector<GhostState>& ghosts, std::mt19937& rng,
                        std::chrono::steady_clock::time_point& multiShotBuffEnd,
                        std::chrono::steady_clock::time_point& pulseStunBuffEnd,
                        std::chrono::steady_clock::time_point& nextPulseStunAt) {
    if (!endlessMode || o == GhostBulletOutcome::None) return;
    int amt = (o == GhostBulletOutcome::Eliminated) ? EndlessCfg::kXpElimGhost : EndlessCfg::kXpStunGhost;
    endlessAddXp(amt, endlessMode, xp, tier, now, gameMap, ghosts, rng, multiShotBuffEnd, pulseStunBuffEnd, nextPulseStunAt);
}

// 玩家：圆饼缺一角表示嘴，缺口朝向移动方向（Pac-Man 标志性披萨切片）
CellGlyph glyphPlayerCell(char facing) {
    if (facing == 'w')
        return CellGlyph{{"o o", "ooo", "ooo"}}; // 嘴朝上
    if (facing == 's')
        return CellGlyph{{"ooo", "ooo", "o o"}}; // 嘴朝下
    if (facing == 'a')
        return CellGlyph{{"ooo", " oo", "ooo"}}; // 嘴朝左
    if (facing == 'd')
        return CellGlyph{{"ooo", "oo ", "ooo"}}; // 嘴朝右
    return CellGlyph{{"ooo", "oo ", "ooo"}};
}

void drawGame(const Map& map, int playerX, int playerY, char playerFacing, const std::vector<GhostState>& ghosts,
              const std::vector<Bullet>& bullets, std::chrono::steady_clock::time_point frameNow, int hp, int maxHp, int chestLeft,
              int superBeanLeft, Difficulty hudDifficulty, bool speedBoostOn, bool playerInvincible, int score, int elapsedSec,
              int superRemainMs, int superTotalMs, int invRemainMs, int invTotalMs, int ammoCur, int ammoMaxVal, bool endlessMode,
              int endlessTier, int endlessXp, int endlessXpPerTier, bool multiShotBuff, int multiRemainMs, bool pulseStunBuff,
              int pulseRemainMs, int pulseTotalMs) {
    char face = playerFacing;
    if (face != 'w' && face != 'a' && face != 's' && face != 'd') face = 'd';

    clearAndHomeCursor();
    std::string diffText = difficultyText(hudDifficulty);
    if (endlessMode) diffText = "Endless " + diffText + " [T" + std::to_string(endlessTier) + "/3]";

    std::cout << BOLD << BRIGHT_CYAN << "+==============================================================+\n" << RESET;
    std::cout << BOLD << BRIGHT_CYAN << "| " << BRIGHT_YELLOW << "PACMAN REALTIME" << BRIGHT_CYAN
              << " | " << BRIGHT_GREEN << "Score: " << score << BRIGHT_CYAN << " | " << BRIGHT_WHITE
              << "Time: " << formatElapsedMmSs(elapsedSec) << BRIGHT_CYAN << " | " << BRIGHT_RED << "HP: " << hpBar(hp, maxHp)
              << BRIGHT_CYAN << " | " << BRIGHT_MAGENTA << "Diff: " << diffText << BRIGHT_CYAN;
    std::cout << " | " << BRIGHT_WHITE << (endlessMode ? "ENDLESS" : "NORMAL") << BRIGHT_CYAN << " |\n" << RESET;
    std::cout << BOLD << BRIGHT_CYAN << "+==============================================================+\n" << RESET;

    for (int y = 0; y < map.getHeight(); ++y) {
        for (int sub = 0; sub < CELL_DIM; ++sub) {
            std::cout << BRIGHT_BLUE << "|" << RESET;
            for (int x = 0; x < map.getWidth(); ++x) {
                CellGlyph g = GLYPH_EMPTY;
                std::string pre;
                std::string suf = RESET;

                if (x == playerX && y == playerY) {
                    g = glyphPlayerCell(face);
                    pre = (playerInvincible && (std::rand() % 2 == 0)) ? DIM + BRIGHT_YELLOW : BOLD + BRIGHT_YELLOW;
                } else {
                    bool bulletHere = false;
                    for (const auto& b : bullets) {
                        if (b.x == x && b.y == y) {
                            bulletHere = true;
                            break;
                        }
                    }
                    if (bulletHere) {
                        g = GLYPH_BULLET;
                        pre = BOLD + BRIGHT_WHITE;
                    } else {
                        bool ghostHere = false;
                        for (const auto& gh : ghosts) {
                            if (frameNow < gh.eliminatedUntil) continue;
                            if (gh.x == x && gh.y == y) {
                                ghostHere = true;
                                if (frameNow < gh.stunnedUntil) {
                                    g = GLYPH_GHOST_STUN;
                                    pre = DIM + BRIGHT_RED;
                                } else if (speedBoostOn) {
                                    g = GLYPH_GHOST_F;
                                    pre = BOLD + BRIGHT_CYAN;
                                } else {
                                    g = GLYPH_GHOST;
                                    pre = BOLD + BRIGHT_RED;
                                }
                                break;
                            }
                        }
                        if (!ghostHere) {
                            char tile = map.getTile(x, y);
                            if (tile == '=') {
                                g = GLYPH_WALL_OUTER;
                                pre = BOLD + BRIGHT_MAGENTA;
                            } else if (tile == '#') {
                                g = GLYPH_WALL;
                                pre = BRIGHT_BLUE;
                            } else if (isDestructibleWallTile(tile)) {
                                g = glyphBreakableWall(tile);
                                pre = BRIGHT_GREEN;
                            } else if (tile == '.') {
                                g = GLYPH_BEAN;
                                pre = WHITE;
                            } else if (tile == 'o') {
                                g = GLYPH_BIG;
                                pre = BRIGHT_MAGENTA;
                            } else if (tile == 'C') {
                                g = GLYPH_CHEST;
                                pre = BOLD + BRIGHT_YELLOW;
                            } else if (tile == 'S') {
                                g = GLYPH_SUPER;
                                pre = BOLD + BRIGHT_GREEN;
                            }
                        }
                    }
                }

                const char* line = g.row[sub];
                if (pre.empty())
                    std::cout << line;
                else
                    std::cout << pre << line << suf;
            }
            std::cout << BRIGHT_BLUE << "|" << RESET << '\n';
        }
    }
    std::cout << BOLD << BRIGHT_CYAN << "+==============================================================+\n" << RESET;

    std::cout << BOLD << BRIGHT_WHITE << "Beans: " << map.countBeans()
              << " | Chests: " << chestLeft
              << " | SuperBeans: " << superBeanLeft
              << " | Super(+INF ammo): " << (speedBoostOn ? BRIGHT_GREEN + std::string("ON") + RESET : BRIGHT_RED + std::string("OFF") + RESET)
              << " | Invincible: " << (playerInvincible ? BRIGHT_GREEN + std::string("ON") + RESET : BRIGHT_RED + std::string("OFF") + RESET)
              << "\n" << RESET;
    drawAmmoBar(ammoCur, ammoMaxVal, speedBoostOn, BRIGHT_CYAN);
    drawTimeBar("SuperMode", superRemainMs, superTotalMs, BRIGHT_GREEN);
    drawTimeBar("Invincible", invRemainMs, invTotalMs, BRIGHT_YELLOW);
    if (endlessMode) {
        int dispXp = endlessXp;
        if (endlessTier >= 3) dispXp = endlessXpPerTier;
        drawEndlessTripleXpBar(dispXp, endlessXpPerTier);
        if (multiShotBuff) drawTimeBar("MultiShot", multiRemainMs, 45000, BRIGHT_MAGENTA);
        if (pulseStunBuff) drawTimeBar("PulseStun", pulseRemainMs, pulseTotalMs, BRIGHT_CYAN);
    }
    std::cout << DIM << "WASD: one step/tap | Space fire | Q quit | Super bean: eat ghosts + INF ammo" << RESET << "\n";
}
} // namespace

int main() {
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    bool needMenu = true;
    int mapChoice = 1;
    Difficulty difficulty = Difficulty::Easy;
    bool endlessMode = false;

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
            endlessMode = menuConfig.endlessMode;
        }

        std::string mapFile = "map_small.txt";
        int activeTier = mapChoice;
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
        } else if (mapChoice == 4) {
            activeTier = randBetween(1, 3, rng);
            if (activeTier == 2) {
                ghostCount = 3;
                chestCount = 3;
                superBeanCount = 2;
            } else if (activeTier == 3) {
                ghostCount = 4;
                chestCount = 5;
                superBeanCount = 3;
            }
        }

        Map gameMap;
        if (mapChoice == 4) {
            if (!generateRandomPacmanMap(gameMap, activeTier, rng)) {
                std::cerr << "Random map generation failed after retries\n";
                return 1;
            }
        } else {
            if (!gameMap.loadFromFile(mapFile)) {
                std::cerr << "Failed to load " << mapFile << '\n';
                return 1;
            }
        }

        normalizeMapWalls(gameMap);

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

        int endlessTier = 1;

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
            if (endlessMode) {
                if (endlessTier >= 3)
                    mode = GhostMode::Chase;
                else if (endlessTier == 2)
                    mode = (i % 2 == 0) ? GhostMode::Chase : GhostMode::Random;
                else
                    mode = GhostMode::Random;
            } else if (difficulty == Difficulty::Hard) {
                mode = GhostMode::Chase;
            } else if (difficulty == Difficulty::Medium) {
                mode = (i % 2 == 0) ? GhostMode::Chase : GhostMode::Random;
            }

            Difficulty ghostSpd = endlessMode ? endlessTierAsDifficulty(endlessTier) : difficulty;
            int chaseMs = endlessMode ? ghostChaseIntervalMsEndless(ghostSpd) : ghostChaseIntervalMs(ghostSpd);
            int intervalMs =
                (mode == GhostMode::Chase) ? chaseMs : randBetween(chaseMs * 55 / 100, chaseMs * 175 / 100, rng);
            GhostState gs;
            gs.x = pos.first;
            gs.y = pos.second;
            gs.spawnX = pos.first;
            gs.spawnY = pos.second;
            gs.mode = mode;
            gs.moveInterval = std::chrono::milliseconds(intervalMs);
            gs.lastMoveTime = now;
            ghosts.push_back(gs);
        }

        int hp = 3;
        int score = 0;
        const int maxHp = 3;
        bool running = true;
        char direction = '\0';

        int endlessXp = 0;
        auto multiShotBuffEnd = std::chrono::steady_clock::time_point{};
        auto pulseStunBuffEnd = std::chrono::steady_clock::time_point{};
        auto nextPulseStunAt = std::chrono::steady_clock::time_point{};

        std::vector<Bullet> bullets;
        std::vector<PendingBean> pendingBeans;
        int ammo = GameCfg::kAmmoMax;
        auto lastAmmoRegen = now;
        auto lastFireTime = now - GameCfg::kFireCooldown;

        const auto frameTime = std::chrono::milliseconds(33);
        auto boostEndTime = now;
        const auto superDuration = std::chrono::seconds(6);
        auto invincibleEndTime = now;
        const auto hitInvincibleDuration = std::chrono::milliseconds(1200);

        std::chrono::steady_clock::time_point gameSessionStart;
#ifndef _WIN32
        {
            TerminalRawMode rawMode;
#endif
            hideCursor();
            gameSessionStart = std::chrono::steady_clock::now();

            while (running) {
                auto frameStart = std::chrono::steady_clock::now();
                bool speedBoostOn = frameStart < boostEndTime;
                bool playerInvincible = frameStart < invincibleEndTime;
                Difficulty ghostSpeedDiff = endlessMode ? endlessTierAsDifficulty(endlessTier) : difficulty;
                int elapsedSec = static_cast<int>(
                    std::chrono::duration_cast<std::chrono::seconds>(frameStart - gameSessionStart).count());

                char input = pollInput();
                bool wantFire = false;
                while (input != '\0') {
                    if (input == 'q') {
                        running = false;
                        break;
                    }
                    if (input == ' ') {
                        wantFire = true;
                        input = pollInput();
                        continue;
                    }
                    if (input == 'w' || input == 'a' || input == 's' || input == 'd') {
                        direction = input;
                        int nx = playerX;
                        int ny = playerY;
                        if (input == 'w')
                            --ny;
                        else if (input == 's')
                            ++ny;
                        else if (input == 'a')
                            --nx;
                        else
                            ++nx;
                        if (gameMap.isSafe(nx, ny)) {
                            playerX = nx;
                            playerY = ny;
                            char tile = gameMap.getTile(playerX, playerY);
                            if (tile == '.' || tile == 'o') {
                                if (endlessMode) pendingBeans.push_back({playerX, playerY, tile, frameStart + GameCfg::kBeanRespawnDelay});
                                gameMap.eatBean(playerX, playerY);
                                score += (tile == 'o') ? 25 : 10;
                                if (endlessMode) {
                                    endlessAddXp((tile == 'o') ? EndlessCfg::kXpBigBean : EndlessCfg::kXpSmallBean, endlessMode, endlessXp,
                                                   endlessTier, frameStart, gameMap, ghosts, rng, multiShotBuffEnd, pulseStunBuffEnd,
                                                   nextPulseStunAt);
                                }
                            } else if (tile == 'C') {
<<<<<<< HEAD
                                score += 50;
=======
                                hp = std::min(maxHp, hp + 1);
                                score += 80;
>>>>>>> c0fd6f4293ca85bf4ce8c1c77d26e9922d30b5c1
                                gameMap.setTile(playerX, playerY, ' ');
                                playChestAnimation(gameMap.getWidth(), gameMap.getHeight());
                            } else if (tile == 'S') {
                                gameMap.setTile(playerX, playerY, ' ');
<<<<<<< HEAD
                                speedBoostOn = true;
=======
>>>>>>> c0fd6f4293ca85bf4ce8c1c77d26e9922d30b5c1
                                boostEndTime = frameStart + superDuration;
                                score += 120;
                            }
                        }
                    }
                    input = pollInput();
                }
                if (!running) break;

                if (frameStart - lastAmmoRegen >= GameCfg::kAmmoRegenInterval) {
                    lastAmmoRegen = frameStart;
                    if (!speedBoostOn && ammo < GameCfg::kAmmoMax)
                        ammo = std::min(GameCfg::kAmmoMax, ammo + GameCfg::kAmmoRegenAmount);
                }

                if (wantFire && frameStart - lastFireTime >= GameCfg::kFireCooldown) {
                    char fd = direction ? direction : 'd';
                    bool canFire = speedBoostOn || ammo >= GameCfg::kAmmoFireCost;
                    if (canFire) {
                        bool multi = (frameStart < multiShotBuffEnd);
                        int dx = 0, dy = 0;
                        if (fd == 'w')
                            dy = -1;
                        else if (fd == 's')
                            dy = 1;
                        else if (fd == 'a')
                            dx = -1;
                        else
                            dx = 1;
                        std::vector<std::pair<int, int>> cells;
                        cells.push_back({playerX + dx, playerY + dy});
                        if (multi) {
                            if (dx == 0) {
                                cells.push_back({playerX - 1, playerY + dy});
                                cells.push_back({playerX + 1, playerY + dy});
                            } else {
                                cells.push_back({playerX + dx, playerY - 1});
                                cells.push_back({playerX + dx, playerY + 1});
                            }
                        }
                        bool consumed = false;
                        for (const auto& c : cells) {
                            int bx = c.first;
                            int by = c.second;
                            char adjTile = gameMap.getTile(bx, by);
                            if (isDestructibleWallTile(adjTile)) {
                                damageDestructibleWall(gameMap, bx, by);
                                consumed = true;
                                continue;
                            }
                            if (!gameMap.isSafe(bx, by)) continue;
                            bool immediateHit = false;
                            for (auto& gh : ghosts) {
                                if (bx == gh.x && by == gh.y && frameStart >= gh.eliminatedUntil) {
                                    GhostBulletOutcome o = applyBulletHit(gh, frameStart, score);
                                    ghostBulletGrantXp(o, endlessMode, endlessXp, endlessTier, frameStart, gameMap, ghosts, rng,
                                                       multiShotBuffEnd, pulseStunBuffEnd, nextPulseStunAt);
                                    immediateHit = true;
                                    consumed = true;
                                    break;
                                }
                            }
                            if (!immediateHit) {
                                bullets.push_back({bx, by, fd, frameStart - GameCfg::kBulletMoveInterval});
                                consumed = true;
                            }
                        }
                        if (consumed) {
                            if (!speedBoostOn) ammo -= GameCfg::kAmmoFireCost;
                            lastFireTime = frameStart;
                        }
                    }
                }

                {
                    std::vector<Bullet> survived;
                    survived.reserve(bullets.size());
                    for (Bullet b : bullets) {
                        if (frameStart - b.lastMove < GameCfg::kBulletMoveInterval) {
                            survived.push_back(b);
                            continue;
                        }
                        int nx = b.x;
                        int ny = b.y;
                        if (b.dir == 'w')
                            --ny;
                        else if (b.dir == 's')
                            ++ny;
                        else if (b.dir == 'a')
                            --nx;
                        else
                            ++nx;
                        if (!gameMap.isSafe(nx, ny)) {
                            char wt = gameMap.getTile(nx, ny);
                            if (isDestructibleWallTile(wt)) damageDestructibleWall(gameMap, nx, ny);
                            continue;
                        }
                        bool hitGhost = false;
                        for (auto& gh : ghosts) {
                            if (nx == gh.x && ny == gh.y && frameStart >= gh.eliminatedUntil) {
                                GhostBulletOutcome o = applyBulletHit(gh, frameStart, score);
                                ghostBulletGrantXp(o, endlessMode, endlessXp, endlessTier, frameStart, gameMap, ghosts, rng,
                                                   multiShotBuffEnd, pulseStunBuffEnd, nextPulseStunAt);
                                hitGhost = true;
                                break;
                            }
                        }
                        if (hitGhost) continue;
                        b.x = nx;
                        b.y = ny;
                        b.lastMove = frameStart;
                        survived.push_back(b);
                    }
                    bullets.swap(survived);
                }

                for (auto& ghost : ghosts) {
                    if (frameStart < ghost.eliminatedUntil) continue;
                    if (frameStart < ghost.stunnedUntil) continue;
                    if (frameStart - ghost.lastMoveTime >= ghost.moveInterval) {
                        if (ghost.mode == GhostMode::Random)
                            moveGhostRandom(ghost, gameMap, ghostSpeedDiff, endlessMode, rng);
                        else
                            moveGhostChase(ghost, gameMap, playerX, playerY);
                        ghost.lastMoveTime = frameStart;
                    }
                }

                if (endlessMode && frameStart < pulseStunBuffEnd && frameStart >= nextPulseStunAt) {
                    for (auto& g : ghosts) {
                        if (frameStart >= g.eliminatedUntil)
                            g.stunnedUntil = std::max(g.stunnedUntil, frameStart + EndlessCfg::kPulseStunDur);
                    }
                    nextPulseStunAt = frameStart + EndlessCfg::kPulseStunPeriod;
                }

                for (auto& ghost : ghosts) {
                    if (ghost.x == playerX && ghost.y == playerY) {
                        if (frameStart < ghost.eliminatedUntil) break;
                        if (speedBoostOn) {
                            score += 200;
<<<<<<< HEAD
                            speedBoostOn = false;
                            ghost.eliminatedUntil = frameStart + std::chrono::seconds(5);
=======
                            ghost.x = ghost.spawnX;
                            ghost.y = ghost.spawnY;
                            ghost.stunnedUntil = std::chrono::steady_clock::time_point{};
                            ghost.eliminatedUntil = std::chrono::steady_clock::time_point{};
                            ghost.lastMoveTime = frameStart + std::chrono::milliseconds(300);
>>>>>>> c0fd6f4293ca85bf4ce8c1c77d26e9922d30b5c1
                        } else if (frameStart < ghost.stunnedUntil) {
                            break;
                        } else if (!playerInvincible) {
                            --hp;
                            invincibleEndTime = frameStart + hitInvincibleDuration;
                            std::this_thread::sleep_for(std::chrono::milliseconds(180));
                        }
                        break;
                    }
                }

                if (endlessMode) {
                    for (auto it = pendingBeans.begin(); it != pendingBeans.end();) {
                        if (frameStart >= it->respawnAt) {
                            if (gameMap.getTile(it->x, it->y) == ' ') gameMap.setTile(it->x, it->y, it->type);
                            it = pendingBeans.erase(it);
                        } else
                            ++it;
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

                Difficulty hudDifficulty = endlessMode ? endlessTierAsDifficulty(endlessTier) : difficulty;
                bool multiShotBuff = frameStart < multiShotBuffEnd;
                bool pulseStunBuff = frameStart < pulseStunBuffEnd;
                int multiRemainMs =
                    std::max(0, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(multiShotBuffEnd - frameStart).count()));
                int pulseRemainMs =
                    std::max(0, static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(pulseStunBuffEnd - frameStart).count()));
                const int pulseTotalMs = static_cast<int>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(90)).count());
                const int endlessXpCapHud =
                    (endlessTier >= 3) ? EndlessCfg::xpCapWhenMaxTier() : EndlessCfg::xpToAdvanceFromTier(endlessTier);

                drawGame(gameMap, playerX, playerY, direction, ghosts, bullets, frameStart, hp, maxHp, chestLeft, superBeanLeft,
                         hudDifficulty, speedBoostOn, playerInvincible, score, elapsedSec, superRemainMs, superTotalMs, invRemainMs,
                         invTotalMs, ammo, GameCfg::kAmmoMax, endlessMode, endlessTier, endlessXp, endlessXpCapHud, multiShotBuff,
                         multiRemainMs, pulseStunBuff, pulseRemainMs, pulseTotalMs);

                if (hp <= 0) {
                    running = false;
                    break;
                }
                if (!endlessMode && !gameMap.hasBeans()) {
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

        bool won = (!endlessMode && hp > 0 && !gameMap.hasBeans());
        std::cout << "\033[0m\n";
        int sessionTimeSec = static_cast<int>(
            std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - gameSessionStart).count());
        SessionEndAction next = showGameOverScreen(won, score, sessionTimeSec);
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