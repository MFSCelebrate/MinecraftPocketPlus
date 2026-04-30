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
#include "../../../world/Difficulty.h"
#include "../../../util/PerfRenderer.h"
#include "../../../client/gui/Gui.h"

DebugScreen::DebugScreen(Minecraft* mc)
    : mc(mc)
{
    for (int i = 0; i < 10; ++i)
        digitButtons[i] = nullptr;
}

DebugScreen::~DebugScreen()
{
    for (int i = 0; i < 10; ++i) delete digitButtons[i];
    for (auto* b : extraButtons) delete b;
}

void DebugScreen::init()
{
    // 数字按钮 0-9
    for (int i = 0; i < 10; ++i)
        addDigitButton(i);

    // 额外功能按钮（顺序保持）
    Button* b;

    b = new Button(ACT_HEAL_RESET, "Heal");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_TOGGLE_GAMEMODE, "Gamemode");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_ADVANCE_TIME, "Time +");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_OPEN_ARMOR, "Armor");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_HURT_RELOAD, "Hurt+Reload");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_SPAWN_MOB, "Spawn Mob");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_MASSACRE, "Kill All");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_REFILL_INV, "Refill Inv");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_PRERENDER, "PreRender");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_DROP_INV, "Drop All");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_TOGGLE_DIFFICULTY, "Toggle Diff");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_TOGGLE_3RDPERSON, "3rd Person");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_SPEEDUP, "Speed Up");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_NOPVP, "No PvP");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_NOPVM, "No PvM");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_NOMVP, "No MvP");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_IMMUTABLE, "Immutable");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_NAMETAGS, "NameTags");
    buttons.push_back(b); extraButtons.push_back(b);

    b = new Button(ACT_PARTICLES, "Particles");
    buttons.push_back(b); extraButtons.push_back(b);

    // 关闭按钮
    Button* closeBtn = new Button(99, "Close");
    buttons.push_back(closeBtn);

    passEvents = false;
}

void DebugScreen::addDigitButton(int digit)
{
    char label[2] = { (char)('0' + digit), 0 };
    Button* btn = new Button(digit, label);
    buttons.push_back(btn);
    digitButtons[digit] = btn;
}

