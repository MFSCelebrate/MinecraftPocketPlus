#include "DebugScreen.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../../world/level/Level.h"
#include "../../../world/item/ItemInstance.h"
#include "../../../world/entity/MobFactory.h"
#include "../../../world/level/MobSpawner.h"
#include "../../../world/entity/player/Inventory.h"
#include "PrerenderTilesScreen.h"
#include "ArmorScreen.h"
#include "../../gamemode/GameMode.h"
#include "../../renderer/Textures.h"
#include "../../Options.h"
#include <cmath>

DebugScreen::DebugScreen(Minecraft* mc) : mc(mc) {
}

DebugScreen::~DebugScreen() {
    for (auto* b : debugButtons) delete b;
}

void DebugScreen::init() {
    addButton(BTN_GODMODE,    "God Mode");
    addButton(BTN_GAMEMODE,   "Gamemode");
    addButton(BTN_TIME,       "Time +");
    addButton(BTN_ARMOR,      "Armor");
    addButton(BTN_HURT_RELOAD, "Hurt+Reload");
    addButton(BTN_SPAWNMOB,   "Spawn Mob");
    addButton(BTN_MASSACRE,   "Massacre");
    addButton(BTN_CLEARINV,   "Clear Inv");
    addButton(BTN_PRERENDER,  "PreRender");
    addButton(BTN_DROPALL,    "Drop All");
    addButton(BTN_SPEEDUP,    "Speed Up");
    addButton(BTN_3RDPERSON,  "3rd Person");

    // 关闭按钮
    Button* closeBtn = new Button(99, "Close");
    closeBtn->width  = 120;
    closeBtn->height = 30;
    buttons.push_back(closeBtn);
}

void DebugScreen::addButton(int id, const std::string& text) {
    Button* btn = new Button(id, text);
    btn->width  = 120;
    btn->height = 30;
    buttons.push_back(btn);
    debugButtons.push_back(btn);
}

void DebugScreen::setupPositions() {
    const int buttonPadding = 4;
    int buttonCount = (int)buttons.size();
    int buttonHeight = 30;                      // 固定高度，清晰易点
    int totalHeight = buttonCount * buttonHeight + (buttonCount - 1) * buttonPadding;

    // 垂直居中，并保证至少 10 像素上边距
    int startY = (height - totalHeight) / 2;
    if (startY < 10) startY = 10;

    // 按钮宽度取屏幕宽度的 70%，最大不超过 400 像素
    int buttonWidth = (int)(width * 0.7f);
    if (buttonWidth > 400) buttonWidth = 400;
    int startX = (width - buttonWidth) / 2;

    for (size_t i = 0; i < buttons.size(); ++i) {
        buttons[i]->x      = startX;
        buttons[i]->y      = startY + i * (buttonHeight + buttonPadding);
        buttons[i]->width  = buttonWidth;
        buttons[i]->height = buttonHeight;
    }
}

void DebugScreen::render(int xm, int ym, float a) {
    fill(0, 0, width, height, 0x80000000);

    // 标题位置动态适配
    int buttonCount = (int)buttons.size();
    int totalHeight = buttonCount * 30 + (buttonCount - 1) * 4;
    int topY = (height - totalHeight) / 2 - 20;
    if (topY < 10) topY = 10;
    drawCenteredString(mc->font, "Debug Panel", width / 2, topY, 0xFFFFFFFF);

    Screen::render(xm, ym, a);
}

void DebugScreen::buttonClicked(Button* button) {
    if (button->id == 99) {
        mc->setScreen(NULL);                // 关闭面板
        return;
    }
    executeAction(button->id);
    // Armor 和 Prerender 会打开新 Screen，自己不要关闭
    if (button->id != BTN_ARMOR && button->id != BTN_PRERENDER) {
        mc->setScreen(NULL);
    }
}

void DebugScreen::executeAction(int id) {
    switch (id) {
        case BTN_GODMODE:
            mc->onGraphicsReset();
            mc->player->heal(100);
            break;
        case BTN_GAMEMODE:
            mc->setIsCreativeMode(!mc->isCreativeMode());
            break;
        case BTN_TIME:
            if (mc->level) mc->level->setTime(mc->level->getTime() + 1000);
            break;
        case BTN_ARMOR:
            mc->setScreen(new ArmorScreen());
            break;
        case BTN_HURT_RELOAD:
            mc->textures->reloadAll();
            mc->player->hurtTo(2);
            break;
        case BTN_SPAWNMOB: {
            Mob* mob = nullptr;
            int types[] = { MobTypes::Sheep, MobTypes::Pig, MobTypes::Chicken, MobTypes::Cow };
            int mobType = types[Mth::random(4)];
            mob = MobFactory::CreateMob(mobType, mc->level);
            float dx = 4 - 8 * Mth::random() + 4 * Mth::sin(Mth::DEGRAD * mc->player->yRot);
            float dz = 4 - 8 * Mth::random() + 4 * Mth::cos(Mth::DEGRAD * mc->player->yRot);
            if (mob && !MobSpawner::addMob(mc->level, mob, mc->player->x + dx, mc->player->y, mc->player->z + dz, Mth::random()*360, 0, true))
                delete mob;
            break;
        }
        case BTN_MASSACRE: {
            const EntityList& entities = mc->level->getAllEntities();
            for (int i = entities.size() - 1; i >= 0; --i) {
                Entity* e = entities[i];
                if (!e->isPlayer()) mc->level->removeEntity(e);
            }
            break;
        }
        case BTN_CLEARINV:
            mc->player->inventory->clearInventoryWithDefault();
            break;
        case BTN_PRERENDER:
            mc->setScreen(new PrerenderTilesScreen());
            break;
        case BTN_DROPALL:
            for (int i = Inventory::MAX_SELECTION_SIZE; i < mc->player->inventory->getContainerSize(); ++i)
                if (mc->player->inventory->getItem(i))
                    mc->player->inventory->dropSlot(i, false);
            break;
        case BTN_SPEEDUP:
            for (int i = 0; i < 5 * SharedConstants::TicksPerSecond; ++i)
                mc->level->tick();
            break;
        case BTN_3RDPERSON:
            mc->options.toggle(OPTIONS_THIRD_PERSON_VIEW);
            break;
    }
}
