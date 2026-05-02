#ifndef GHOST_H
#define GHOST_H

#include "Map.h"

class Ghost {
public:
    Ghost(int startX, int startY);

    void update(const Map &m);

    int getX() const;
    int getY() const;

private:
    int x, y;

    void moveRandomly(const Map &m);
};

#endif
