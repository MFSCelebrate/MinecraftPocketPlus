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
#include "../gui/Gui.h"

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
    // ========== 数字按钮 0-9 ==========
    for (int i = 0; i < 10; ++i)
        addDigitButton(i);

    // ========== 额外调试功能按钮 ==========
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
    if (width <= 0 && mc && mc->width > 0 && Gui::InvGuiScale > 0)
        width = (int)(mc->width * Gui::InvGuiScale);
    if (height <= 0 && mc && mc->height > 0 && Gui::InvGuiScale > 0)
        height = (int)(mc->height * Gui::InvGuiScale);

    const int btnW = 50;
    const int btnH = 50;
    const int pad = 6;
    const int startY = 80;

    // 数字按钮：两排 5个
    for (int i = 0; i < 10; ++i) {
        int row = i / 5;
        int col = i % 5;
        digitButtons[i]->width  = btnW;
        digitButtons[i]->height = btnH;
        digitButtons[i]->x = (width - (btnW * 5 + pad * 4)) / 2 + col * (btnW + pad);
        digitButtons[i]->y = startY + row * (btnH + pad);
    }

    // 额外按钮：自适应网格
    int extraStartY = startY + 2 * (btnH + pad) + 20;
    int cols = 4;
    if (width < 650) cols = 3;
    if (width < 450) cols = 2;
    int extraW = 110;
    int extraH = 28;
    int gridW = extraW * cols + pad * (cols - 1);
    int startX = (width - gridW) / 2;

    for (size_t i = 0; i < extraButtons.size(); ++i) {
        int row = (int)i / cols;
        int col = (int)i % cols;
        extraButtons[i]->width  = extraW;
        extraButtons[i]->height = extraH;
        extraButtons[i]->x = startX + col * (extraW + pad);
        extraButtons[i]->y = extraStartY + row * (extraH + pad);
    }

    // 关闭按钮
    Button* closeBtn = nullptr;
    for (auto* b : buttons) {
        if (b->id == 99) { closeBtn = b; break; }
    }
    if (closeBtn) {
        closeBtn->width  = 120;
        closeBtn->height = 30;
        int lastRow = (extraButtons.size() > 0) ? (int)(extraButtons.size() - 1) / cols : 0;
        int lastY = extraStartY + lastRow * (extraH + pad) + extraH;
        closeBtn->x = (width - closeBtn->width) / 2;
        closeBtn->y = lastY + 16;
    }
}

void DebugScreen::render(int xm, int ym, float a)
{
    fill(0, 0, width, height, 0x30000000); // 半透明背景，底层UI可见
    drawCenteredString(mc->font, "Debug Panel", width / 2, 20, 0xFFFFFFFF);
    Screen::render(xm, ym, a);
}

void DebugScreen::keyPressed(int key)
{
    if (key == 27) {   // Escape
        mc->setScreen(NULL);
    }
    // 数字键不处理，留给 Minecraft::tickInput 切换调试页
}

void DebugScreen::buttonClicked(Button* button)
{
    int id = button->id;
    if (id == 99) {
        mc->setScreen(NULL);
        return;
    }

    // 数字按钮：切换 PerfRenderer 调试页
    if (id >= 0 && id <= 9) {
        if (mc->getPerfRenderer())
            mc->getPerfRenderer()->debugFpsMeterKeyPress(id);
        return;
    }

    // 其他额外功能按钮
    executeExtraAction(id);
    // 除了打开子界面的按钮，其余关闭面板
    if (id != ACT_OPEN_ARMOR && id != ACT_PRERENDER)
        mc->setScreen(NULL);
}

// ---------- 额外功能实现 ----------
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
