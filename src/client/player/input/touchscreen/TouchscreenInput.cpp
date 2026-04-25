#include "TouchscreenInput.h"
#include "../../../Options.h"
#include "../../../../platform/input/Multitouch.h"
#include "../../../gui/Gui.h"
#include "../../../renderer/Tesselator.h"
#include "../../../../world/entity/player/Player.h"
#include "../../../Minecraft.h"
#include "../../../../platform/log.h"
#include "../../../renderer/Textures.h"
#include "../../../sound/SoundEngine.h"
#include "client/gui/screens/ScreenChooser.h"
#include "client/gui/Gui.h"
#include <cmath>

// 区域ID
static const int AREA_DPAD_FIRST = 100;
static const int AREA_DPAD_N = 100;
static const int AREA_DPAD_S = 101;
static const int AREA_DPAD_W = 102;
static const int AREA_DPAD_E = 103;
static const int AREA_DPAD_C = 104;
static const int AREA_PAUSE = 105;
static const int AREA_CHAT = 106;

static int cPressed = 0, cReleased = 0, cDiscreet = 0, cPressedPause = 0, cReleasedPause = 0;

// 辅助绘制函数
static void drawRectangleArea(Tesselator& t, RectangleArea* a, int ux, int vy, float ssz = 64.0f) {
    const float pm = 1.0f / 256.0f;
    const float sz = ssz * pm;
    const float uu = (float)(ux) * pm;
    const float vv = (float)(vy) * pm;
    const float x0 = a->_x0 * Gui::InvGuiScale;
    const float x1 = a->_x1 * Gui::InvGuiScale;
    const float y0 = a->_y0 * Gui::InvGuiScale;
    const float y1 = a->_y1 * Gui::InvGuiScale;
    t.vertexUV(x0, y1, 0, uu, vv+sz);
    t.vertexUV(x1, y1, 0, uu+sz, vv+sz);
    t.vertexUV(x1, y0, 0, uu+sz, vv);
    t.vertexUV(x0, y0, 0, uu, vv);
}

static void drawPolygonArea(Tesselator& t, PolygonArea* a, int x, int y) {
    float pm = 1.0f / 256.0f;
    float sz = 64.0f * pm;
    float uu = (float)(x) * pm;
    float vv = (float)(y) * pm;
    float uvs[] = {uu, vv, uu+sz, vv, uu+sz, vv+sz, uu, vv+sz};
    const int o = 0;
    for (int j = 0; j < a->_numPoints; ++j) {
        t.vertexUV(a->_x[j] * Gui::InvGuiScale, a->_y[j] * Gui::InvGuiScale, 0,
                   uvs[(o+j+j)&7], uvs[(o+j+j+1)&7]);
    }
}

// ---------- 构造与析构 ----------
TouchscreenInput_TestFps::TouchscreenInput_TestFps( Minecraft* mc, Options* options )
    : _minecraft(mc),
      _options(options),
      _northJump(false),
      _forward(false),
      _boundingRectangle(0, 0, 1, 1),
      _pressedJump(false),
      _pauseIsDown(false),
      _sneakTapTime(-999),
      aLeft(0), aRight(0), aUp(0), aDown(0), aJump(0), aUpLeft(0), aUpRight(0),
      aDebug(nullptr)          // 新增加
{
    releaseAllKeys();
    onConfigChanged( createConfig(mc) );

    Tesselator& t = Tesselator::instance;
    const int alpha = 128;
    t.color( 0xc0c0c0, alpha); cPressed = t.getColor();
    t.color( 0xffffff, alpha); cReleased = t.getColor();
    t.color( 0xffffff, alpha / 4); cDiscreet = t.getColor();
    t.color( 0xc0c0c0, 80); cPressedPause=t.getColor();
    t.color( 0xffffff, 80); cReleasedPause=t.getColor();
}

TouchscreenInput_TestFps::~TouchscreenInput_TestFps() {
    clear();
}

