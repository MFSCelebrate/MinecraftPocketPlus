#include "DebugScreen.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/level/Level.h"
#include "../../../world/entity/MobFactory.h"
#include "../../../world/level/MobSpawner.h"
#include "../../../world/entity/player/Inventory.h"
#include "PrerenderTilesScreen.h"
#include "ArmorScreen.h"
#include "../../gamemode/GameMode.h"
#include "../../renderer/Textures.h"
#include "../../Options.h"
#include "../../../network/packet/AdventureSettingsPacket.h"
#include "../../../network/RakNetInstance.h"
#include "../../sound/SoundEngine.h"
#include "../../../util/DebugLog.h"
#include <cmath>

DebugScreen::DebugScreen(Minecraft* mc)
    : mc(mc), _pressedButton(nullptr), columns(0), btnWidth(0), btnHeight(0)
{
    DLOG_C("DebugScreen constructed, mc=%p", mc);
}

DebugScreen::~DebugScreen()
{
    for (auto* b : debugButtons) delete b;
}

void DebugScreen::init()
{
    DLOG_C("DebugScreen::init() start, width=%d, height=%d, InvGuiScale=%.3f, GuiScale=%.3f",
           width, height, Gui::InvGuiScale, Gui::GuiScale);

    // 防御性修正：如果 Screen::width/height 无效，从 Minecraft 实例重新计算
    if (mc && mc->width > 0 && mc->height > 0 && Gui::InvGuiScale > 0.0f) {
        width  = (int)(mc->width  * Gui::InvGuiScale);
        height = (int)(mc->height * Gui::InvGuiScale);
        DLOG_C("Corrected width=%d, height=%d from mc->width=%d, mc->height=%d, InvGuiScale=%.3f",
               width, height, mc->width, mc->height, Gui::InvGuiScale);
    } else {
        DLOG_ERROR(CLIENT, "mc or Gui::InvGuiScale invalid! mc=%p, mc->width=%d, mc->height=%d, InvGuiScale=%.3f",
                   (void*)mc, mc ? mc->width : -1, mc ? mc->height : -1, Gui::InvGuiScale);
    }

    addButton(BTN_GODMODE,      "God");
    addButton(BTN_GAMEMODE,     "Gamemode");
    addButton(BTN_TIME,         "Time +");
    addButton(BTN_ARMOR,        "Armor");
    addButton(BTN_HURT_RELOAD,  "Hurt+Reload");
    addButton(BTN_SPAWNMOB,     "Spawn Mob");
    addButton(BTN_MASSACRE,     "Massacre");
    addButton(BTN_CLEARINV,     "Clear Inv");
    addButton(BTN_PRERENDER,    "PreRender");
    addButton(BTN_DROPALL,      "Drop All");
    addButton(BTN_SPEEDUP,      "Speed Up");
    addButton(BTN_3RDPERSON,    "3rd Person");
    addButton(BTN_NOPVP,        "NoPvP");
    addButton(BTN_NOPVM,        "NoPvM");
    addButton(BTN_NOMVP,        "NoMvP");
    addButton(BTN_IMMUTABLE,    "Immutable");
    addButton(BTN_NAMETAGS,     "NameTags");
    addButton(BTN_PARTICLES,    "Particles");

    Button* closeBtn = new Button(99, "Close");
    closeBtn->width = 120;
    closeBtn->height = 30;
    buttons.push_back(closeBtn);

    passEvents = false;

    DLOG_C("DebugScreen::init(): %d buttons total", (int)buttons.size());

    setupPositions();
}

void DebugScreen::addButton(int id, const std::string& text)
{
    Button* btn = new Button(id, text);
    buttons.push_back(btn);
    debugButtons.push_back(btn);
}

