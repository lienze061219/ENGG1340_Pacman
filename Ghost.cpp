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
    
    // 校验目标点（玩家）是否在合法范围内
    if (tx < 0 || tx >= w || ty < 0 || ty >= h) return {x, y};
    // 校验起点（幽灵自身）是否在合法范围内
    if (x < 0 || x >= w || y < 0 || y >= h) return {x, y};

    std::vector<std::vector<bool>> vis(h, std::vector<bool>(w, false));
    std::vector<std::vector<std::pair<int,int>>> parent(h, std::vector<std::pair<int,int>>(w, {-1,-1}));
    
    std::queue<std::pair<int,int>> q;
    q.push({x, y});
    vis[y][x] = true;

    int dx[] = {0,0,-1,1}, dy[] = {-1,1,0,0};
    bool found = false;

    while (!q.empty()) {
        auto cur = q.front(); q.pop();
        int cx = cur.first, cy = cur.second;
        if (cx == tx && cy == ty) {
            found = true;
            break;
        }
        for (int i = 0; i < 4; ++i) {
            int nx = cx + dx[i], ny = cy + dy[i];
            // 显式的边界检查，防止 vis[ny][nx] 越界
            if (nx >= 0 && nx < w && ny >= 0 && ny < h && m.isSafe(nx, ny) && !vis[ny][nx]) {
                vis[ny][nx] = true;
                parent[ny][nx] = {cx, cy};
                q.push({nx, ny});
            }
        }
    }

    // 如果没找到路径，不执行回溯，直接返回原地或随机
    if (!found) return {x, y};

    int cx = tx, cy = ty;
    // 回溯直到找到起点的下一步
    while (parent[cy][cx].first != -1) {
        std::pair<int,int> p = parent[cy][cx];
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
