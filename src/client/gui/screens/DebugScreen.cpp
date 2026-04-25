#include "DebugScreen.h"
#include "../components/Button.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../../world/level/Level.h"
#include "../../world/item/ItemInstance.h"
#include "../../world/entity/MobFactory.h"
#include "../../world/level/MobSpawner.h"
#include "../../world/entity/player/Inventory.h"
#include "PrerenderTilesScreen.h"
#include "ArmorScreen.h"
#include "client/gamemode/GameMode.h"
#include "client/renderer/Textures.h"
#include "client/Options.h"
#include <cmath>

DebugScreen::DebugScreen(Minecraft* mc) : mc(mc) {
}

DebugScreen::~DebugScreen() {
    for (auto* b : debugButtons) delete b;
}

void DebugScreen::init() {
    // 创建12个调试按钮
    addButton(BTN_GODMODE, "God Mode");
    addButton(BTN_GAMEMODE, "Gamemode");
    addButton(BTN_TIME, "Time +");
    addButton(BTN_ARMOR, "Armor");
    addButton(BTN_HURT_RELOAD, "Hurt+Reload");
    addButton(BTN_SPAWNMOB, "Spawn Mob");
    addButton(BTN_MASSACRE, "Massacre");
    addButton(BTN_CLEARINV, "Clear Inv");
    addButton(BTN_PRERENDER, "PreRender");
    addButton(BTN_DROPALL, "Drop All");
    addButton(BTN_SPEEDUP, "Speed Up");
    addButton(BTN_3RDPERSON, "3rd Person");

    // 加入关闭按钮
    Button* closeBtn = new Button(99, "Close");
    closeBtn->width = 120;
    buttons.push_back(closeBtn);
}

void DebugScreen::addButton(int id, const std::string& text) {
    Button* btn = new Button(id, text);
    btn->width = 120;
    btn->height = 30;
    buttons.push_back(btn);
    debugButtons.push_back(btn);
}

void DebugScreen::setupPositions() {
    int startX = width / 2 - 130;
    int startY = height / 2 - (buttons.size() * 35) / 2;
    for (size_t i = 0; i < buttons.size(); ++i) {
        buttons[i]->x = startX;
        buttons[i]->y = startY + i * 35;
        buttons[i]->width = 260;
        buttons[i]->height = 30;
    }
}

void DebugScreen::render(int xm, int ym, float a) {
    // 半透明背景
    fill(0, 0, width, height, 0x80000000);
    // 标题
    drawCenteredString(mc->font, "Debug Panel", width / 2, 
                       height / 2 - (buttons.size() * 35) / 2 - 20, 0xFFFFFFFF);
    Screen::render(xm, ym, a);
}

void DebugScreen::buttonClicked(Button* button) {
    if (button->id == 99) {
        mc->setScreen(NULL); // 关闭面板
        return;
    }
    executeAction(button->id);
    // 部分操作会打开新 Screen，不需要关闭面板；其余操作关闭面板
    if (button->id != BTN_ARMOR && button->id != BTN_PRERENDER) {
        mc->setScreen(NULL);
    }
}

void DebugScreen::executeAction(int id) {
    switch (id) {
        case BTN_GODMODE: // GodMode (KEY_U)
            mc->onGraphicsReset();
            mc->player->heal(100);
            break;
        case BTN_GAMEMODE: // Gamemode toggle (KEY_B)
            mc->setIsCreativeMode(!mc->isCreativeMode());
            break;
        case BTN_TIME: // Time + (KEY_P)
            if (mc->level) mc->level->setTime(mc->level->getTime() + 1000);
            break;
        case BTN_ARMOR: // Armor (KEY_G)
            mc->setScreen(new ArmorScreen());
            break;
        case BTN_HURT_RELOAD: // Hurt+Reload (KEY_Y)
            mc->textures->reloadAll();
            mc->player->hurtTo(2);
            break;
        case BTN_SPAWNMOB: // Spawn mob (KEY_Z)
        {
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
        case BTN_MASSACRE: // Kill all non-players (KEY_X)
        {
            const EntityList& entities = mc->level->getAllEntities();
            for (int i = entities.size() - 1; i >= 0; --i) {
                Entity* e = entities[i];
                if (!e->isPlayer()) mc->level->removeEntity(e);
            }
            break;
        }
        case BTN_CLEARINV: // Clear inventory (KEY_C)
            mc->player->inventory->clearInventoryWithDefault();
            break;
        case BTN_PRERENDER: // Prerender tiles (KEY_H)
            mc->setScreen(new PrerenderTilesScreen());
            break;
        case BTN_DROPALL: // Drop all (KEY_O)
            for (int i = Inventory::MAX_SELECTION_SIZE; i < mc->player->inventory->getContainerSize(); ++i)
                if (mc->player->inventory->getItem(i))
                    mc->player->inventory->dropSlot(i, false);
            break;
        case BTN_SPEEDUP: // Speed up ticks (KEY_M)
            for (int i = 0; i < 5 * SharedConstants::TicksPerSecond; ++i)
                mc->level->tick();
            break;
        case BTN_3RDPERSON: // 3rd person (KEY_F5)
            mc->options.toggle(OPTIONS_THIRD_PERSON_VIEW);
            break;
    }
}