void DebugScreen::setupPositions()
{
    // 确保宽高有效
    if (width <= 0 && mc && mc->width > 0 && Gui::InvGuiScale > 0)
        width = (int)(mc->width * Gui::InvGuiScale);
    if (height <= 0 && mc && mc->height > 0 && Gui::InvGuiScale > 0)
        height = (int)(mc->height * Gui::InvGuiScale);

    // ---------- 数字按钮网格 ----------
    const int digitCols = 5;          // 固定每行5个
    int digitW = 42;                  // 稍小
    int digitH = 36;
    int digitPadX = 4;
    int digitPadY = 4;
    int digitStartY = 70;

    // 根据屏幕宽度微调数字按钮尺寸
    if (width < 360) {
        digitW = 36;
        digitH = 30;
    }

    for (int i = 0; i < 10; ++i) {
        int row = i / digitCols;
        int col = i % digitCols;
        digitButtons[i]->width  = digitW;
        digitButtons[i]->height = digitH;
        digitButtons[i]->x = (width - (digitW * digitCols + digitPadX * (digitCols-1))) / 2 + col * (digitW + digitPadX);
        digitButtons[i]->y = digitStartY + row * (digitH + digitPadY);
    }

    // ---------- 额外按钮网格 ----------
    int extraCols = 4;
    if (width < 600) extraCols = 3;
    if (width < 400) extraCols = 2;

    int extraW = 100;                 // 足够显示文字
    int extraH = 26;
    int extraPadX = 4;
    int extraPadY = 4;
    int extraStartY = digitStartY + 2 * (digitH + digitPadY) + 16;

    int totalExtras = (int)extraButtons.size();
    int gridW = extraW * extraCols + extraPadX * (extraCols - 1);
    int startX = (width - gridW) / 2;

    for (int i = 0; i < totalExtras; ++i) {
        int row = i / extraCols;
        int col = i % extraCols;
        extraButtons[i]->width  = extraW;
        extraButtons[i]->height = extraH;
        extraButtons[i]->x = startX + col * (extraW + extraPadX);
        extraButtons[i]->y = extraStartY + row * (extraH + extraPadY);
    }

    // ---------- 关闭按钮 ----------
    Button* closeBtn = nullptr;
    for (auto* b : buttons) {
        if (b->id == 99) { closeBtn = b; break; }
    }
    if (closeBtn) {
        closeBtn->width  = 120;
        closeBtn->height = 30;
        int lastRow = (totalExtras > 0) ? (totalExtras - 1) / extraCols : 0;
        int lastY = extraStartY + lastRow * (extraH + extraPadY) + extraH;
        closeBtn->x = (width - closeBtn->width) / 2;
        closeBtn->y = lastY + 12;
    }

    // ---------- 高度溢出检查，整体缩放 ----------
    int totalHeight = closeBtn ? (closeBtn->y + closeBtn->height + 10) : 0;
    if (totalHeight > height) {
        float scale = (float)(height - 30) / totalHeight;  // 留出顶部标题空间
        if (scale < 1.0f) {
            // 粗暴但有效：将所有按钮的 y 坐标和高度按比例缩放
            for (int i = 0; i < 10; ++i) {
                digitButtons[i]->y = (int)(digitStartY + (digitButtons[i]->y - digitStartY) * scale);
                digitButtons[i]->height = (int)(digitButtons[i]->height * scale);
            }
            for (size_t i = 0; i < extraButtons.size(); ++i) {
                extraButtons[i]->y = (int)(extraStartY + (extraButtons[i]->y - extraStartY) * scale);
                extraButtons[i]->height = (int)(extraButtons[i]->height * scale);
            }
            if (closeBtn) {
                closeBtn->y = (int)(extraStartY + (closeBtn->y - extraStartY) * scale);
                closeBtn->height = (int)(closeBtn->height * scale);
            }
        }
    }
}

void DebugScreen::render(int xm, int ym, float a)
{
    // 半透明背景，底层 UI 可见
    fill(0, 0, width, height, 0x30000000);
    drawCenteredString(mc->font, "Debug Panel", width / 2, 20, 0xFFFFFFFF);
    Screen::render(xm, ym, a);
}

void DebugScreen::keyPressed(int key)
{
    if (key == 27) {   // Escape
        mc->setScreen(NULL);
    }
    // 数字键仍由 Minecraft::tickInput 处理（切换调试页），面板不再拦截
}

void DebugScreen::buttonClicked(Button* button)
{
    int id = button->id;
    if (id == 99) {
        mc->setScreen(NULL);
        return;
    }

    // 数字按钮 0~9：调用 PerfRenderer 切换调试页
    if (id >= 0 && id <= 9) {
        if (mc->getPerfRenderer())
            mc->getPerfRenderer()->debugFpsMeterKeyPress(id);
        return;
    }

    executeExtraAction(id);
    if (id != ACT_OPEN_ARMOR && id != ACT_PRERENDER)
        mc->setScreen(NULL);
}

