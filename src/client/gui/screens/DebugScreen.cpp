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
#include "../../../network/packet/AdventureSettingsPacket.h"
#include "../../../network/RakNetInstance.h"
#include "../../sound/SoundEngine.h"
#include <cmath>

DebugScreen::DebugScreen(Minecraft* mc)
    : mc(mc), scrollY(0.0f), maxScroll(0.0f), lastTouchY(0.0f), dragging(false),
      contentHeight(0), viewportHeight(0), _pressedButton(nullptr)
{
}

DebugScreen::~DebugScreen()
{
    for (auto* b : debugButtons) delete b;
}

// ---------- 初始化 ----------
void DebugScreen::init()
{
    // 12 原有按钮
    addButton(BTN_GODMODE,      "God Mode");
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

    // 6 新增按钮
    addButton(BTN_NOPVP,        "NoPvP Toggle");
    addButton(BTN_NOPVM,        "NoPvM Toggle");
    addButton(BTN_NOMVP,        "NoMvP Toggle");
    addButton(BTN_IMMUTABLE,    "Immutable Toggle");
    addButton(BTN_NAMETAGS,     "NameTags Toggle");
    addButton(BTN_PARTICLES,    "Test Particles");

    // 关闭按钮
    Button* closeBtn = new Button(99, "Close");
    closeBtn->width = 120;
    closeBtn->height = 30;
    buttons.push_back(closeBtn);

    passEvents = false;
}

void DebugScreen::addButton(int id, const std::string& text)
{
    Button* btn = new Button(id, text);
    btn->width = 120;
    btn->height = 30;
    buttons.push_back(btn);
    debugButtons.push_back(btn);
}

// ---------- 布局 ----------
void DebugScreen::setupPositions()
{
    const int buttonPadding = 4;
    const int btnH = 30;
    int btnW = (int)(width * 0.7f);
    if (btnW > 400) btnW = 400;
    int startX = (width - btnW) / 2;

    for (size_t i = 0; i < buttons.size(); ++i)
    {
        buttons[i]->x = startX;
        buttons[i]->y = 30 + (int)(i * (btnH + buttonPadding));
        buttons[i]->width = btnW;
        buttons[i]->height = btnH;
    }

    contentHeight = (int)(buttons.size() * (btnH + buttonPadding) - buttonPadding);
    viewportHeight = height - 50;       // 标题 30，下边距 20

    // 强制从顶部开始
    scrollY = 0.0f;
    maxScroll = contentHeight - viewportHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollY > maxScroll) scrollY = maxScroll;
    if (scrollY < 0) scrollY = 0;
}

void DebugScreen::updateScrollLimits()
{
    maxScroll = contentHeight - viewportHeight;
    if (maxScroll < 0) maxScroll = 0;
    if (scrollY > maxScroll) scrollY = maxScroll;
    if (scrollY < 0) scrollY = 0;
}

// ---------- Tick：专用于拖拽滚动 ----------
void DebugScreen::tick()
{
    if (!dragging) return;

    int x = Mouse::getX();
    int y = Mouse::getY();
    toGUICoordinate(x, y);                       // 屏幕 → Gui 坐标

    float currentLogicY = (float)y + scrollY;   // 当前触摸点在内容空间中的 Y
    float delta = lastTouchY - currentLogicY;   // 手指上推 delta 为正
    scrollY += delta;
    lastTouchY = currentLogicY;

    updateScrollLimits();
}

// ---------- 鼠标按下 ----------
void DebugScreen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum != MouseAction::ACTION_LEFT) return;

    toGUICoordinate(x, y);                       // 屏幕 → Gui 坐标
    int logicalY = y + (int)scrollY;            // 转换为按钮列表中的逻辑 Y

    for (auto* btn : buttons)
    {
        if (btn->active &&
            x >= btn->x && x < btn->x + btn->width &&
            logicalY >= btn->y && logicalY < btn->y + btn->height)
        {
            _pressedButton = btn;
            _pressedButton->setPressed();
            return;                              // 千万别漏掉，保证不启动拖拽
        }
    }

    // 没命中按钮 → 开始拖拽
    dragging = true;
    lastTouchY = (float)(y + scrollY);          // 记录内容坐标中的触摸起点
}

// ---------- 鼠标释放 ----------
void DebugScreen::mouseReleased(int x, int y, int buttonNum)
{
    if (buttonNum != MouseAction::ACTION_LEFT) return;

    toGUICoordinate(x, y);

    if (dragging)
    {
        dragging = false;
        return;
    }

    if (_pressedButton)
    {
        int logicalY = y + (int)scrollY;
        if (_pressedButton->active &&
            x >= _pressedButton->x && x < _pressedButton->x + _pressedButton->width &&
            logicalY >= _pressedButton->y && logicalY < _pressedButton->y + _pressedButton->height)
        {
            buttonClicked(_pressedButton);
            mc->soundEngine->playUI("random.click", 1, 1);
        }
        _pressedButton->released(x, y);
        _pressedButton = nullptr;
    }
}

// ---------- 渲染 ----------
void DebugScreen::render(int xm, int ym, float a)
{
    fill(0, 0, width, height, 0x80000000);
    drawCenteredString(mc->font, "Debug Panel", width / 2, 10, 0xFFFFFFFF);

    // 裁剪区域（上边距 30，左右各留 10，下边距 20）
    glEnable2(GL_SCISSOR_TEST);
    int clipX = 10;
    int clipY = 30;
    int clipW = width - 20;
    int clipH = height - clipY - 20;
    glScissor(
        Gui::GuiScale * clipX,
        mc->height - Gui::GuiScale * (clipY + clipH),
        Gui::GuiScale * clipW,
        Gui::GuiScale * clipH
    );

    // 平移内容
    glPushMatrix();
    glTranslatef(0, -scrollY, 0);
    // 注意：传给基类的鼠标坐标不需要加上 scrollY，因为基类只是用来画按钮，不执行点击逻辑
    Screen::render(xm, ym, a);
    glPopMatrix();

    glDisable2(GL_SCISSOR_TEST);
}

void DebugScreen::buttonClicked(Button* button)
{
    if (button->id == 99) { mc->setScreen(NULL); return; }
    executeAction(button->id);
    if (button->id != BTN_ARMOR && button->id != BTN_PRERENDER)
        mc->setScreen(NULL);
}

// ---------- 功能实现（含发包） ----------
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
