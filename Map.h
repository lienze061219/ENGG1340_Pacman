#ifndef MAP_H
#define MAP_H

#include <vector>
#include <string>

class Map {
public:
    // 从文件加载地图，返回是否成功
    bool loadFromFile(const std::string& filename);

    // 将地图输出到控制台，px,py 为玩家坐标，gx,gy 为幽灵坐标
    void display(int px, int py, int gx, int gy) const;

    // 判断坐标 (x,y) 是否安全（非墙）
    bool isSafe(int x, int y) const;

    // 吃掉坐标 (x,y) 的豆子（如果存在）
    void eatBean(int x, int y);

    // 返回地图宽度（最长行长度）
    int getWidth() const;

    // 返回地图高度（行数）
    int getHeight() const;

    // 统计并返回当前剩余豆子数量
    int countBeans() const;

    // 检查是否还有豆子
    bool hasBeans() const;

    // 可选：把当前地图保存回文件（用于调试或持久化）
    bool saveToFile(const std::string& filename) const;

private:
    // data[y][x] 表示第 y 行第 x 列字符
    std::vector<std::vector<char>> data;

    // 内部安全访问：如果坐标越界返回 false
    bool inBounds(int x, int y) const;
};

#endif // MAP_H