void TouchscreenInput_TestFps::clear() {
    _model.clear();
    delete aUpLeft; aUpLeft = nullptr;
    delete aUpRight; aUpRight = nullptr;
    delete aDebug; aDebug = nullptr;   // 新增加
}

bool TouchscreenInput_TestFps::isButtonDown(int areaId) {
    return _buttons[areaId - AREA_DPAD_FIRST];
}

// ---------- onConfigChanged (添加齿轮区域) ----------
void TouchscreenInput_TestFps::onConfigChanged(const Config& c) {
    clear();

    const float w = (float)c.width;
    const float h = (float)c.height;

    float Bw = w * 0.11f;
    float Bh = Bw;
    PixelCalc& pc = _minecraft->pixelCalc;
    if (pc.pixelsToMillimeters(Bw) > 200) {
        Bw = Bh = pc.millimetersToPixels(200);
    }

    const float BaseY = -8 + h - 3.0f * Bh;
    const float BaseX = _options->getBooleanValue(OPTIONS_IS_LEFT_HANDED) ? -8 + w - 3 * Bw : 8 + 0;
    _boundingRectangle = RectangleArea(BaseX, BaseY, BaseX + 3 * Bw, BaseY + 3 * Bh);

    float xx = BaseX + Bw, yy = BaseY;
    _model.addArea(AREA_DPAD_N, aUp = new RectangleArea(xx, yy, xx+Bw, yy+Bh));
    xx = BaseX;
    aUpLeft = new RectangleArea(xx, yy, xx+Bw, yy+Bh);
    xx = BaseX + 2 * Bw;
    aUpRight = new RectangleArea(xx, yy, xx+Bw, yy+Bh);

    xx = BaseX + Bw; yy = BaseY + Bh;
    _model.addArea(AREA_DPAD_C, aJump = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

    xx = BaseX + Bw; yy = BaseY + 2 * Bh;
    _model.addArea(AREA_DPAD_S, aDown = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

    xx = BaseX; yy = BaseY + Bh;
    _model.addArea(AREA_DPAD_W, aLeft = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

    xx = BaseX + 2 * Bw; yy = BaseY + Bh;
    _model.addArea(AREA_DPAD_E, aRight = new RectangleArea(xx, yy, xx+Bw, yy+Bh));

    float maxPixels = _minecraft->pixelCalc.millimetersToPixels(10);
    float btnSize = _minecraft->pixelCalc.millimetersToPixels(18 * Gui::GuiScale);
    _model.addArea(AREA_PAUSE, aPause = new RectangleArea(w - 4 - btnSize, 4, w - 4, 4 + btnSize));
    _model.addArea(AREA_CHAT, aChat = new RectangleArea(w - 8 - btnSize * 2, 4, w - 8 - btnSize, 4 + btnSize));

    // 新增齿轮区域（在聊天左侧）
    aDebug = new RectangleArea(w - 8 - btnSize * 3 - 4, 4, w - 8 - btnSize * 2 - 4, 4 + btnSize);
    _model.addArea(AREA_DEBUG, aDebug);
}

// ---------- tick (添加 AREA_DEBUG 处理) ----------
void TouchscreenInput_TestFps::tick( Player* player ) {
    xa = ya = 0;
    jumping = false;
    bool heldJump = false;
    bool tmpForward = false;
    bool tmpNorthJump = false;

    for (int i = 0; i < 6; ++i) _buttons[i] = false;

    const int* pointerIds;
    int pointerCount = Multitouch::getActivePointerIdsThisUpdate(&pointerIds);
    for (int i = 0; i < pointerCount; ++i) {
        int p = pointerIds[i];
        int x = Multitouch::getX(p), y = Multitouch::getY(p);

        if (_boundingRectangle.isInside((float)x, (float)y) && _forward && !isChangingFlightHeight) {
            float angle = Mth::PI + Mth::atan2(y - _boundingRectangle.centerY(), x - _boundingRectangle.centerX());
            ya = Mth::sin(angle);
            xa = Mth::cos(angle);
            tmpForward = true;
        }

        int areaId = _model.getPointerId(x, y, p);
        if (areaId < AREA_DPAD_FIRST) continue;

        bool setButton = false;

        if (Multitouch::isPressed(p))
            _allowHeightChange = (areaId == AREA_DPAD_C);

        if (areaId == AREA_DPAD_C) {
            setButton = true;
            heldJump = true;
            if (player->isInWater()) jumping = true;
            else if (Multitouch::isPressed(p)) jumping = true;
            else if (_forward && !player->abilities.flying) { areaId = AREA_DPAD_N; tmpNorthJump = true; ya += 1; }
        }

        if (areaId == AREA_DPAD_N) {
            setButton = true;
            if (player->isInWater()) jumping = true;
            else if (!isChangingFlightHeight) tmpForward = true;
            ya += 1;
        }
        else if (areaId == AREA_DPAD_S && !_forward) { setButton = true; ya -= 1; }
        else if (areaId == AREA_DPAD_W && !_forward) { setButton = true; xa += 1; }
        else if (areaId == AREA_DPAD_E && !_forward) { setButton = true; xa -= 1; }
        else if (areaId == AREA_PAUSE) {
            if (Multitouch::isReleased(p)) {
                _minecraft->soundEngine->playUI("random.click", 1, 1);
                _minecraft->screenChooser.setScreen(SCREEN_PAUSE);
            }
        }
        else if (areaId == AREA_CHAT) {
            if (Multitouch::isReleased(p)) {
                _minecraft->soundEngine->playUI("random.click", 1, 1);
                _minecraft->screenChooser.setScreen(SCREEN_CONSOLE);
                _minecraft->platform()->showKeyboard();
            }
        }
        // 新增：齿轮打开调试屏幕
        else if (areaId == AREA_DEBUG) {
            if (Multitouch::isReleased(p)) {
                _minecraft->soundEngine->playUI("random.click", 1, 1);
                _minecraft->screenChooser.setScreen(SCREEN_DEBUG);
            }
        }

        _buttons[areaId - AREA_DPAD_FIRST] = setButton;
    }

    _forward = tmpForward;
    if (tmpNorthJump) { if (!_northJump) jumping = true; _northJump = true; } else _northJump = false;

    isChangingFlightHeight = false;
    wantUp = isButtonDown(AREA_DPAD_N) && (_allowHeightChange & (_pressedJump | wantUp));
    wantDown = isButtonDown(AREA_DPAD_S) && (_allowHeightChange & (_pressedJump | wantDown));
    if (player->abilities.flying && (wantUp || wantDown || (heldJump && !_forward))) { isChangingFlightHeight = true; ya = 0; }
    _renderFlightImage = player->abilities.flying;

#ifdef WIN32
    if (_keys[KEY_UP]) ya++;
    if (_keys[KEY_DOWN]) ya--;
    if (_keys[KEY_LEFT]) xa++;
    if (_keys[KEY_RIGHT]) xa--;
    if (_keys[KEY_JUMP]) jumping = true;
#endif

    if (sneaking) { xa *= 0.3f; ya *= 0.3f; }
    _pressedJump = heldJump;
}

// ---------- render, setKey, releaseAllKeys, getRectangleArea, getPauseRectangleArea (不变) ----------
void TouchscreenInput_TestFps::render( float a ) {
    glDisable2(GL_ALPHA_TEST);
    glEnable2(GL_BLEND);
    glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _minecraft->textures->loadAndBindTexture("gui/gui.png");
    rebuild();
    glDisable2(GL_BLEND);
}

void TouchscreenInput_TestFps::setKey( int key, bool state ) {
#ifdef WIN32
    int id = -1;
    if (key == _options->getIntValue(OPTIONS_KEY_FORWARD)) id = KEY_UP;
    if (key == _options->getIntValue(OPTIONS_KEY_BACK))   id = KEY_DOWN;
    if (key == _options->getIntValue(OPTIONS_KEY_LEFT))   id = KEY_LEFT;
    if (key == _options->getIntValue(OPTIONS_KEY_RIGHT))  id = KEY_RIGHT;
    if (key == _options->getIntValue(OPTIONS_KEY_JUMP))   id = KEY_JUMP;
    if (key == _options->getIntValue(OPTIONS_KEY_SNEAK))  id = KEY_SNEAK;
#endif
}

void TouchscreenInput_TestFps::releaseAllKeys() {
    xa = ya = 0;
    for (int i = 0; i < 8; ++i) _buttons[i] = false;
#ifdef WIN32
    for (int i = 0; i < NumKeys; ++i) _keys[i] = false;
#endif
    _pressedJump = false;
    _allowHeightChange = false;
}

const RectangleArea& TouchscreenInput_TestFps::getRectangleArea() {
    return _boundingRectangle;
}

const RectangleArea& TouchscreenInput_TestFps::getPauseRectangleArea() {
    return *aPause;
}

// ---------- rebuild (添加齿轮图标绘制) ----------
void TouchscreenInput_TestFps::rebuild() {
    if (_options->getBooleanValue(OPTIONS_HIDEGUI)) return;

    Tesselator& t = Tesselator::instance;
    t.begin();

    const int imageU = 0;
    const int imageV = 107;
    const int imageSize = 26;

    bool northDiagonals = !isChangingFlightHeight && (_northJump || _forward);

    if (northDiagonals || isChangingFlightHeight) t.colorABGR(cDiscreet);
    else if (isButtonDown(AREA_DPAD_W)) t.colorABGR(cPressed);
    else t.colorABGR(cReleased);
    drawRectangleArea(t, aLeft, imageU + imageSize, imageV, (float)imageSize);

    if (northDiagonals || isChangingFlightHeight) t.colorABGR(cDiscreet);
    else if (isButtonDown(AREA_DPAD_E)) t.colorABGR(cPressed);
    else t.colorABGR(cReleased);
    drawRectangleArea(t, aRight, imageU + imageSize * 3, imageV, (float)imageSize);

    if (isButtonDown(AREA_DPAD_N)) t.colorABGR(cPressed);
    else t.colorABGR(cReleased);
    if (isChangingFlightHeight) drawRectangleArea(t, aUp, imageU + imageSize * 2, imageV + imageSize, (float)imageSize);
    else drawRectangleArea(t, aUp, imageU, imageV, (float)imageSize);

    if (northDiagonals) {
        t.colorABGR(cReleased);
        drawRectangleArea(t, aUpLeft, imageU, imageV + imageSize, (float)imageSize);
        drawRectangleArea(t, aUpRight, imageU + imageSize, imageV + imageSize, (float)imageSize);
    }

    if (northDiagonals) t.colorABGR(cDiscreet);
    else if (isButtonDown(AREA_DPAD_S)) t.colorABGR(cPressed);
    else t.colorABGR(cReleased);
    if (isChangingFlightHeight) drawRectangleArea(t, aDown, imageU + imageSize * 3, imageV + imageSize, (float)imageSize);
    else drawRectangleArea(t, aDown, imageU + imageSize * 2, imageV, (float)imageSize);

    if (_renderFlightImage && northDiagonals) t.colorABGR(cDiscreet);
    else if (isButtonDown(AREA_DPAD_C)) t.colorABGR(cPressed);
    else t.colorABGR(cReleased);
    if (_renderFlightImage) drawRectangleArea(t, aJump, imageU + imageSize * 4, imageV + imageSize, (float)imageSize);
    else drawRectangleArea(t, aJump, imageU + imageSize * 4, imageV, (float)imageSize);

    // 右上角按钮（暂停、聊天、调试）
    if (!_minecraft->screen) {
        t.colorABGR(0xFFFFFFFF);
        drawRectangleArea(t, aPause, 200, 64, 18.0f);
        drawRectangleArea(t, aChat, 200, 82, 18.0f);
        drawRectangleArea(t, aDebug, 200, 64, 18.0f);   // 齿轮图标
    }

    t.draw();
}
