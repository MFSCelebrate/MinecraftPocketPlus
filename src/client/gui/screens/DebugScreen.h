#ifndef DEBUGSCREEN_H
#define DEBUGSCREEN_H

#include "../Screen.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include <vector>

class DebugScreen : public Screen {
public:
    DebugScreen(Minecraft* mc);
    virtual ~DebugScreen();
    virtual void init();
    virtual void setupPositions();
    virtual void render(int xm, int ym, float a);
    virtual bool renderGameBehind() { return true; }
    virtual bool isInGameScreen() { return false; }
    virtual bool isPauseScreen() { return false; }

    virtual void tick() override;
    virtual void mouseClicked(int x, int y, int buttonNum) override;
    virtual void mouseReleased(int x, int y, int buttonNum) override;
    virtual void mouseEvent() override;      // 处理移动

protected:
    virtual void buttonClicked(Button* button);

private:
    void addButton(int id, const std::string& text);
    void executeAction(int id);
    void updateScrollLimits();

    Minecraft* mc;
    std::vector<Button*> debugButtons;

    float scrollY;
    float maxScroll;
    float lastTouchY;
    bool  dragging;
    int   contentHeight;
    int   viewportHeight;

    static const int BTN_GODMODE    = 0;
    static const int BTN_GAMEMODE   = 1;
    static const int BTN_TIME       = 2;
    static const int BTN_ARMOR      = 3;
    static const int BTN_HURT_RELOAD= 4;
    static const int BTN_SPAWNMOB   = 5;
    static const int BTN_MASSACRE   = 6;
    static const int BTN_CLEARINV   = 7;
    static const int BTN_PRERENDER  = 8;
    static const int BTN_DROPALL    = 9;
    static const int BTN_SPEEDUP    = 10;
    static const int BTN_3RDPERSON  = 11;
};

#endif
