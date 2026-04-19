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

void Map::display() {
    std::cout << "\033[2J\033[1;1H"; // 清屏指令
    for (const auto& row : data) {
        for (char c : row) std::cout << c << " ";
        std::cout << "\n";
    }
}