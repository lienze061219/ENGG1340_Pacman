#include "Map.h"
#include "Player.h"
#include <iostream>

int main() {
    Map gameMap;
    if (!gameMap.loadFromFile("map.txt")) return 1;

    // 临时定义的玩家位置（以后这里会被组员 A 的 Player 类替代）
    Player player(1, 1);
    int gX = 9, gY = 3; // 幽灵位置
    bool gameRunning = true;

    while (gameRunning) {
        gameMap.display(player.getX(), player.getY(), gX, gY); // 刷新画面
        
        std::cout << "Move (W/A/S/D), Q to quit: ";
        char input;
        std::cin >> input;

        if (input == 'q') break;

        // 计算下一步位置
        player.move(input, gameMap);

        gameMap.eatBean(player.getX(), player.getY()); // 尝试吃豆子

        // 死亡判定：撞到幽灵
        if (player.getX() == gX && player.getY() == gY) {
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