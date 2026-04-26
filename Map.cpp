#include "Map.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// 辅助：检查坐标是否在地图范围内
bool Map::inBounds(int x, int y) const {
    if (y < 0 || y >= static_cast<int>(data.size())) return false;
    if (x < 0 || x >= static_cast<int>(data[y].size())) return false;
    return true;
}

bool Map::loadFromFile(const std::string& filename) {
    std::ifstream in(filename);
    if (!in.is_open()) return false;

    data.clear();
    std::string line;
    while (std::getline(in, line)) {
        std::vector<char> row;
        row.reserve(line.size());
        for (char c : line) row.push_back(c);
        data.push_back(std::move(row));
    }
    in.close();

    // 如果某些行长度不一致，可以选择填充空格使矩阵更规整
    size_t maxlen = 0;
    for (const auto& row : data) if (row.size() > maxlen) maxlen = row.size();
    for (auto& row : data) {
        if (row.size() < maxlen) row.resize(maxlen, ' ');
    }

    return !data.empty();
}

void Map::display(int px, int py, int gx, int gy) const {
    for (int y = 0; y < static_cast<int>(data.size()); ++y) {
        for (int x = 0; x < static_cast<int>(data[y].size()); ++x) {
            if (x == px && y == py && x == gx && y == gy) {
                std::cout << 'X'; // 玩家与幽灵同格，表示碰撞
            } else if (x == px && y == py) {
                std::cout << 'P';
            } else if (x == gx && y == gy) {
                std::cout << 'G';
            } else {
                std::cout << data[y][x];
            }
        }
        std::cout << '\n';
    }
}

bool Map::isSafe(int x, int y) const {
    if (!inBounds(x, y)) return false;
    // 约定 '#' 为墙，其他字符均可通行
    return data[y][x] != '#';
}

void Map::eatBean(int x, int y) {
    if (!inBounds(x, y)) return;
    // 约定 '.' 为普通豆子，'o' 可作为能量豆，下面示例同时处理两种
    if (data[y][x] == '.' || data[y][x] == 'o') {
        data[y][x] = ' '; // 吃掉后变为空格
    }
}

int Map::getWidth() const {
    if (data.empty()) return 0;
    return static_cast<int>(data[0].size());
}

int Map::getHeight() const {
    return static_cast<int>(data.size());
}

int Map::countBeans() const {
    int cnt = 0;
    for (const auto& row : data) {
        for (char c : row) {
            if (c == '.' || c == 'o') ++cnt;
        }
    }
    return cnt;
}

bool Map::hasBeans() const {
    return countBeans() > 0;
}

bool Map::saveToFile(const std::string& filename) const {
    std::ofstream out(filename);
    if (!out.is_open()) return false;
    for (const auto& row : data) {
        for (char c : row) out << c;
        out << '\n';
    }
    out.close();
    return true;
}
