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

DebugScreen::DebugScreen(Minecraft* mc)
    : mc(mc)
{
}

DebugScreen::~DebugScreen()
{
    for (auto* b : debugButtons) delete b;
}

void DebugScreen::init()
{
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
    buttons.push_back(closeBtn);

    passEvents = false;
}

void DebugScreen::addButton(int id, const std::string& text)
{
    Button* btn = new Button(id, text);
    buttons.push_back(btn);
    debugButtons.push_back(btn);
}

void DebugScreen::setupPositions()
{
    // 直接使用从 Screen::init 传入的 width 和 height（此时应已正确初始化）
    // 若宽度无效，从 mc 实例计算，避免垃圾值
    if (width <= 0 && mc && mc->width > 0 && Gui::InvGuiScale > 0)
        width = (int)(mc->width * Gui::InvGuiScale);
    if (height <= 0 && mc && mc->height > 0 && Gui::InvGuiScale > 0)
        height = (int)(mc->height * Gui::InvGuiScale);

    const int totalBtns = (int)debugButtons.size();
    const int btnW = 100;
    const int btnH = 24;
    const int pad  = 4;

    // 分列
    int cols = 5;
    if (width < 600) cols = 4;
    if (width < 400) cols = 3;
    // 计算右边距，若超出则缩列
    while (cols > 1 && (btnW * cols + pad * (cols - 1)) > (width - 20)) --cols;

    int gridW = btnW * cols + pad * (cols - 1);
    int startX = (width - gridW) / 2;
    int startY = 35;   // 标题下方

    for (size_t i = 0; i < debugButtons.size(); ++i) {
        int row = (int)i / cols;
        int col = (int)i % cols;
        debugButtons[i]->x = startX + col * (btnW + pad);
        debugButtons[i]->y = startY + row * (btnH + pad);
        debugButtons[i]->width = btnW;
        debugButtons[i]->height = btnH;
    }

    // 关闭按钮居中
    Button* closeBtn = nullptr;
    for (auto* b : buttons) {
        if (b->id == 99) { closeBtn = b; break; }
    }
    if (closeBtn) {
        closeBtn->width = 120;
        closeBtn->height = 30;
        closeBtn->x = (width - closeBtn->width) / 2;
        int lastRow = (totalBtns - 1) / cols;
        int lastY = startY + lastRow * (btnH + pad) + btnH;
        closeBtn->y = lastY + 20;
    }
}

void DebugScreen::render(int xm, int ym, float a)
{
    // 背景
    fill(0, 0, width, height, 0x80000000);
    drawCenteredString(mc->font, "Debug Panel", width / 2, 8, 0xFFFFFFFF);

    // 直接使用基类渲染按钮（不进行任何鼠标坐标修正，基类会处理）
    Screen::render(xm, ym, a);
}

void DebugScreen::buttonClicked(Button* button)
{
    if (button->id == 99) {
        mc->setScreen(NULL);
        return;
    }
    executeAction(button->id);
    if (button->id != BTN_ARMOR && button->id != BTN_PRERENDER)
        mc->setScreen(NULL);
}

// ---- 功能实现（完整，已压缩但不变） ----
void DebugScreen::executeAction(int id)
{
    switch (id)
    {
    case BTN_GODMODE:
        mc->onGraphicsReset(); mc->player->heal(100); break;
    case BTN_GAMEMODE:
        mc->setIsCreativeMode(!mc->isCreativeMode()); break;
    case BTN_TIME:
        if (mc->level) mc->level->setTime(mc->level->getTime() + 1000); break;
    case BTN_ARMOR:
        mc->setScreen(new ArmorScreen()); break;
    case BTN_HURT_RELOAD:
        mc->textures->reloadAll(); mc->player->hurtTo(2); break;
    case BTN_SPAWNMOB:
    {
        Mob* mob = nullptr;
        int types[] = {MobTypes::Sheep, MobTypes::Pig, MobTypes::Chicken, MobTypes::Cow};
        int mobType = types[Mth::random(4)];
        mob = MobFactory::CreateMob(mobType, mc->level);
        float dx = 4 - 8 * Mth::random() + 4 * Mth::sin(Mth::DEGRAD * mc->player->yRot);
        float dz = 4 - 8 * Mth::random() + 4 * Mth::cos(Mth::DEGRAD * mc->player->yRot);
        if (mob && !MobSpawner::addMob(mc->level, mob, mc->player->x + dx, mc->player->y, mc->player->z + dz,
                                       Mth::random() * 360, 0, true))
            delete mob;
        break;
    }
    case BTN_MASSACRE:
    {
        const EntityList& entities = mc->level->getAllEntities();
        for (int i = entities.size() - 1; i >= 0; --i) {
            Entity* e = entities[i];
            if (!e->isPlayer()) mc->level->removeEntity(e);
        }
        break;
    }
    case BTN_CLEARINV:
        mc->player->inventory->clearInventoryWithDefault(); break;
    case BTN_PRERENDER:
        mc->setScreen(new PrerenderTilesScreen()); break;
    case BTN_DROPALL:
        for (int i = Inventory::MAX_SELECTION_SIZE; i < mc->player->inventory->getContainerSize(); ++i)
            if (mc->player->inventory->getItem(i)) mc->player->inventory->dropSlot(i, false);
        break;
    case BTN_SPEEDUP:
        for (int i = 0; i < 5 * SharedConstants::TicksPerSecond; ++i) mc->level->tick();
        break;
    case BTN_3RDPERSON:
        mc->options.toggle(OPTIONS_THIRD_PERSON_VIEW); break;

    case BTN_NOPVP: {
        auto& as = mc->level->adventureSettings; as.noPvP = !as.noPvP;
        AdventureSettingsPacket p(as); mc->raknetInstance->send(p); break;
    }
    case BTN_NOPVM: {
        auto& as = mc->level->adventureSettings; as.noPvM = !as.noPvM;
        AdventureSettingsPacket p(as); mc->raknetInstance->send(p); break;
    }
    case BTN_NOMVP: {
        auto& as = mc->level->adventureSettings; as.noMvP = !as.noMvP;
        AdventureSettingsPacket p(as); mc->raknetInstance->send(p); break;
    }
    case BTN_IMMUTABLE: {
        auto& as = mc->level->adventureSettings; as.immutableWorld = !as.immutableWorld;
        AdventureSettingsPacket p(as); mc->raknetInstance->send(p); break;
    }
    case BTN_NAMETAGS: {
        auto& as = mc->level->adventureSettings; as.showNameTags = !as.showNameTags;
        AdventureSettingsPacket p(as); mc->raknetInstance->send(p); break;
    }
    case BTN_PARTICLES: {
        Level* lvl = mc->level; if (!lvl) break;
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
