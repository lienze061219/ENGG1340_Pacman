#ifndef MAP_H
#define MAP_H
#include <vector>
#include <string>

class Map {
public:
    bool loadFromFile(std::string filename);
    void display();
private:
    std::vector<std::vector<char>> data;
};
#endif