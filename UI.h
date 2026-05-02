#ifndef UI_H
#define UI_H

#include <string>
#include <vector>
#include <iostream>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <unistd.h>
#endif

class UI {
public:
    // 清屏函数
    static void clearScreen();
    
    // 设置控制台颜色
    enum class Color {
        DEFAULT = 0,
        RED = 1,
        GREEN = 2,
        YELLOW = 3,
        BLUE = 4,
        MAGENTA = 5,
        CYAN = 6,
        WHITE = 7,
        BRIGHT_RED = 8,
        BRIGHT_GREEN = 9,
        BRIGHT_YELLOW = 10,
        BRIGHT_BLUE = 11,
        BRIGHT_MAGENTA = 12,
        BRIGHT_CYAN = 13,
        BRIGHT_WHITE = 14
    };
    
    static void setColor(Color color);
    static void setColor(int colorCode);
    static void resetColor();
    
    // 主要UI函数 - 合并为一个函数，消除歧义
    void drawHUD(int score, int hp, int level = 1, int maxHP = 3);
    
    void drawMenu();
    void showLevelIntro(int level);
    void showStory(int level);
    void showGameOver(int finalScore, bool won = false);
    void showPauseMenu();
    void drawGameTitle();
    void showInstructions();
    void drawLoadingBar(const std::string& message, int progress, int total);
    void showCountdown(int seconds);
    
    // 辅助函数
    void drawLine(int length, std::string ch = "=");
    void drawCenteredText(const std::string& text, int width = 60);
    void drawBox(int x, int y, int width, int height, const std::string& title = "");
    
    // 动画效果
    void typewriterEffect(const std::string& text, int delayMs = 30);
    void blinkingText(const std::string& text, int x, int y, int blinks = 3, int delayMs = 300);
    
private:
    int terminalWidth = 80;
    int terminalHeight = 24;
    
    void gotoxy(int x, int y);
    void sleep(int milliseconds);
};

#endif // UI_H
