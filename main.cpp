#include "Map.h"
#include <iostream>

int main() {
    Map gameMap;
    int difficulty;
    std::cout << "Select Difficulty (1: Easy, 2: Hard): ";
    std::cin >> difficulty;

    if (gameMap.loadFromFile("map.txt")) {
        gameMap.display();
        std::cout << "\nGame started at difficulty " << difficulty << "!\n";
    } else {
        std::cerr << "Error: map.txt missing!\n";
    }
    return 0;
}