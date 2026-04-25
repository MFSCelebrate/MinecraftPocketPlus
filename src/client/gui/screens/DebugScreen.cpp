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

DebugScreen::DebugScreen(Minecraft* mc)
    : mc(mc), scrollY(0.0f), maxScroll(0.0f), lastTouchY(0.0f), dragging(false),
      contentHeight(0), viewportHeight(0)
{
}

DebugScreen::~DebugScreen()
{
    for (auto* b : debugButtons) delete b;
}

void DebugScreen::init()
{
    // 12 个调试按钮
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

    // 关闭按钮
    Button* closeBtn = new Button(99, "Close");
    closeBtn->width = 120;
    closeBtn->height = 30;
    buttons.push_back(closeBtn);

    // 让屏幕接收所有输入事件
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

void DebugScreen::setupPositions()
{
    const int buttonPadding = 4;
    const int btnH = 30;
    // 按钮宽度取屏幕宽度的 70%，最大 400 像素
    int btnW = (int)(width * 0.7f);
    if (btnW > 400) btnW = 400;
    int startX = (width - btnW) / 2;

    // 为所有按钮统一设置大小，Y 坐标从 0 开始（后续通过滚动偏移绘制）
    for (size_t i = 0; i < buttons.size(); ++i)
    {
        buttons[i]->x = startX;
        buttons[i]->y = (int)(i * (btnH + buttonPadding)); // 逻辑位置，滚动偏移在 render 中处理
        buttons[i]->width = btnW;
        buttons[i]->height = btnH;
    }

    // 计算内容总高度及可滚动范围
    contentHeight = (int)(buttons.size() * (btnH + buttonPadding) - buttonPadding);
    viewportHeight = height - 50;  // 顶部留标题 30，底部留 20

    // 强制从顶部开始显示
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

void DebugScreen::tick()
{
    // 拖拽惯性已经在 mouseEvent 中直接更新 scrollY，这里不用额外处理
}

// ---------- 触摸 / 鼠标滚动支持 ----------
void DebugScreen::mouseClicked(int x, int y, int buttonNum)
{
    if (buttonNum == MouseAction::ACTION_LEFT)
    {
        // 先检查是否点在按钮上（按钮坐标需要加上滚动偏移）
        for (auto* btn : buttons)
        {
            if (btn->active &&
                x >= btn->x && x < btn->x + btn->width &&
                y >= (btn->y - scrollY) && y < (btn->y - scrollY) + btn->height)
            {
                // 按钮点击交给父类处理
                Screen::mouseClicked(x, y, buttonNum);
                dragging = false;
                return;
            }
        }
        // 否则开始拖拽
        dragging = true;
        lastTouchY = (float)y + scrollY; // 记录触摸点对应的内容坐标
    }
}

void DebugScreen::mouseReleased(int x, int y, int buttonNum)
{
    if (dragging)
    {
        dragging = false;
        return;
    }
    Screen::mouseReleased(x, y, buttonNum);
}

void DebugScreen::mouseEvent()
{
    // 优先处理按钮 click，否则滚动
    Screen::mouseEvent();
    if (dragging)
    {
        const MouseAction& e = Mouse::getEvent();
        if (e.action == MouseAction::ACTION_MOVE)
        {
            float touchY = (float)e.y + scrollY; // 因为 scrollY 还未更新，需保持和 mouseClicked 相同基准
            float delta = lastTouchY - touchY;
            scrollY += delta;
            lastTouchY = touchY;
            updateScrollLimits();
        }
    }
}

// ---------- 渲染（裁剪区域） ----------
void DebugScreen::render(int xm, int ym, float a)
{
    // 半透明背景
    fill(0, 0, width, height, 0x80000000);

    // 标题（固定位置）
    drawCenteredString(mc->font, "Debug Panel", width / 2, 10, 0xFFFFFFFF);

    // 裁剪区域（修正的 scissor）
    glEnable2(GL_SCISSOR_TEST);
    int clipX = 10;
    int clipY = 30;                       // 上边距
    int clipW = width - 20;               // 左右各留 10
    int clipH = height - clipY - 20;      // 下边距 20，所以高度 = height - 50
    glScissor(
        Gui::GuiScale * clipX,
        mc->height - Gui::GuiScale * (clipY + clipH),
        Gui::GuiScale * clipW,
        Gui::GuiScale * clipH
    );

    // 绘制所有按钮，Y 坐标减去 scrollY
    glPushMatrix();
    glTranslatef(0, -scrollY, 0);
    Screen::render(xm, ym + (int)scrollY, a); // 调整鼠标坐标，保证按钮点击正常
    glPopMatrix();
    glDisable2(GL_SCISSOR_TEST);
}

void DebugScreen::buttonClicked(Button* button)
{
    if (button->id == 99)
    {
        mc->setScreen(NULL);
        return;
    }
    executeAction(button->id);
    if (button->id != BTN_ARMOR && button->id != BTN_PRERENDER)
    {
        mc->setScreen(NULL);
    }
}

// ---------- 12 个调试功能（保持不变） ----------
void DebugScreen::executeAction(int id)
{
    switch (id)
    {
    case BTN_GODMODE:
        mc->onGraphicsReset();
        mc->player->heal(100);
        break;
    case BTN_GAMEMODE:
        mc->setIsCreativeMode(!mc->isCreativeMode());
        break;
    case BTN_TIME:
        if (mc->level)
            mc->level->setTime(mc->level->getTime() + 1000);
        break;
    case BTN_ARMOR:
        mc->setScreen(new ArmorScreen());
        break;
    case BTN_HURT_RELOAD:
        mc->textures->reloadAll();
        mc->player->hurtTo(2);
        break;
    case BTN_SPAWNMOB:
    {
        Mob* mob = nullptr;
        int types[] = {MobTypes::Sheep, MobTypes::Pig, MobTypes::Chicken, MobTypes::Cow};
        int mobType = types[Mth::random(4)];
        mob = MobFactory::CreateMob(mobType, mc->level);
        float dx = 4 - 8 * Mth::random() + 4 * Mth::sin(Mth::DEGRAD * mc->player->yRot);
        float dz = 4 - 8 * Mth::random() + 4 * Mth::cos(Mth::DEGRAD * mc->player->yRot);
        if (mob && !MobSpawner::addMob(mc->level, mob, mc->player->x + dx, mc->player->y, mc->player->z + dz, Mth::random() * 360, 0, true))
            delete mob;
        break;
    }
    case BTN_MASSACRE:
    {
        const EntityList& entities = mc->level->getAllEntities();
        for (int i = entities.size() - 1; i >= 0; --i)
        {
            Entity* e = entities[i];
            if (!e->isPlayer())
                mc->level->removeEntity(e);
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
