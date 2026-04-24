#ifndef MAP_H
#define MAP_H
#include <vector>
#include <string>

class Map {
public:
    bool loadFromFile(std::string filename);
    void display(int px = -1, int py = -1, int gx = -1, int gy = -1);
    bool isSafe(int x, int y);  // 判断 (x,y) 是不是墙
    void eatBean(int x, int y); // 尝试吃掉 (x,y) 处的豆子
    bool hasBeans();            // 检查是否还有豆子
private:
    std::vector<std::vector<char>> data;
};
#endif