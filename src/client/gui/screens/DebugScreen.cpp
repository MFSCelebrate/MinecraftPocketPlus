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
// 不再使用 DebugLog 和任何日志，避免初始化顺序问题

// 固定缩放因子：1 逻辑单位 = 3 物理像素
#define PIXEL_SCALE 3

DebugScreen::DebugScreen(Minecraft* mc)
    : mc(mc), _pressedButton(nullptr), columns(0), btnWidth(0), btnHeight(0)
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
    // 完全不依赖 width/height，使用固定物理尺寸，在 render 里动态计算
    // 这里仅占位，实际计算放在 render 中确保安全
}

void DebugScreen::render(int xm, int ym, float a)
{
    // 动态获取物理屏幕大小（相信此时已初始化）
    int physW = 854, physH = 480;
    if (mc && mc->width > 0 && mc->height > 0) {
        physW = mc->width;
        physH = mc->height;
    }

    // 用固定缩放因子转换为逻辑坐标
    int logicW = physW / PIXEL_SCALE;
    int logicH = physH / PIXEL_SCALE;

    // 更新基类 width/height（渲染背景需要）
    width = logicW;
    height = logicH;

    // 绘制半透明背景
    fill(0, 0, width, height, 0x80000000);
    drawCenteredString(mc->font, "Debug Panel", width / 2, 8, 0xFFFFFFFF);

    // 动态计算按钮布局
    const int totalBtns = (int)debugButtons.size();
    btnWidth = 80;            // 逻辑单位
    btnHeight = 20;
    const int padding = 4;

    int maxColumns = (logicW - 20) / (btnWidth + padding);
    if (maxColumns < 1) maxColumns = 1;

    if (totalBtns <= 6) columns = 3;
    else if (totalBtns <= 12) columns = 4;
    else columns = 5;
    if (columns > maxColumns) columns = maxColumns;

    int gridW = columns * btnWidth + (columns - 1) * padding;
    int startX = (logicW - gridW) / 2;
    int startY = 30;

    // 设置每个功能按钮的位置（逻辑坐标）
    for (size_t i = 0; i < debugButtons.size(); ++i) {
        int row = (int)i / columns;
        int col = (int)i % columns;
        debugButtons[i]->x = startX + col * (btnWidth + padding);
        debugButtons[i]->y = startY + row * (btnHeight + padding);
        debugButtons[i]->width = btnWidth;
        debugButtons[i]->height = btnHeight;
    }

    // 关闭按钮
    Button* closeBtn = nullptr;
    for (auto* b : buttons) {
        if (b->id == 99) { closeBtn = b; break; }
    }
    if (closeBtn) {
        closeBtn->width = 120;
        closeBtn->height = 30;
        closeBtn->x = (logicW - closeBtn->width) / 2;
        int lastRow = (totalBtns - 1) / columns;
        int lastY = startY + lastRow * (btnHeight + padding) + btnHeight;
        closeBtn->y = lastY + 20;
    }

    // 现在按钮坐标已经按逻辑坐标设定好，调用基类渲染（基类会用按钮的逻辑坐标绘制）
    // 必须确保 render 传入的 xm/ym 也使用逻辑坐标
    // 将物理鼠标坐标转换为逻辑坐标
    int logicXM = xm / PIXEL_SCALE;
    int logicYM = ym / PIXEL_SCALE;
    Screen::render(logicXM, logicYM, a);
}

void DebugScreen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum != MouseAction::ACTION_LEFT) return;

    // 物理坐标转逻辑坐标（与 render 中的转换一致）
    int logicalX = x / PIXEL_SCALE;
    int logicalY = y / PIXEL_SCALE;

    for (auto* btn : buttons) {
        if (btn->active &&
            logicalX >= btn->x && logicalX < btn->x + btn->width &&
            logicalY >= btn->y && logicalY < btn->y + btn->height)
        {
            _pressedButton = btn;
            _pressedButton->setPressed();
            return;
        }
    }
}

void DebugScreen::mouseReleased(int x, int y, int buttonNum)
{
    if (buttonNum != MouseAction::ACTION_LEFT) return;

    int logicalX = x / PIXEL_SCALE;
    int logicalY = y / PIXEL_SCALE;

    if (_pressedButton) {
        if (_pressedButton->active &&
            logicalX >= _pressedButton->x && logicalX < _pressedButton->x + _pressedButton->width &&
            logicalY >= _pressedButton->y && logicalY < _pressedButton->y + _pressedButton->height)
        {
            buttonClicked(_pressedButton);
            mc->soundEngine->playUI("random.click", 1, 1);
        }
        _pressedButton->released(logicalX, logicalY);
        _pressedButton = nullptr;
    }
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
