#include <SFML/Graphics.hpp>
#include <vector>
#include <cmath>
#include <thread>
#include <chrono>

using namespace sf;
using namespace std;

const int MAP_W = 32;
const int MAP_H = 24;
const int gridSize = 30;
const int NUM_LEVELS = 3;

int levelMaps[NUM_LEVELS][MAP_H][MAP_W] = {
    // 关卡 1 - 原始迷宫
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,2,2,2,2,2,2,2,1,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,2,1,1,2,1,2,1,1,2,1,1,1,2,1},
        {1,3,1,0,1,2,1,1,2,1,2,1,1,2,1,0,1,3,1},
        {1,2,1,1,1,2,1,1,2,1,2,1,1,2,1,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,2,1,2,1,1,1,1,2,2,1,1,1,2,1},
        {1,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1},
        {1,1,1,1,1,2,1,2,2,2,2,2,2,1,2,1,1,1,1},
        {1,1,1,1,1,2,1,2,1,1,1,1,2,1,2,1,1,1,1},
        {1,2,2,2,2,2,2,2,1,0,0,1,2,2,2,2,2,2,1},
        {1,2,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 2 - 更复杂迷宫
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,1,2,1,2,2,2,2,1,2,2,2,2,1,2,1,2,1},
        {1,2,1,2,1,2,1,1,2,1,2,1,1,2,1,2,1,2,1},
        {1,3,2,2,2,2,1,1,2,1,2,1,1,2,2,2,2,3,1},
        {1,2,1,1,1,2,1,1,2,1,2,1,1,2,1,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,2,1,2,1,1,1,1,2,2,1,1,1,2,1},
        {1,2,2,2,2,2,1,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,1,1,1,1,2,1,1,1,1,1,1,1,1,2,1,1,1,1},
        {1,1,1,1,1,2,1,2,2,2,2,2,2,1,2,1,1,1,1},
        {1,1,1,1,1,2,1,2,1,1,1,1,2,1,2,1,1,1,1},
        {1,2,2,2,2,2,2,2,1,0,0,1,2,2,2,2,2,2,1},
        {1,2,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    },
    // 关卡 3 - 开放式地图
    {
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1},
        {1,3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,3,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1},
        {1,2,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,2,1},
        {1,2,2,2,2,2,2,2,1,0,0,1,2,2,2,2,2,2,1},
        {1,2,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,2,1},
        {1,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,1},
        {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
        {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
    }
};

int mapData[MAP_H][MAP_W];

ConvexShape createStar(float radius, const Color& fillColor) {
    ConvexShape star;
    star.setPointCount(10);
    for (int i = 0; i < 10; i++) {
        float angle = i * 2.0f * 3.14159265f / 10.0f;
        float r = (i % 2 == 0) ? radius : radius * 0.45f;
        star.setPoint(i, Vector2f(cos(angle) * r, sin(angle) * r));
    }
    star.setFillColor(fillColor);
    return star;
}