void DebugScreen::setupPositions()
{
    if (!mc || mc->width <= 0 || mc->height <= 0) {
        DLOG_ERROR(CLIENT, "setupPositions called with invalid mc dimensions");
        return;
    }

    const int totalBtns = (int)debugButtons.size();
    btnWidth = 100;
    btnHeight = 24;
    const int padding = 4;

    // 逻辑宽度 = 物理宽度 * InvGuiScale
    int logicalWidth  = (int)(mc->width  * Gui::InvGuiScale);
    int logicalHeight = (int)(mc->height * Gui::InvGuiScale);

    DLOG_C("setupPositions: physical=%dx%d, InvGuiScale=%.3f, logical=%dx%d",
           mc->width, mc->height, Gui::InvGuiScale, logicalWidth, logicalHeight);

    int maxColumns = (logicalWidth - 20) / (btnWidth + padding);
    if (maxColumns < 1) maxColumns = 1;

    if (totalBtns <= 6) {
        columns = 3;
    } else if (totalBtns <= 12) {
        columns = 4;
    } else {
        columns = 5;
    }
    if (columns > maxColumns) columns = maxColumns;

    int gridWidth = columns * btnWidth + (columns - 1) * padding;
    int startX = (logicalWidth - gridWidth) / 2;
    int startY = 35;

    DLOG_C("columns=%d, startX=%d, startY=%d, btnW=%d, btnH=%d",
           columns, startX, startY, btnWidth, btnHeight);

    for (size_t i = 0; i < debugButtons.size(); ++i) {
        int row = (int)i / columns;
        int col = (int)i % columns;
        debugButtons[i]->x = startX + col * (btnWidth + padding);
        debugButtons[i]->y = startY + row * (btnHeight + padding);
        debugButtons[i]->width = btnWidth;
        debugButtons[i]->height = btnHeight;

        DLOG_C(" Btn[%d] '%s' at (%d,%d) %dx%d", (int)i, debugButtons[i]->msg.c_str(),
               debugButtons[i]->x, debugButtons[i]->y, btnWidth, btnHeight);
    }

    Button* closeBtn = nullptr;
    for (auto* b : buttons) {
        if (b->id == 99) { closeBtn = b; break; }
    }
    if (closeBtn) {
        closeBtn->x = (logicalWidth - closeBtn->width) / 2;
        int lastRow = (totalBtns - 1) / columns;
        int lastY = startY + lastRow * (btnHeight + padding) + btnHeight;
        closeBtn->y = lastY + 20;
        DLOG_C(" CloseBtn at (%d,%d) %dx%d", closeBtn->x, closeBtn->y, closeBtn->width, closeBtn->height);
    }
}

void DebugScreen::render(int xm, int ym, float a)
{
    fill(0, 0, width, height, 0x80000000);
    drawCenteredString(mc->font, "Debug Panel", width / 2, 8, 0xFFFFFFFF);
    Screen::render(xm, ym, a);
}

void DebugScreen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum != MouseAction::ACTION_LEFT) return;

    int logicalX = (int)(x * Gui::InvGuiScale);
    int logicalY = (int)(y * Gui::InvGuiScale);

    DLOG_C("click raw(%d,%d) logical(%d,%d) InvScale=%.3f", x, y, logicalX, logicalY, Gui::InvGuiScale);

    for (auto* btn : buttons) {
        if (btn->active &&
            logicalX >= btn->x && logicalX < btn->x + btn->width &&
            logicalY >= btn->y && logicalY < btn->y + btn->height)
        {
            _pressedButton = btn;
            _pressedButton->setPressed();
            DLOG_C("  Hit '%s' (%d,%d %dx%d)", btn->msg.c_str(), btn->x, btn->y, btn->width, btn->height);
            return;
        }
    }
    DLOG_C("  No hit");
}

void DebugScreen::mouseReleased(int x, int y, int buttonNum)
{
    if (buttonNum != MouseAction::ACTION_LEFT) return;

    int logicalX = (int)(x * Gui::InvGuiScale);
    int logicalY = (int)(y * Gui::InvGuiScale);

    if (_pressedButton) {
        bool inside = (_pressedButton->active &&
                       logicalX >= _pressedButton->x && logicalX < _pressedButton->x + _pressedButton->width &&
                       logicalY >= _pressedButton->y && logicalY < _pressedButton->y + _pressedButton->height);

        DLOG_C("release logical(%d,%d) inside=%d", logicalX, logicalY, (int)inside);

        if (inside) {
            buttonClicked(_pressedButton);
            mc->soundEngine->playUI("random.click", 1, 1);
        }
        _pressedButton->released(logicalX, logicalY);
        _pressedButton = nullptr;
    }
}

void DebugScreen::buttonClicked(Button* button)
{
    DLOG_C("buttonClicked id=%d '%s'", button->id, button->msg.c_str());
    if (button->id == 99) {
        mc->setScreen(NULL);
        return;
    }
    executeAction(button->id);
    if (button->id != BTN_ARMOR && button->id != BTN_PRERENDER) {
        mc->setScreen(NULL);
    }
}

// ... executeAction 实现同上，略 ...
