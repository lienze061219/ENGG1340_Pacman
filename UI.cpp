#include "UI.h"
#include <iostream>
#include <string>
#include <chrono>
#include <thread>

using namespace std;

// ==================== 系统工具函数 ====================

void UI::clearScreen() {
    #ifdef _WIN32
        system("cls");
    #else
        system("clear");
    #endif
}

void UI::gotoxy(int x, int y) {
    #ifdef _WIN32
        COORD coord;
        coord.X = x;
        coord.Y = y;
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), coord);
    #else
        printf("\033[%d;%dH", y, x);
    #endif
}

void UI::sleep(int milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// ==================== 颜色控制 ====================

void UI::setColor(Color color) {
    setColor(static_cast<int>(color));
}

void UI::setColor(int colorCode) {
    #ifdef _WIN32
        HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
        SetConsoleTextAttribute(hConsole, colorCode);
    #else
        // ANSI颜色代码
        string colorStr;
        switch(colorCode) {
            case 1: colorStr = "\033[31m"; break;      // 红色
            case 2: colorStr = "\033[32m"; break;      // 绿色
            case 3: colorStr = "\033[33m"; break;      // 黄色
            case 4: colorStr = "\033[34m"; break;      // 蓝色
            case 5: colorStr = "\033[35m"; break;      // 洋红
            case 6: colorStr = "\033[36m"; break;      // 青色
            case 7: colorStr = "\033[37m"; break;      // 白色
            case 8: colorStr = "\033[91m"; break;      // 亮红
            case 9: colorStr = "\033[92m"; break;      // 亮绿
            case 10: colorStr = "\033[93m"; break;     // 亮黄
            case 11: colorStr = "\033[94m"; break;     // 亮蓝
            case 12: colorStr = "\033[95m"; break;     // 亮洋红
            case 13: colorStr = "\033[96m"; break;     // 亮青
            case 14: colorStr = "\033[97m"; break;     // 亮白
            default: colorStr = "\033[0m"; break;
        }
        cout << colorStr;
    #endif
}

void UI::resetColor() {
    #ifdef _WIN32
        setColor(7); // 默认白色
    #else
        cout << "\033[0m";
    #endif
}

// ==================== 辅助绘制函数 ====================

void UI::drawLine(int length, std::string ch) {
    for (int i = 0; i < length; i++) {
        cout << ch;
    }
    cout << endl;
}

void UI::drawCenteredText(const string& text, int width) {
    int padding = (width - text.length()) / 2;
    if (padding < 0) padding = 0;
    for (int i = 0; i < padding; i++) {
        cout << " ";
    }
    cout << text << endl;
}

void UI::drawBox(int x, int y, int width, int height, const string& title) {
    gotoxy(x, y);
    
    // 顶边
    cout << "+";
    for (int i = 0; i < width - 2; i++) {
        cout << "-";
    }
    cout << "+";
    
    // 标题
    if (!title.empty()) {
        gotoxy(x + (width - title.length() - 2) / 2, y);
        cout << " " << title << " ";
    }
    
    // 侧边
    for (int i = 1; i < height - 1; i++) {
        gotoxy(x, y + i);
        cout << "|";
        for (int j = 0; j < width - 2; j++) {
            cout << " ";
        }
        cout << "|";
    }
    
    // 底边
    gotoxy(x, y + height - 1);
    cout << "+";
    for (int i = 0; i < width - 2; i++) {
        cout << "-";
    }
    cout << "+";
}

// ==================== 主菜单 ====================

void UI::drawGameTitle() {
    setColor(Color::BRIGHT_YELLOW);
    cout << R"(
    +======================================================+
    |                                                      |
    |     PAC-MAN  CONSOLE  EDITION                        |
    |                                                      |
    |      ____                                             |
    |     |  _ \ __ _  ___ _ __ ___   __ _ _ __            |
    |     | |_) / _` |/ __| '_ ` _ \ / _` | '_ \           |
    |     |  __/ (_| | (__| | | | | | (_| | | | |          |
    |     |_|   \__,_|\___|_| |_| |_|\__,_|_| |_|          |
    |                                                      |
    +======================================================+
    )" << endl;
    resetColor();
}

void UI::drawMenu() {
    clearScreen();
    
    setColor(Color::BRIGHT_CYAN);
    drawLine(60, "=");
    drawCenteredText("PAC-MAN CONSOLE EDITION", 60);
    drawLine(60, "=");
    cout << endl;
    
    drawGameTitle();
    
    cout << endl;
    
    setColor(Color::BRIGHT_GREEN);
    drawCenteredText("+-------------------------------------+", 60);
    drawCenteredText("|                                     |", 60);
    drawCenteredText("|        1. Start Game                |", 60);
    drawCenteredText("|        2. How to Play               |", 60);
    drawCenteredText("|        3. Select Level              |", 60);
    drawCenteredText("|        4. Exit                      |", 60);
    drawCenteredText("|                                     |", 60);
    drawCenteredText("+-------------------------------------+", 60);
    
    cout << endl;
    setColor(Color::BRIGHT_WHITE);
    drawCenteredText("Press 1-4 to select...", 60);
    cout << endl;
    
    setColor(Color::BRIGHT_YELLOW);
    drawCenteredText("* TIP: Use WASD to move *", 60);
    drawCenteredText("* Press P to pause *", 60);
    
    setColor(Color::BRIGHT_CYAN);
    cout << endl;
    drawLine(60, "=");
    drawCenteredText("v1.0 - C++ Course Project", 60);
    drawLine(60, "=");
    
    resetColor();
    cout << endl;
}

// ==================== 游戏HUD ====================

void UI::drawHUD(int score, int hp, int level, int maxHP) {
    setColor(Color::BRIGHT_CYAN);
    drawLine(40, "=");
    
    setColor(Color::BRIGHT_YELLOW);
    cout << "  ";
    
    // 分数
    setColor(Color::BRIGHT_WHITE);
    cout << "* Score: ";
    setColor(Color::BRIGHT_GREEN);
    cout << score;
    
    cout << "  |  ";
    
    // 关卡
    setColor(Color::BRIGHT_WHITE);
    cout << "Level: ";
    setColor(Color::BRIGHT_MAGENTA);
    cout << level;
    
    cout << "  |  ";
    
    // HP显示
    setColor(Color::BRIGHT_WHITE);
    cout << "HP: ";
    setColor(Color::BRIGHT_RED);
    for (int i = 0; i < hp; i++) {
        cout << "H ";
    }
    setColor(Color::DEFAULT);
    for (int i = hp; i < maxHP; i++) {
        cout << "O ";
    }
    
    cout << endl;
    
    setColor(Color::BRIGHT_CYAN);
    drawLine(40, "=");
    resetColor();
}

// ==================== 关卡介绍 ====================

void UI::showLevelIntro(int level) {
    clearScreen();
    
    cout << endl << endl;
    setColor(Color::BRIGHT_YELLOW);
    drawCenteredText("+===================================+", 50);
    drawCenteredText("|                                   |", 50);
    
    string levelText = "|         LEVEL " + to_string(level) + " - Prepare!        |";
    drawCenteredText(levelText, 50);
    
    drawCenteredText("|                                   |", 50);
    drawCenteredText("+===================================+", 50);
    
    cout << endl;
    resetColor();
    
    showCountdown(3);
}

void UI::showCountdown(int seconds) {
    for (int i = seconds; i > 0; i--) {
        clearScreen();
        cout << endl << endl << endl;
        
        setColor(Color::BRIGHT_CYAN);
        drawCenteredText("Get Ready...", 40);
        cout << endl;
        
        setColor(Color::BRIGHT_YELLOW);
        string countText = "       " + to_string(i) + "       ";
        drawCenteredText(countText, 40);
        
        cout << endl;
        setColor(Color::BRIGHT_WHITE);
        
        // 简单的进度条动画
        cout << "        ";
        setColor(Color::BRIGHT_GREEN);
        int barWidth = 20;
        int progress = (seconds - i + 1) * barWidth / seconds;
        for (int j = 0; j < progress; j++) {
            cout << "#";
        }
        setColor(Color::DEFAULT);
        for (int j = progress; j < barWidth; j++) {
            cout << "-";
        }
        cout << endl;
        
        sleep(800);
    }
    
    clearScreen();
    setColor(Color::BRIGHT_GREEN);
    drawCenteredText("GO! GO! GO!", 40);
    resetColor();
    sleep(500);
    clearScreen();
}

// ==================== 故事过场 ====================

void UI::showStory(int level) {
    clearScreen();
    
    cout << endl;
    setColor(Color::BRIGHT_CYAN);
    drawLine(50, "~");
    drawCenteredText("Story Scene", 50);
    drawLine(50, "~");
    cout << endl;
    
    vector<string> story;
    
    switch(level) {
        case 1:
            story = {
                "In a dark maze...",
                "Little Pac-Man awakens.",
                "He is hungry, very very hungry.",
                "The maze is full of tasty beans,",
                "But ghosts lurk in the shadows...",
                "",
                "\"I must eat all the beans!\"",
                "Pac-Man gathers his courage,",
                "And begins his adventure."
            };
            break;
        case 2:
            story = {
                "Pac-Man discovers a secret!",
                "This was once a ghost kingdom,",
                "Cursed by an evil wizard.",
                "",
                "Eating power pellets makes",
                "the ghosts temporarily vulnerable.",
                "",
                "\"I will free these poor souls!\"",
                "The adventure continues..."
            };
            break;
        case 3:
            story = {
                "This is the final battle!",
                "The Ghost King himself appears,",
                "Stronger and faster than any ghost.",
                "",
                "\"I will never give up!\"",
                "Pac-Man clenches his fists.",
                "",
                "For freedom, for food,",
                "The ultimate challenge begins!"
            };
            break;
        default:
            story = {
                "A new challenge appears!",
                "The maze gets more complex,",
                "The ghosts get smarter.",
                "",
                "\"No matter what, I'll keep going!\"",
                "Pac-Man never gives up!"
            };
            break;
    }
    
    for (const auto& line : story) {
        if (line.empty()) {
            cout << endl;
        } else if (!line.empty() && line[0] == '\"') {
            setColor(Color::BRIGHT_YELLOW);
            drawCenteredText(line, 50);
        } else {
            setColor(Color::BRIGHT_WHITE);
            drawCenteredText(line, 50);
        }
        sleep(500);
    }
    
    cout << endl << endl;
    setColor(Color::BRIGHT_GREEN);
    drawCenteredText("Press Enter to continue...", 50);
    resetColor();
    
    cin.ignore();
    cin.get();
}

// ==================== 游戏结束 ====================

void UI::showGameOver(int finalScore, bool won) {
    clearScreen();
    
    cout << endl << endl;
    
    if (won) {
        // 胜利画面
        setColor(Color::BRIGHT_YELLOW);
        cout << R"(
        +====================================+
        |                                    |
        |   CONGRATULATIONS! YOU WIN!        |
        |                                    |
        +====================================+
        )" << endl;
    } else {
        // 失败画面
        setColor(Color::BRIGHT_RED);
        cout << R"(
        +====================================+
        |                                    |
        |   G A M E   O V E R                |
        |                                    |
        +====================================+
        )" << endl;
    }
    
    cout << endl;
    setColor(Color::BRIGHT_CYAN);
    drawCenteredText("=========================", 45);
    
    cout << endl;
    setColor(Color::BRIGHT_WHITE);
    cout << "            Final Score: ";
    setColor(Color::BRIGHT_GREEN);
    cout << finalScore;
    
    cout << endl << endl;
    setColor(Color::BRIGHT_WHITE);
    drawCenteredText("Press R to restart", 45);
    drawCenteredText("Press M for main menu", 45);
    drawCenteredText("Press Q to quit", 45);
    
    setColor(Color::BRIGHT_CYAN);
    drawCenteredText("=========================", 45);
    resetColor();
    
    cout << endl;
}

// ==================== 游戏说明 ====================

void UI::showInstructions() {
    clearScreen();
    
    setColor(Color::BRIGHT_CYAN);
    drawLine(60, "=");
    drawCenteredText("HOW TO PLAY", 60);
    drawLine(60, "=");
    cout << endl;
    
    setColor(Color::BRIGHT_YELLOW);
    drawCenteredText("+-- Controls --------------------------+", 55);
    setColor(Color::BRIGHT_WHITE);
    cout << "  |  W / Arrow Up    : Move Up             |" << endl;
    cout << "  |  A / Arrow Left  : Move Left           |" << endl;
    cout << "  |  S / Arrow Down  : Move Down           |" << endl;
    cout << "  |  D / Arrow Right : Move Right          |" << endl;
    cout << "  |  P               : Pause Game          |" << endl;
    cout << "  |  Q             : Quit Current Game   |" << endl;
    setColor(Color::BRIGHT_YELLOW);
    cout << "  +----------------------------------------+" << endl;
    
    cout << endl;
    
    setColor(Color::BRIGHT_GREEN);
    drawCenteredText("+-- Objective --------------------------+", 55);
    setColor(Color::BRIGHT_WHITE);
    cout << "  |  * Eat all beans (. and O) in the maze |" << endl;
    cout << "  |  * Avoid ghosts (G) chasing you        |" << endl;
    cout << "  |  * Power pellets (O) let you eat ghosts|" << endl;
    setColor(Color::BRIGHT_GREEN);
    cout << "  +----------------------------------------+" << endl;
    
    cout << endl;
    
    setColor(Color::BRIGHT_MAGENTA);
    drawCenteredText("+-- Elements ---------------------------+", 55);
    setColor(Color::BRIGHT_WHITE);
    cout << "  |  P : Player (Pac-Man)                  |" << endl;
    cout << "  |  G : Ghost (Enemy)                     |" << endl;
    cout << "  |  . : Small Bean (+10 points)           |" << endl;
    cout << "  |  O : Power Pellet (+50 points)         |" << endl;
    cout << "  |  # : Wall (Cannot pass)                |" << endl;
    setColor(Color::BRIGHT_MAGENTA);
    cout << "  +----------------------------------------+" << endl;
    
    cout << endl;
    
    setColor(Color::BRIGHT_RED);
    drawCenteredText("+-- Life System ------------------------+", 55);
    setColor(Color::BRIGHT_WHITE);
    cout << "  |  HHH : 3 lives                         |" << endl;
    cout << "  |  Touch a ghost = lose 1 life           |" << endl;
    cout << "  |  Lose all lives = Game Over            |" << endl;
    setColor(Color::BRIGHT_RED);
    cout << "  +----------------------------------------+" << endl;
    
    cout << endl;
    setColor(Color::BRIGHT_CYAN);
    drawLine(60, "=");
    setColor(Color::BRIGHT_GREEN);
    drawCenteredText("Press Enter to return to menu...", 60);
    resetColor();
    
    cin.ignore();
    cin.get();
}

// ==================== 暂停菜单 ====================

void UI::showPauseMenu() {
    clearScreen();
    
    cout << endl << endl;
    setColor(Color::BRIGHT_YELLOW);
    drawCenteredText("+============================================+", 40);
    drawCenteredText("|                                            |", 40);
    drawCenteredText("|          G A M E   P A U S E D             |", 40);
    drawCenteredText("|                                            |", 40);
    drawCenteredText("+============================================+", 40);
    
    cout << endl;
    setColor(Color::BRIGHT_WHITE);
    drawCenteredText("Press P to continue", 40);
    drawCenteredText("Press Q to quit to menu", 40);
    resetColor();
}

// ==================== 特效函数 ====================

void UI::typewriterEffect(const string& text, int delayMs) {
    for (char c : text) {
        cout << c;
        cout.flush();
        sleep(delayMs);
    }
}

void UI::blinkingText(const string& text, int x, int y, int blinks, int delayMs) {
    for (int i = 0; i < blinks; i++) {
        gotoxy(x, y);
        setColor(Color::BRIGHT_YELLOW);
        cout << text;
        sleep(delayMs);
        
        gotoxy(x, y);
        // 清除文本
        for (size_t j = 0; j < text.length(); j++) {
            cout << " ";
        }
        sleep(delayMs);
    }
    
    // 最后显示
    gotoxy(x, y);
    setColor(Color::BRIGHT_GREEN);
    cout << text;
    resetColor();
}

void UI::drawLoadingBar(const string& message, int progress, int total) {
    cout << "\r" << message << " [";
    int barWidth = 30;
    int filled = (progress * barWidth) / total;
    
    setColor(Color::BRIGHT_GREEN);
    for (int i = 0; i < filled; i++) {
        cout << "=";
    }
    cout << ">";
    setColor(Color::DEFAULT);
    for (int i = filled + 1; i < barWidth; i++) {
        cout << " ";
    }
    
    cout << "] " << (progress * 100 / total) << "%";
    cout.flush();
}
