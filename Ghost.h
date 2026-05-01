#ifndef GHOST_H
#define GHOST_H

#include "Map.h"
#include <vector>

enum GhostBehavior { RANDOM, CHASE };

class Ghost {
public:
    Ghost(int startX, int startY, GhostBehavior behavior, double moveInterval);
    void update(double currentTime, const Map& m, int playerX, int playerY);
    int getX() const;
    int getY() const;
    void setBehavior(GhostBehavior b);
    double getMoveInterval() const;
private:
    int x, y;
    GhostBehavior behavior;
    double moveInterval;
    double nextMoveTime;
    void moveRandomly(const Map& m);
    void moveTowardsPlayer(const Map& m, int playerX, int playerY);
    std::pair<int,int> bfsNextStep(const Map& m, int targetX, int targetY) const;
};

#endif
