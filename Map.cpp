#include "Map.h"
#include <iostream>
#include <fstream>

bool Map::loadFromFile(std::string filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return false;
    std::string line;
    data.clear();
    while (getline(file, line)) {
        std::vector<char> row(line.begin(), line.end());
        data.push_back(row);
    }
    file.close();
    return true;
}

void Map::display(int px, int py, int gx, int gy) {
    std::cout << "\033[2J\033[1;1H"; // 清屏
    for (int y = 0; y < data.size(); y++) {
        for (int x = 0; x < data[y].size(); x++) {
            if (x == px && y == py) std::cout << "P "; // 画玩家
            else if (x == gx && y == gy) std::cout << "G "; // 画幽灵
            else std::cout << data[y][x] << " "; // 画墙或豆子
        }
        std::cout << "\n";
    }
}

bool Map::isSafe(int x, int y) {
    // 越界保护：如果坐标超出地图范围，直接返回 false
    if (y < 0 || y >= data.size() || x < 0 || x >= data[y].size()) return false;
    // 碰撞检查：如果是 '#' 则不可通行
    return data[y][x] != '#';
}

void Map::eatBean(int x, int y) {
    // 检查坐标合法性
    if (y >= 0 && y < data.size() && x >= 0 && x < data[y].size()) {
        if (data[y][x] == '.') { // 如果是豆子
            data[y][x] = ' ';   // 把它改为空格（代表吃掉了）
        }
    }
}

bool Map::hasBeans() {
    for (const auto& row : data) {
        for (char c : row) {
            if (c == '.') return true; // 还有豆子
        }
    }
    return false; // 豆子吃光了
}