void DebugScreen::executeExtraAction(int id) {
    switch (id) {
        case ACT_HEAL_RESET:
            mc->onGraphicsReset(); mc->player->heal(100);
            break;
        case ACT_TOGGLE_GAMEMODE:
            mc->setIsCreativeMode(!mc->isCreativeMode());
            break;
        case ACT_ADVANCE_TIME:
            if (mc->level) mc->level->setTime(mc->level->getTime() + 1000);
            break;
        case ACT_OPEN_ARMOR:
            mc->setScreen(new ArmorScreen());
            break;
        case ACT_HURT_RELOAD:
            mc->textures->reloadAll(); mc->player->hurtTo(2);
            break;
        case ACT_SPAWN_MOB: {
            Mob* mob = nullptr;
            int types[] = {MobTypes::Sheep, MobTypes::Pig, MobTypes::Chicken, MobTypes::Cow};
            mob = MobFactory::CreateMob(types[Mth::random(4)], mc->level);
            float dx = 4 - 8 * Mth::random() + 4 * Mth::sin(Mth::DEGRAD * mc->player->yRot);
            float dz = 4 - 8 * Mth::random() + 4 * Mth::cos(Mth::DEGRAD * mc->player->yRot);
            if (mob && !MobSpawner::addMob(mc->level, mob, mc->player->x + dx, mc->player->y, mc->player->z + dz,
                                           Mth::random() * 360, 0, true))
                delete mob;
            break;
        }
        case ACT_MASSACRE: {
            const EntityList& entities = mc->level->getAllEntities();
            for (int i = entities.size() - 1; i >= 0; --i) {
                Entity* e = entities[i];
                if (!e->isPlayer()) mc->level->removeEntity(e);
            }
            break;
        }
        case ACT_REFILL_INV:
            mc->player->inventory->clearInventoryWithDefault();
            break;
        case ACT_PRERENDER:
            mc->setScreen(new PrerenderTilesScreen());
            break;
        case ACT_DROP_INV:
            for (int i = Inventory::MAX_SELECTION_SIZE; i < mc->player->inventory->getContainerSize(); ++i)
                if (mc->player->inventory->getItem(i))
                    mc->player->inventory->dropSlot(i, false);
            break;
        case ACT_TOGGLE_DIFFICULTY: {
            Difficulty diff = (Difficulty)mc->options.getIntValue(OPTIONS_DIFFICULTY);
            diff = (diff == Difficulty::PEACEFUL) ? Difficulty::NORMAL : Difficulty::PEACEFUL;
            mc->options.set(OPTIONS_DIFFICULTY, diff);
            mc->level->difficulty = diff;
            break;
        }
        case ACT_TOGGLE_3RDPERSON:
            mc->options.toggle(OPTIONS_THIRD_PERSON_VIEW);
            break;
        case ACT_SPEEDUP:
            for (int i = 0; i < 5 * SharedConstants::TicksPerSecond; ++i)
                mc->level->tick();
            break;
        case ACT_NOPVP: {
            auto& as = mc->level->adventureSettings;
            as.noPvP = !as.noPvP;
            AdventureSettingsPacket p(as); mc->raknetInstance->send(p);
            break;
        }
        case ACT_NOPVM: {
            auto& as = mc->level->adventureSettings;
            as.noPvM = !as.noPvM;
            AdventureSettingsPacket p(as); mc->raknetInstance->send(p);
            break;
        }
        case ACT_NOMVP: {
            auto& as = mc->level->adventureSettings;
            as.noMvP = !as.noMvP;
            AdventureSettingsPacket p(as); mc->raknetInstance->send(p);
            break;
        }
        case ACT_IMMUTABLE: {
            auto& as = mc->level->adventureSettings;
            as.immutableWorld = !as.immutableWorld;
            AdventureSettingsPacket p(as); mc->raknetInstance->send(p);
            break;
        }
        case ACT_NAMETAGS: {
            auto& as = mc->level->adventureSettings;
            as.showNameTags = !as.showNameTags;
            AdventureSettingsPacket p(as); mc->raknetInstance->send(p);
            break;
        }
        case ACT_PARTICLES: {
            Level* lvl = mc->level;
            if (!lvl) return;
            float px = mc->player->x, py = mc->player->y, pz = mc->player->z;
            for (int i = 0; i < 50; ++i) {
                lvl->addParticle("explode", px, py + 1.0f, pz,
                                0.02f * (rand() % 100 - 50), 0.02f * (rand() % 100),
                                0.02f * (rand() % 100 - 50));
                lvl->addParticle("largesmoke", px, py + 1.0f, pz,
                                0.04f * (rand() % 100 - 50), 0.04f * (rand() % 100),
                                0.04f * (rand() % 100 - 50));
            }
            break;
        }
    }
}
