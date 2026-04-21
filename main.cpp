#include "Map.h"
#include <iostream>

int main() {
    Map gameMap;
    if (!gameMap.loadFromFile("map.txt")) return 1;

    // 临时定义的玩家位置（以后这里会被组员 A 的 Player 类替代）
    int pX = 1, pY = 1; 
    int gX = 9, gY = 3; // 幽灵位置
    bool gameRunning = true;

    while (gameRunning) {
        gameMap.display(pX, pY, gX, gY); // 刷新画面
        
        std::cout << "Move (W/A/S/D), Q to quit: ";
        char input;
        std::cin >> input;

        if (input == 'q') break;

        // 计算下一步位置
        int nextX = pX, nextY = pY;
        if (input == 'w') nextY--;
        else if (input == 's') nextY++;
        else if (input == 'a') nextX--;
        else if (input == 'd') nextX++;

        // --- 组长的核心逻辑：调用接口 ---
        if (gameMap.isSafe(nextX, nextY)) {
            pX = nextX; 
            pY = nextY;
        }

        gameMap.eatBean(pX, pY); // 尝试吃豆子

        // 死亡判定：撞到幽灵
        if (pX == gX && pY == gY) {
            std::cout << "\nGAME OVER! The ghost caught you!\n";
            gameRunning = false;
        }
        // 胜利判定：豆子吃光
        if (!gameMap.hasBeans()) {
            std::cout << "\nYOU WIN! All beans eaten!\n";
            gameRunning = false;
        }
    }
    return 0;
}