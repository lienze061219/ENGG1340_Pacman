#include "Ghost.h"
#include <cstdlib>
using namespace std;

/*
 * What it does: 构造函数，初始化幽灵在地图上的初始坐标。
 * Inputs: startX (初始的 X 坐标), startY (初始的 Y 坐标)
 * Outputs: 无
 */
Ghost::Ghost(int startX, int startY) : x(startX), y(startY) {}

/*
 * What it does: 每帧被系统组调用，更新幽灵的当前状态。目前阶段执行随机移动。
 * Inputs: m (Map 对象的常量引用，用于获取地图的地形信息)
 * Outputs: 无 (修改自身的 x, y 坐标)
 */
void Ghost::update(const Map &m) {
    moveRandomly(m);
}

/*
 * What it does: 实现幽灵的随机移动逻辑，四个方向随机选择，碰到墙壁则停留在原地。
 * Inputs: m (Map 对象的常量引用)
 * Outputs: 无 (如果判断通过，修改自身的 x, y 坐标)
 */
void Ghost::moveRandomly(const Map &m) {
    int dir = rand() % 4;
    int nextX = x;
    int nextY = y;

    if (dir == 0) {
        nextY -= 1;
    } else if (dir == 1) {
        nextY += 1;
    } else if (dir == 2) {
        nextX -= 1;
    } else if (dir == 3) {
        nextX += 1;
    }

    if (m.isSafe(nextX, nextY)) {
        x = nextX;
        y = nextY;
    }
}

/*
 * What it does: 获取幽灵当前的 X 坐标。
 * Inputs: 无
 * Outputs: 幽灵当前的 X 坐标 (int)
 */
int Ghost::getX() const {
    return x;
}

/*
 * What it does: 获取幽灵当前的 Y 坐标。
 * Inputs: 无
 * Outputs: 幽灵当前的 Y 坐标 (int)
 */
int Ghost::getY() const {
    return y;
}
