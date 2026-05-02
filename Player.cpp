#include "Player.h"

Player::Player(int startX, int startY) {
    x = startX;
    y = startY;
}

<<<<<<< HEAD
/**
 * @brief 处理玩家移动的核心逻辑
 * @param input 用户输入的按键 (w, a, s, d)
 * @param gameMap 游戏地图对象的引用，用于碰撞检测
 */

void Player::move(char input, Map &m) {
    // 记录移动前的坐标，用于恢复或逻辑判断
    int nextX = x;
    int nextY = y;

    // 根据输入的方向计算目标位置
=======
void Player::move(char input, Map &m) {
    int nextX = x;
    int nextY = y;

>>>>>>> c0fd6f4293ca85bf4ce8c1c77d26e9922d30b5c1
    if (input == 'w') {
        nextY = y - 1;
    } else if (input == 's') {
        nextY = y + 1;
    } else if (input == 'a') {
        nextX = x - 1;
    } else if (input == 'd') {
        nextX = x + 1;
    }

    if (m.isSafe(nextX, nextY)) {
        x = nextX;
        y = nextY;
    }
}

int Player::getX() const {
    return x;
}

int Player::getY() const {
    return y;
} 