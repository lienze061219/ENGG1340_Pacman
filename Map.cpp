#include "Map.h"
#include <fstream>
#include <iostream>
#include <algorithm>

// 辅助：检查坐标是否在地图范围内
bool Map::inBounds(int x, int y) const {
    if (y < 0 || y >= height) return false;
    if (x < 0 || x >= width) return false;
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

    size_t maxlen = 0;
    for (const auto& row : data) if (row.size() > maxlen) maxlen = row.size();
    for (auto& row : data) {
        if (row.size() < maxlen) row.resize(maxlen, ' ');
    }

    height = static_cast<int>(data.size());
    width = static_cast<int>(maxlen);
    return !data.empty();
}

bool Map::loadFromGrid(std::vector<std::vector<char>> grid) {
    data = std::move(grid);
    if (data.empty()) {
        width = 0;
        height = 0;
        return false;
    }
    size_t maxlen = 0;
    for (const auto& row : data) if (row.size() > maxlen) maxlen = row.size();
    for (auto& row : data) {
        if (row.size() < maxlen) row.resize(maxlen, ' ');
    }
    height = static_cast<int>(data.size());
    width = static_cast<int>(maxlen);
    return true;
}

void Map::display(int px, int py, int gx, int gy) const {
    std::string row;
    row.reserve(width > 0 ? width : 16);
    for (int y = 0; y < height; ++y) {
        row.clear();
        for (int x = 0; x < width; ++x) {
            if (x == px && y == py && x == gx && y == gy) {
                row.push_back('X');
            } else if (x == px && y == py) {
                row.push_back('P');
            } else if (x == gx && y == gy) {
                row.push_back('G');
            } else {
                row.push_back(data[y][x]);
            }
        }
        std::cout << row << '\n';
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
    return width;
}

int Map::getHeight() const {
    return height;
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

char Map::getTile(int x, int y) const {
    if (!inBounds(x, y)) return '#';
    return data[y][x];
}

void Map::setTile(int x, int y, char tile) {
    if (!inBounds(x, y)) return;
    data[y][x] = tile;
}
