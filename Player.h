#ifndef PLAYER_H
#define PLAYER_H

#include "Map.h"

class Player {
public:
    Player(int startX, int startY);
    void move(char input, Map &m);
    int getX() const;
    int getY() const;

private:
    int x;
    int y;
};

#endif