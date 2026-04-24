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
#include "world/entity/MobFactory.h"
#include "world/level/MobSpawner.h"
#include "client/Options.h"
#include "world/level/Level.h"
#include "world/level/tile/Tile.h"
#include "client/gui/screens/PrerenderTilesScreen.h"
#include "client/gamemode/GameMode.h"
#include "world/entity/player/Inventory.h"
#include "client/player/LocalPlayer.h"          // 修复 LocalPlayer 不完整类型
#include "client/gui/screens/ArmorScreen.h"    // 修复 ArmorScreen 找不到
#include "client/renderer/Textures.h"
#include "client/renderer/entity/EntityRenderDispatcher.h"
#include "client/renderer/LevelRenderer.h"
#include "client/renderer/GameRenderer.h"
#include <cmath>

// 原有区域ID
static const int AREA_DPAD_FIRST = 100;
static const int AREA_DPAD_N = 100;
static const int AREA_DPAD_S = 101;
static const int AREA_DPAD_W = 102;
static const int AREA_DPAD_E = 103;
static const int AREA_DPAD_C = 104;
static const int AREA_PAUSE = 105;
static const int AREA_CHAT = 106;
// 调试区域ID
static const int AREA_DEBUG = 200;
static const int AREA_DBG_BTN0 = 201;

static int cPressed = 0, cReleased = 0, cDiscreet = 0, cPressedPause = 0, cReleasedPause = 0;

// 调试按钮标签
const char* TouchscreenInput_TestFps::DEBUG_LABELS[12] = {
    "GodMode", "Gamemode", "Time+", "Armor",
    "Hurt+Reload", "SpawnMob", "Massacre", "ClearInv",
    "PreRender", "DropAll", "SpeedUp", "3rdPerson"
};

// ========== 辅助绘制函数（必须放在 rebuild 之前） ==========
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
        t.vertexUV(a->_x[j] * Gui::InvGuiScale, a->_y[j] * Gui::InvGuiScale, 0, uvs[(o+j+j)&7], uvs[(o+j+j+1)&7]);
    }
}

// ==================== 构造函数 ====================
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
      aDebug(nullptr), aDebugBG(nullptr),
      _debugPanelVisible(false)
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
    delete aDebug; aDebug = nullptr;
    delete aDebugBG; aDebugBG = nullptr;
    for (auto* btn : _debugButtons) delete btn;
    _debugButtons.clear();
}

bool TouchscreenInput_TestFps::isButtonDown(int areaId) {
    return _buttons[areaId - AREA_DPAD_FIRST];
}

// ==================== onConfigChanged ====================
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
    // 新增加密按钮入口
    aDebug = new RectangleArea(w - 8 - btnSize * 3 - 4, 4, w - 8 - btnSize * 2 - 4, 4 + btnSize);
    _model.addArea(AREA_DEBUG, aDebug);

    // 如果面板已打开，重新生成面板按钮网格
    if (_debugPanelVisible) {
        aDebugBG = new RectangleArea(50, 80, w - 50, h - 80);
        for (auto* btn : _debugButtons) delete btn;
        _debugButtons.clear();

        int gridW = 64 * DEBUG_PANEL_COLS;
        int gridH = 64 * DEBUG_PANEL_ROWS;
        int startX = (w - gridW) / 2;
        int startY = (h - gridH) / 2;
        for (int r = 0; r < DEBUG_PANEL_ROWS; ++r) {
            for (int col = 0; col < DEBUG_PANEL_COLS; ++col) {
                RectangleArea* btn = new RectangleArea(
                    startX + col * 64, startY + r * 64,
                    startX + (col+1) * 64, startY + (r+1) * 64 );
                _debugButtons.push_back(btn);
            }
        }
    }
}

