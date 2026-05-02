#include "Ghost.h"
#include <cstdlib>
#include <queue>
#include <algorithm>

Ghost::Ghost(int sx, int sy, GhostBehavior b, double interval)
    : x(sx), y(sy), behavior(b), moveInterval(interval), nextMoveTime(0) {}

void Ghost::update(double ct, const Map& m, int px, int py) {
    if (ct < nextMoveTime) return;
    nextMoveTime = ct + moveInterval;
    if (behavior == RANDOM) moveRandomly(m);
    else moveTowardsPlayer(m, px, py);
}

void Ghost::moveRandomly(const Map& m) {
    int dir = rand() % 4;
    int nx = x, ny = y;
    switch (dir) {
        case 0: ny--; break;
        case 1: ny++; break;
        case 2: nx--; break;
        case 3: nx++; break;
    }
    // 确保移动后的坐标在地图内且安全
    if (nx >= 0 && nx < m.getWidth() && ny >= 0 && ny < m.getHeight() && m.isSafe(nx, ny)) {
        x = nx; y = ny;
    }
}

std::pair<int,int> Ghost::bfsNextStep(const Map& m, int tx, int ty) const {
    int w = m.getWidth(), h = m.getHeight();
    
    if (tx < 0 || tx >= w || ty < 0 || ty >= h) return {x, y};
    if (x < 0 || x >= w || y < 0 || y >= h) return {x, y};
    if (x == tx && y == ty) return {x, y};

    std::vector<std::vector<bool>> vis(h, std::vector<bool>(w, false));
    std::vector<std::vector<std::pair<int,int>>> parent(h, std::vector<std::pair<int,int>>(w, {-1, -1}));
    std::vector<std::pair<int,int>> q;
    q.reserve(static_cast<size_t>(w) * static_cast<size_t>(h));

    q.emplace_back(x, y);
    vis[y][x] = true;

    const int dx[] = {0, 0, -1, 1};
    const int dy[] = {-1, 1, 0, 0};
    size_t qi = 0;

    while (qi < q.size()) {
        auto cur = q[qi++];
        int cx = cur.first, cy = cur.second;
        if (cx == tx && cy == ty) break;
        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i];
            int ny = cy + dy[i];
            if (nx >= 0 && nx < w && ny >= 0 && ny < h && m.isSafe(nx, ny) && !vis[ny][nx]) {
                vis[ny][nx] = true;
                parent[ny][nx] = {cx, cy};
                q.emplace_back(nx, ny);
            }
        }
    }

    if (!vis[ty][tx]) return {x, y};

    int cx = tx, cy = ty;
    while (parent[cy][cx].first != -1) {
        auto p = parent[cy][cx];
        if (p.first == x && p.second == y) return {cx, cy};
        cx = p.first;
        cy = p.second;
    }
    return {x, y};
}

void Ghost::moveTowardsPlayer(const Map& m, int px, int py) {
    auto nxt = bfsNextStep(m, px, py);
    int nx = nxt.first, ny = nxt.second;
    // 再次确认安全性
    if ((nx != x || ny != y) && nx >= 0 && nx < m.getWidth() && ny >= 0 && ny < m.getHeight() && m.isSafe(nx, ny)) {
        x = nx; y = ny;
    } else {
        moveRandomly(m);
    }
}
// 其他 getter/setter 保持不变...

int Ghost::getX() const { return x; }
int Ghost::getY() const { return y; }
void Ghost::setBehavior(GhostBehavior b) { behavior = b; }
double Ghost::getMoveInterval() const { return moveInterval; }
