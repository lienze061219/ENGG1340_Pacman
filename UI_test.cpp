#include "UI.h"
#include <iostream>
#include <string>

using namespace std;

int main() {
    UI ui;
    
    // 测试主菜单
    ui.drawMenu();
    cout << "Press Enter to continue testing..." << endl;
    cin.get();
    
    // 测试游戏HUD - 现在使用统一的4参数版本
    ui.clearScreen();
    cout << "Map display area..." << endl << endl;
    ui.drawHUD(1250, 2);        // 得分1250，2条命，默认关卡1，默认最大HP=3
    ui.drawHUD(3500, 1, 5);     // 得分3500，1条命，关卡5，默认最大HP=3
    ui.drawHUD(5000, 3, 2, 5);  // 得分5000，3条命，关卡2，最大HP=5
    cout << endl << "Press Enter to continue..." << endl;
    cin.get();
    
    // 测试关卡介绍
    ui.showLevelIntro(2);  // 自动倒计时
    
    // 测试故事
    ui.showStory(1);
    
    // 测试游戏结束
    ui.showGameOver(5000, true);  // 胜利
    ui.clearScreen();
    ui.showGameOver(100, false);  // 失败
    
    // 测试暂停菜单
    ui.showPauseMenu();
    cout << "Press Enter to continue..." << endl;
    cin.get();
    
    // 测试说明界面
    ui.showInstructions();
    
    // 测试特效
    ui.clearScreen();
    ui.typewriterEffect("This is typewriter effect...\n", 50);
    ui.drawLoadingBar("Loading", 75, 100);
    
    cout << "\n\nAll UI tests complete!" << endl;
    
    return 0;
}
