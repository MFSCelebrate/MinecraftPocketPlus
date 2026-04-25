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

    virtual void tick() override;

protected:
    virtual void buttonClicked(Button* button) override;

private:
    void addButton(int id, const std::string& text);
    void executeAction(int id);
    void updateScrollLimits();

    virtual void mouseClicked(int x, int y, int buttonNum) override;
    virtual void mouseReleased(int x, int y, int buttonNum) override;
    
    Minecraft* mc;
    std::vector<Button*> debugButtons;

    float scrollY;
    float maxScroll;
    float lastTouchY;
    bool dragging;
    int contentHeight;
    int viewportHeight;

    Button* _pressedButton;

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
