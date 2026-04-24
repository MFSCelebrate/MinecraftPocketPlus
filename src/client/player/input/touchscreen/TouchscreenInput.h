#ifndef NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchscreenInput_H__
#define NET_MINECRAFT_CLIENT_PLAYER_INPUT_TOUCHSCREEN_TouchscreenInput_H__

#include "../IMoveInput.h"
#include "../../../gui/GuiComponent.h"
#include "TouchAreaModel.h"
#include "../../../renderer/RenderChunk.h"

class Options;
class Player;
class Minecraft;
class PolygonArea;

class TouchscreenInput_TestFps : public IMoveInput,
                                 public GuiComponent
{
public:
    static const int KEY_UP = 0;
    static const int KEY_DOWN = 1;
    static const int KEY_LEFT = 2;
    static const int KEY_RIGHT = 3;
    static const int KEY_JUMP = 4;
    static const int KEY_SNEAK = 5;
    static const int KEY_CRAFT = 6;
    static const int NumKeys = 7;

    TouchscreenInput_TestFps(Minecraft* mc, Options* options);
    ~TouchscreenInput_TestFps();

    void onConfigChanged(const Config& c);
    void tick(Player* player);
    void render(float a);
    void setKey(int key, bool state);
    void releaseAllKeys();
    const RectangleArea& getRectangleArea();
    const RectangleArea& getPauseRectangleArea();

private:
    void clear();
    void rebuild();
    bool isButtonDown(int areaId);
    void executeDebugAction(int btnIdx);

    RectangleArea _boundingRectangle;
    bool _keys[NumKeys];
    Options* _options;

    bool _pressedJump;
    bool _forward;
    bool _northJump;
    bool _renderFlightImage;

    TouchAreaModel _model;
    Minecraft* _minecraft;

    RectangleArea* aLeft;
    RectangleArea* aRight;
    RectangleArea* aUp;
    RectangleArea* aDown;
    RectangleArea* aPause;
    RectangleArea* aChat;
    RectangleArea* aJump;
    RectangleArea* aUpLeft;
    RectangleArea* aUpRight;

    bool _pauseIsDown;
    RenderChunk _render;
    bool _allowHeightChange;
    float _sneakTapTime;
    bool _buttons[8];

    // ========== 调试面板 ==========
    // 调试齿轮（不涉及任何新对象创建）
bool _debugPanelVisible;
RectangleArea* aDebug;
static const int AREA_DEBUG = 200;                   // 入口按钮
   // RectangleArea* aDebugBG;                 // 面板背景
   // std::vector<RectangleArea*> _debugButtons; // 所有调试按钮区域
    //static const int DEBUG_PANEL_ROWS = 3;
   // static const int DEBUG_PANEL_COLS = 4;   // 3行4列 → 12个按钮
    //static const int NUM_DEBUG_BUTTONS = 12;
  //  static const char* DEBUG_LABELS[12];     // 按钮上的文字
};

#endif
