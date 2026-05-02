#include "Player.h"

Player::Player(int startX, int startY) {
    x = startX;
    y = startY;
}

void Player::move(char input, Map &m) {
    int nextX = x;
    int nextY = y;

    if (input == 'w') {
        nextY = y - 1;
    } else if (input == 's') {
        nextY = y + 1;
    } else if (input == 'a') {
        nextX = x - 1;
    } else if (input == 'd') {
        nextX = x + 1;
    }

    if (m.isSafe(nextX, nextY)) {
        x = nextX;
        y = nextY;
    }
}

int Player::getX() const {
    return x;
}

int Player::getY() const {
    return y;
} 