// ==================== tick ====================
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

        // 调试按钮 (入口和面板内)
        int areaId2 = _model.getPointerId(x, y, p);
        if (areaId2 == AREA_DEBUG) {
            if (Multitouch::isReleased(p)) {
                _minecraft->soundEngine->playUI("random.click", 1, 1);
                _debugPanelVisible = !_debugPanelVisible;
             //   onConfigChanged( createConfig(_minecraft) );
            }
        }

        /*if (_debugPanelVisible) {
            int dbgIdx = areaId2 - AREA_DBG_BTN0;
            if (dbgIdx >= 0 && dbgIdx < (int)_debugButtons.size()) {
                if (Multitouch::isReleased(p)) {
                    executeDebugAction(dbgIdx);
                    _minecraft->soundEngine->playUI("random.click", 1, 1);
                }
            }
            continue;
        }*/

        // 原有移动逻辑
        if (_boundingRectangle.isInside((float)x, (float)y) && _forward && !isChangingFlightHeight) {
            float angle = Mth::PI + Mth::atan2(y - _boundingRectangle.centerY(), x - _boundingRectangle.centerX());
            ya = Mth::sin(angle);
            xa = Mth::cos(angle);
            tmpForward = true;
        }

        int areaId = _model.getPointerId(x, y, p);
        if (areaId < AREA_DPAD_FIRST) continue;

        bool setButton = false;
        if (Multitouch::isPressed(p)) _allowHeightChange = (areaId == AREA_DPAD_C);

        if (areaId == AREA_DPAD_C) {
            setButton = true; heldJump = true;
            if (player->isInWater()) jumping = true;
            else if (Multitouch::isPressed(p)) jumping = true;
            else if (_forward && !player->abilities.flying) { areaId = AREA_DPAD_N; tmpNorthJump = true; ya += 1; }
        }
        if (areaId == AREA_DPAD_N) { setButton = true; if (player->isInWater()) jumping = true; else if (!isChangingFlightHeight) tmpForward = true; ya += 1; }
        else if (areaId == AREA_DPAD_S && !_forward) { setButton = true; ya -= 1; }
        else if (areaId == AREA_DPAD_W && !_forward) { setButton = true; xa += 1; }
        else if (areaId == AREA_DPAD_E && !_forward) { setButton = true; xa -= 1; }
        else if (areaId == AREA_PAUSE) {
            if (Multitouch::isReleased(p)) { _minecraft->soundEngine->playUI("random.click", 1, 1); _minecraft->screenChooser.setScreen(SCREEN_PAUSE); }
        }
        else if (areaId == AREA_CHAT) {
            if (Multitouch::isReleased(p)) { _minecraft->soundEngine->playUI("random.click", 1, 1); _minecraft->screenChooser.setScreen(SCREEN_CONSOLE); _minecraft->platform()->showKeyboard(); }
        }
        _buttons[areaId - AREA_DPAD_FIRST] = setButton;
    }

    if (_debugPanelVisible) return;

    _forward = tmpForward;
    if (tmpNorthJump) { if (!_northJump) jumping = true; _northJump = true; }
    else _northJump = false;

    isChangingFlightHeight = false;
    wantUp = isButtonDown(AREA_DPAD_N) && (_allowHeightChange & (_pressedJump | wantUp));
    wantDown = isButtonDown(AREA_DPAD_S) && (_allowHeightChange & (_pressedJump | wantDown));
    if (player->abilities.flying && (wantUp || wantDown || (heldJump && !_forward))) { isChangingFlightHeight = true; ya = 0; }
    _renderFlightImage = player->abilities.flying;

    if (sneaking) { xa *= 0.3f; ya *= 0.3f; }
    _pressedJump = heldJump;
}

// ==================== rebuild ====================
void TouchscreenInput_TestFps::rebuild() {
    if (_options->getBooleanValue(OPTIONS_HIDEGUI)) return;

    Tesselator& t = Tesselator::instance;
    t.begin();

    const int imageU = 0, imageV = 107, imageSize = 26;
    bool northDiagonals = !isChangingFlightHeight && (_northJump || _forward);

    // 方向键按钮（左、右、上、下、对角、跳跃）
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

    // 右上角 HUD 按钮
    if (!_minecraft->screen) {
        t.colorABGR(0xFFFFFFFF);
        drawRectangleArea(t, aPause, 200, 64, 18.0f);
        drawRectangleArea(t, aChat, 200, 82, 18.0f);
        drawRectangleArea(t, aDebug, 200, 64, 18.0f); // 和暂停一样的图标
    }
    t.draw();

    // 调试面板渲染
    /*if (_debugPanelVisible) {
        float invScale = Gui::InvGuiScale;
        fill(50.f * invScale, 80.f * invScale,
             (_minecraft->width - 50) * invScale,
             (_minecraft->height - 80) * invScale,
             0x80000000);

        t.begin();
        t.colorABGR(0xFFFFFFFF);
        for (int i = 0; i < NUM_DEBUG_BUTTONS; ++i) {
            RectangleArea* btn = _debugButtons[i];
            drawRectangleArea(t, btn, 0, 0, 64.0f);
        }
        t.draw();

        for (int i = 0; i < NUM_DEBUG_BUTTONS; ++i) {
            RectangleArea* btn = _debugButtons[i];
            float cx = btn->centerX() * invScale;
            float cy = btn->centerY() * invScale;
            const char* label = DEBUG_LABELS[i];
            _minecraft->font->drawShadow(label,
                cx - _minecraft->font->width(label) * 0.5f,
                cy - _minecraft->font->lineHeight * 0.5f,
                0xFFFFFFFF);
        }
    }*/
}