int main() {
    const int screenWidth = MAP_W * gridSize;
    const int screenHeight = MAP_H * gridSize;
    RenderWindow window(VideoMode(Vector2u(screenWidth, screenHeight)), "PacMan 3D");
    window.setFramerateLimit(60);

    Font font;
    if (!font.openFromFile("/System/Library/Fonts/STHeiti Medium.ttc")) {
        // 如果加载失败，尝试备选字体
        font.openFromFile("/System/Library/Fonts/STHeiti Light.ttc");
    }

    int currentLevel = 0;
    int totalPellets = 0;
    int originalMap[MAP_H][MAP_W];
    bool eaten[MAP_H][MAP_W] = {false};
    float eatenTime[MAP_H][MAP_W] = {0};

    const float moveSpeed = 3.5f;
    const float turnSpeed = 2.5f;
    const float FOV = 80.0f * 3.14159265f / 180.0f;
    const float maxDepth = 16.0f;

    float playerX = 9.5f;
    float playerY = 11.5f;
    float playerAngle = 0.0f;

    int score = 0;
    float gameTime = 0.0f;
    const float refreshTime = 5.0f;
    float animationTime = 0.0f;

    auto isWall = [&](int x, int y) -> bool {
        if (x < 0 || x >= MAP_W || y < 0 || y >= MAP_H) return true;
        if (mapData[y][x] != 1) return false;
        
        // 根据位置和时间计算该墙壁是否为实体
        float phase = sin(gameTime + (x * 0.3f + y * 0.7f)) * 0.5f + 0.5f; // 0 到 1
        return phase > 0.3f; // 当 phase > 0.3 时墙壁为实体，否则虚化
    };

    auto loadLevel = [&](int level) {
        totalPellets = 0;
        for (int y = 0; y < MAP_H; y++) {
            for (int x = 0; x < MAP_W; x++) {
                mapData[y][x] = levelMaps[level][y][x];
                originalMap[y][x] = levelMaps[level][y][x];
                eaten[y][x] = false;
                if (mapData[y][x] == 2 || mapData[y][x] == 3) {
                    totalPellets++;
                }
            }
        }
    };

    loadLevel(currentLevel);

    Clock clock;
    while (window.isOpen()) {
        while (auto eventOpt = window.pollEvent()) {
            if (eventOpt->is<Event::Closed>()) {
                window.close();
            }
        }

        float dt = clock.restart().asSeconds();
        gameTime += dt;
        animationTime += dt;

        for (int y = 0; y < MAP_H; y++) {
            for (int x = 0; x < MAP_W; x++) {
                if (eaten[y][x] && gameTime - eatenTime[y][x] >= refreshTime) {
                    mapData[y][x] = originalMap[y][x];
                    eaten[y][x] = false;
                }
            }
        }

        float moveDir = 0.0f;
        float turnDir = 0.0f;

        if (Keyboard::isKeyPressed(Keyboard::Key::Up) || Keyboard::isKeyPressed(Keyboard::Key::W)) moveDir = 1.0f;
        if (Keyboard::isKeyPressed(Keyboard::Key::Down) || Keyboard::isKeyPressed(Keyboard::Key::S)) moveDir = -1.0f;
        if (Keyboard::isKeyPressed(Keyboard::Key::Left) || Keyboard::isKeyPressed(Keyboard::Key::A)) turnDir = -1.0f;
        if (Keyboard::isKeyPressed(Keyboard::Key::Right) || Keyboard::isKeyPressed(Keyboard::Key::D)) turnDir = 1.0f;

        if (Keyboard::isKeyPressed(Keyboard::Key::N)) {
            currentLevel = (currentLevel + 1) % NUM_LEVELS;
            loadLevel(currentLevel);
            playerX = 9.5f;
            playerY = 11.5f;
            playerAngle = 0.0f;
            score = 0;
            gameTime = 0.0f;
        }
        if (Keyboard::isKeyPressed(Keyboard::Key::P)) {
            currentLevel = (currentLevel - 1 + NUM_LEVELS) % NUM_LEVELS;
            loadLevel(currentLevel);
            playerX = 9.5f;
            playerY = 11.5f;
            playerAngle = 0.0f;
            score = 0;
            gameTime = 0.0f;
        }

        playerAngle += turnDir * turnSpeed * dt;
        float nextX = playerX + cos(playerAngle) * moveDir * moveSpeed * dt;
        float nextY = playerY + sin(playerAngle) * moveDir * moveSpeed * dt;

        if (!isWall(int(nextX), int(playerY))) playerX = nextX;
        if (!isWall(int(playerX), int(nextY))) playerY = nextY;

        int playerCellX = int(playerX);
        int playerCellY = int(playerY);
        if (playerCellX >= 0 && playerCellX < MAP_W && playerCellY >= 0 && playerCellY < MAP_H) {
            if (!eaten[playerCellY][playerCellX] && (mapData[playerCellY][playerCellX] == 2 || mapData[playerCellY][playerCellX] == 3)) {
                eaten[playerCellY][playerCellX] = true;
                eatenTime[playerCellY][playerCellX] = gameTime;
                if (mapData[playerCellY][playerCellX] == 2) score += 10;
                else score += 50;
                mapData[playerCellY][playerCellX] = 0;
            }
        }

        int eatenCount = 0;
        for (int y = 0; y < MAP_H; y++) {
            for (int x = 0; x < MAP_W; x++) {
                if (eaten[y][x]) eatenCount++;
            }
        }
        if (eatenCount >= totalPellets && totalPellets > 0) {
            currentLevel = (currentLevel + 1) % NUM_LEVELS;
            loadLevel(currentLevel);
            playerX = 9.5f;
            playerY = 11.5f;
            playerAngle = 0.0f;
        }

        window.clear();

        RectangleShape ceiling(Vector2f(float(screenWidth), float(screenHeight / 2)));
        ceiling.setFillColor(Color(40, 90, 200));
        window.draw(ceiling);

        for (int i = 0; i < 6; i++) {
            CircleShape glow(4.0f);
            glow.setFillColor(Color(255, 255, 255, 120));
            float px = 80.0f + i * 120.0f + fmod(gameTime * 40.0f + i * 15.0f, 40.0f);
            float py = 40.0f + (i % 2) * 20.0f;
            glow.setPosition(Vector2f(px, py));
            window.draw(glow);
        }

        RectangleShape floorShape(Vector2f(float(screenWidth), float(screenHeight / 2)));
        floorShape.setFillColor(Color(20, 20, 80));
        floorShape.setPosition(Vector2f(0.0f, float(screenHeight / 2)));
        window.draw(floorShape);

        for (int i = 0; i < 6; i++) {
            CircleShape spark(3.0f);
            spark.setFillColor(Color(120, 180, 255, 110));
            float px = 60.0f + i * 140.0f - fmod(gameTime * 20.0f + i * 25.0f, 30.0f);
            float py = float(screenHeight * 0.75f) + (i % 2) * 22.0f;
            spark.setPosition(Vector2f(px, py));
            window.draw(spark);
        }

        for (int x = 0; x < screenWidth; x++) {
            float rayAngle = (playerAngle - FOV / 2.0f) + (float(x) / float(screenWidth)) * FOV;
            float rayX = cos(rayAngle);
            float rayY = sin(rayAngle);

            float distanceToWall = 0.0f;
            bool hitWall = false;

            while (!hitWall && distanceToWall < maxDepth) {
                distanceToWall += 0.05f;
                int testX = int(playerX + rayX * distanceToWall);
                int testY = int(playerY + rayY * distanceToWall);

                if (testX < 0 || testX >= MAP_W || testY < 0 || testY >= MAP_H) {
                    hitWall = true;
                    distanceToWall = maxDepth;
                } else if (isWall(testX, testY)) {
                    hitWall = true;
                }
            }

            float ceilingPos = float(screenHeight / 2) - float(screenHeight) / max(distanceToWall, 0.1f);
            float floorPos = float(screenHeight) - ceilingPos;
            Color wallColor(180, 180, 240);
            if (distanceToWall > maxDepth * 0.5f) wallColor = Color(120, 120, 190);
            if (distanceToWall > maxDepth * 0.8f) wallColor = Color(80, 80, 140);
            if ((x / 20) % 2 == 0) {
                wallColor = Color(min(wallColor.r + 20, 255), min(wallColor.g + 20, 255), min(wallColor.b + 20, 255));
            }

            // 根据时间渐变墙壁透明度
            float wallPhase = sin(gameTime * 0.5f) * 0.5f + 0.5f; // 0 到 1 之间
            int alpha = int(255 * wallPhase);
            wallColor.a = alpha;

            RectangleShape wallSlice(Vector2f(1.0f, floorPos - ceilingPos));
            wallSlice.setPosition(Vector2f(float(x), ceilingPos));
            wallSlice.setFillColor(wallColor);
            window.draw(wallSlice);
        }

        // 渲染 3D 豆子模型
        for (int y = 0; y < MAP_H; y++) {
            for (int x = 0; x < MAP_W; x++) {
                if (mapData[y][x] == 2 || mapData[y][x] == 3) {
                    // 豆子也根据时间动态出现/消失
                    float pelletPhase = sin(gameTime * 0.8f + (x * 0.2f + y * 0.5f)) * 0.5f + 0.5f;
                    if (pelletPhase < 0.2f) continue; // 豆子虚化时不显示
                    
                    float pelletX = x + 0.5f;
                    float pelletY = y + 0.5f;
                    float dx = pelletX - playerX;
                    float dy = pelletY - playerY;
                    float distance = sqrt(dx*dx + dy*dy);
                    if (distance < 0.5f) distance = 0.5f;

                    float angleToPellet = atan2(dy, dx);
                    float relativeAngle = angleToPellet - playerAngle;
                    while (relativeAngle < -3.14159265f) relativeAngle += 2.0f * 3.14159265f;
                    while (relativeAngle >  3.14159265f) relativeAngle -= 2.0f * 3.14159265f;
                    if (fabs(relativeAngle) > FOV / 2.0f) continue;

                    bool blocked = false;
                    float step = 0.05f;
                    for (float cur = 0.0f; cur < distance; cur += step) {
                        int checkX = int(playerX + cos(angleToPellet) * cur);
                        int checkY = int(playerY + sin(angleToPellet) * cur);
                        if (isWall(checkX, checkY)) {
                            blocked = true;
                            break;
                        }
                    }
                    if (blocked) continue;

                    float screenX = (0.5f * (relativeAngle / (FOV / 2.0f)) + 0.5f) * float(screenWidth);
                    float size = (mapData[y][x] == 3 ? 16.0f : 10.0f) * (1.0f / distance) * 4.0f;
                    if (size < 8.0f) size = 8.0f;
                    float screenY = float(screenHeight / 2) + (float(screenHeight / 2) - size) * 0.35f;

                    static const Color starColors[] = {
                        Color(255, 120, 180), Color(120, 220, 255), Color(255, 220, 120),
                        Color(180, 255, 130), Color(160, 140, 255), Color(255, 140, 220)
                    };
                    Color starColor = starColors[(x + y) % 6];
                    if (mapData[y][x] == 3) starColor = Color(255, 200, 80);

                    static const std::string chars[3] = {"李", "恩", "泽"};
                    int charIndex = (x + y) % 3;
                    sf::Text pelletText(font, chars[charIndex], static_cast<unsigned int>(size * 0.4f));
                    pelletText.setPosition(Vector2f(screenX - size * 0.2f, screenY - size * 0.2f));
                    pelletText.setRotation(degrees(animationTime * 120.0f + (x + y) * 20.0f));

                    // 闪烁效果
                    float blink = sin(animationTime * 5.0f) * 0.5f + 0.5f;
                    int alpha = static_cast<int>(255 * (0.3f + blink * 0.7f));
                    pelletText.setFillColor(Color(starColor.r, starColor.g, starColor.b, alpha));
                    pelletText.setOutlineColor(Color(255, 255, 255, alpha));
                    pelletText.setOutlineThickness(2.0f);
                    window.draw(pelletText);
                }
            }
        }

        const float miniScale = 12.0f;
        RectangleShape miniBg(Vector2f(MAP_W * miniScale + 6.0f, MAP_H * miniScale + 6.0f));
        miniBg.setFillColor(Color(0, 0, 0, 180));
        miniBg.setOutlineColor(Color(255, 255, 255, 160));
        miniBg.setOutlineThickness(2);
        miniBg.setPosition(Vector2f(10.0f, 10.0f));
        window.draw(miniBg);

        for (int y = 0; y < MAP_H; y++) {
            for (int x = 0; x < MAP_W; x++) {
                RectangleShape tile(Vector2f(miniScale - 2.0f, miniScale - 2.0f));
                tile.setPosition(Vector2f(12.0f + x * miniScale, 12.0f + y * miniScale));
                
                if (levelMaps[currentLevel][y][x] == 1) {
                    // 计算该位置墙体的当前状态
                    float phase = sin(gameTime + (x * 0.3f + y * 0.7f)) * 0.5f + 0.5f;
                    if (phase > 0.3f) {
                        // 墙体为实体，绘制
                        tile.setFillColor(Color(80, 80, 180));
                        window.draw(tile);
                    } else {
                        // 墙体虚化，半透明显示
                        tile.setFillColor(Color(80, 80, 180, 80));
                        window.draw(tile);
                    }
                } else {
                    tile.setFillColor(Color(20, 20, 60));
                    window.draw(tile);
                }

                if ((levelMaps[currentLevel][y][x] == 2 || levelMaps[currentLevel][y][x] == 3) && !eaten[y][x]) {
                    // 豆子根据时间动态显示
                    float pelletPhase = sin(gameTime * 0.8f + (x * 0.2f + y * 0.5f)) * 0.5f + 0.5f;
                    if (pelletPhase >= 0.2f) {
                        CircleShape dot(levelMaps[currentLevel][y][x] == 3 ? 4.0f : 2.5f);
                        // 根据相位调整透明度
                        int alpha = int(255 * pelletPhase);
                        Color dotColor = levelMaps[currentLevel][y][x] == 3 ? Color(255, 180, 0, alpha) : Color(255, 255, 150, alpha);
                        dot.setFillColor(dotColor);
                        dot.setPosition(Vector2f(12.0f + x * miniScale + miniScale / 2.0f - dot.getRadius(), 12.0f + y * miniScale + miniScale / 2.0f - dot.getRadius()));
                        window.draw(dot);
                    }
                }
            }
        }

        CircleShape playerDot(5);
        playerDot.setFillColor(Color(255, 255, 0));
        playerDot.setPosition(Vector2f(12.0f + playerX * miniScale - 5.0f, 12.0f + playerY * miniScale - 5.0f));
        window.draw(playerDot);

        RectangleShape uiBg(Vector2f(280, 120));
        uiBg.setFillColor(Color(0, 0, 0, 170));
        uiBg.setPosition(Vector2f(10.0f, float(screenHeight - 140)));
        uiBg.setOutlineColor(Color(255, 255, 255, 200));
        uiBg.setOutlineThickness(2);
        window.draw(uiBg);

        Text scoreText(font, "⭐ Score: " + to_string(score), 22);
        scoreText.setFillColor(Color(255, 230, 120));
        scoreText.setPosition(Vector2f(20.0f, float(screenHeight - 130)));
        window.draw(scoreText);

        Text timeText(font, "⏰ Time: " + to_string(int(gameTime)) + "s", 22);
        timeText.setFillColor(Color(255, 150, 255));
        timeText.setPosition(Vector2f(20.0f, float(screenHeight - 95)));
        window.draw(timeText);

        Text levelText(font, "🏁 Level " + to_string(currentLevel + 1), 22);
        levelText.setFillColor(Color(150, 255, 150));
        levelText.setPosition(Vector2f(20.0f, float(screenHeight - 60)));
        window.draw(levelText);

        Text helpText(font, "W/S move  A/D turn  N/P level", 18);
        helpText.setFillColor(Color(200, 200, 255));
        helpText.setPosition(Vector2f(20.0f, float(screenHeight - 30)));
        window.draw(helpText);

        window.display();
    }

    return 0;
}
