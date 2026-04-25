#include "Map.h"
#include <iostream>

int main() {
    Map m;
    if (!m.loadFromFile("map.txt")) {
        std::cerr << "Failed to load map.txt\n";
        return 1;
    }

    int px = 1, py = 1; // 玩家初始位置
    int gx = 10, gy = 3; // 幽灵初始位置

    // 显示初始地图
    std::cout << "Initial map:\n";
    m.display(px, py, gx, gy);
    std::cout << "Beans remaining: " << m.countBeans() << "\n";

    // 模拟玩家向右移动并吃豆子
    if (m.isSafe(px + 1, py)) {
        px += 1;
        m.eatBean(px, py);
    }

    std::cout << "\nAfter move:\n";
    m.display(px, py, gx, gy);
    std::cout << "Beans remaining: " << m.countBeans() << "\n";

    return 0;
}