// ==================== executeDebugAction ====================
void TouchscreenInput_TestFps::executeDebugAction(int btnIdx) {
    return;
    /*Minecraft* mc = _minecraft;
    switch (btnIdx) {
        case 0: // GodMode (KEY_U)
            mc->onGraphicsReset();
            mc->player->heal(100);
            break;
        case 1: // Gamemode (KEY_B)
            mc->setIsCreativeMode(!mc->isCreativeMode());
            break;
        case 2: // Time + (KEY_P)
            if (mc->level) mc->level->setTime(mc->level->getTime() + 1000);
            break;
        case 3: // Armor (KEY_G)
            mc->setScreen(new ArmorScreen());
            break;
        case 4: // Self hurt + reload textures (KEY_Y)
            mc->textures->reloadAll();
            mc->player->hurtTo(2);
            break;
        case 5: // Random spawn (KEY_Z)
            for (int i = 0; i < 1; ++i) {
                Mob* mob = nullptr;
                int types[] = { MobTypes::Sheep, MobTypes::Pig, MobTypes::Chicken, MobTypes::Cow };
                int mobType = types[Mth::random(4)];
                mob = MobFactory::CreateMob(mobType, mc->level);
                float dx = 4 - 8 * Mth::random() + 4 * Mth::sin(Mth::DEGRAD * mc->player->yRot);
                float dz = 4 - 8 * Mth::random() + 4 * Mth::cos(Mth::DEGRAD * mc->player->yRot);
                if (mob && !MobSpawner::addMob(mc->level, mob, mc->player->x + dx, mc->player->y, mc->player->z + dz, Mth::random()*360, 0, true))
                    delete mob;
            }
            break;
        case 6: // Kill all non-player entities (KEY_X)
        {
            const EntityList& entities = mc->level->getAllEntities();
            for (int i = entities.size() - 1; i >= 0; --i) {
                Entity* e = entities[i];
                if (!e->isPlayer()) mc->level->removeEntity(e);
            }
        }
            break;
        case 7: // Clear inventory (KEY_C)
            mc->player->inventory->clearInventoryWithDefault();
            break;
        case 8: // Prerender tiles (KEY_H)
            mc->setScreen(new PrerenderTilesScreen());
            break;
        case 9: // Drop all items (KEY_O)
            for (int i = Inventory::MAX_SELECTION_SIZE; i < mc->player->inventory->getContainerSize(); ++i)
                if (mc->player->inventory->getItem(i))
                    mc->player->inventory->dropSlot(i, false);
            break;
        case 10: // Speed up ticks (KEY_M)
            for (int i = 0; i < 5 * SharedConstants::TicksPerSecond; ++i)
                mc->level->tick();
            break;
        case 11: // 3rd person (KEY_F5)
            mc->options.toggle(OPTIONS_THIRD_PERSON_VIEW);
            break;
    }*/
}

// ==================== render ====================
void TouchscreenInput_TestFps::render( float a ) {
    glDisable2(GL_ALPHA_TEST);
    glEnable2(GL_BLEND);
    glBlendFunc2(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    _minecraft->textures->loadAndBindTexture("gui/gui.png");

    rebuild();

    glDisable2(GL_BLEND);
}

// ==================== setKey ====================
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

// ==================== releaseAllKeys ====================
void TouchscreenInput_TestFps::releaseAllKeys() {
    xa = ya = 0;
    for (int i = 0; i < 8; ++i) _buttons[i] = false;
#ifdef WIN32
    for (int i = 0; i < NumKeys; ++i) _keys[i] = false;
#endif
    _pressedJump = false;
    _allowHeightChange = false;
}

// ==================== getRectangleArea ====================
const RectangleArea& TouchscreenInput_TestFps::getRectangleArea() {
    return _boundingRectangle;
}

// ==================== getPauseRectangleArea ====================
const RectangleArea& TouchscreenInput_TestFps::getPauseRectangleArea() {
    return *aPause;
}

// 其他原有函数保持不变（setKey, releaseAllKeys, render 等省略，实际文件需包含）
