#ifndef DEBUGSCREEN_H
#define DEBUGSCREEN_H

#include "../Screen.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include <vector>

class DebugScreen : public Screen
{
public:
    DebugScreen(Minecraft* mc);
    virtual ~DebugScreen();

    virtual void init() override;
    virtual void setupPositions() override;
    virtual void render(int xm, int ym, float a) override;

    virtual bool renderGameBehind() override { return true; }
    virtual bool isInGameScreen() override { return false; }
    virtual bool isPauseScreen() override { return false; }

    virtual void tick() override {}

protected:
    virtual void buttonClicked(Button* button) override;

private:
    void addButton(int id, const std::string& text);
    void executeAction(int id);

    // 完全抛弃自定义鼠标事件，使用基类的默认处理（避免任何坐标转换问题）
    // 基类的 Screen::mouseClicked / mouseReleased 会直接使用按钮的逻辑坐标，
    // 前提是按钮的 x, y, width, height 已经是正确的逻辑坐标。
    
    Minecraft* mc;
    std::vector<Button*> debugButtons;

    static const int BTN_GODMODE      = 0;
    static const int BTN_GAMEMODE     = 1;
    static const int BTN_TIME         = 2;
    static const int BTN_ARMOR        = 3;
    static const int BTN_HURT_RELOAD  = 4;
    static const int BTN_SPAWNMOB     = 5;
    static const int BTN_MASSACRE     = 6;
    static const int BTN_CLEARINV     = 7;
    static const int BTN_PRERENDER    = 8;
    static const int BTN_DROPALL      = 9;
    static const int BTN_SPEEDUP      = 10;
    static const int BTN_3RDPERSON    = 11;
    static const int BTN_NOPVP        = 12;
    static const int BTN_NOPVM        = 13;
    static const int BTN_NOMVP        = 14;
    static const int BTN_IMMUTABLE    = 15;
    static const int BTN_NAMETAGS     = 16;
    static const int BTN_PARTICLES    = 17;
};

#endif
