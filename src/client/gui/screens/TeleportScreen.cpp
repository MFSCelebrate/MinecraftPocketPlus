#include "TeleportScreen.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../Font.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../world/level/Level.h"
#include <sstream>

TeleportScreen::TeleportScreen(Minecraft* minecraft)
    : minecraft(minecraft), textBox(nullptr) {}

TeleportScreen::~TeleportScreen() {
    if (textBox) delete textBox;
}

void TeleportScreen::init() {
    int boxX = width / 2 - 100;
    int boxY = height / 2 - 20;
    textBox = new TextBox(0, boxX, boxY, 200, 20, "");
}

void TeleportScreen::render(int xm, int ym, float a) {
    fill(0, 0, width, height, 0x80000000);
    minecraft->font->draw("Enter coordinates (X Y Z):",
                          width / 2 - 100, height / 2 - 50, 0xffffff);
    if (textBox) textBox->render(minecraft, xm, ym);
}

void TeleportScreen::tick() {
    if (textBox) textBox->tick(minecraft);
}

void TeleportScreen::keyPressed(int key) {
    if (key == Keyboard::KEY_RETURN) {
        teleport();
        minecraft->setScreen(nullptr);
    } else if (key == Keyboard::KEY_ESCAPE) {
        minecraft->setScreen(nullptr);
    } else if (textBox) {
        textBox->keyPressed(minecraft, key);
    }
}

void TeleportScreen::charPressed(char inputChar) {
    if (textBox) textBox->charPressed(minecraft, inputChar);
}

void TeleportScreen::mouseClicked(int x, int y, int button) {
    if (textBox) textBox->mouseClicked(minecraft, x, y, button);
}

void TeleportScreen::teleport() {
    std::string input = textBox->text;
    if (input.empty()) return;

    std::stringstream ss(input);
    float x, y, z;
    if (ss >> x >> y >> z) {
        LocalPlayer* player = minecraft->player;
        Level* level = minecraft->level;
        if (player && level) {
            // 获取世界出生点偏移（显示坐标转世界坐标）
            Pos spawnPos = level->getSharedSpawnPos();
            float worldX = x + spawnPos.x;
            float worldY = y + spawnPos.y;
            float worldZ = z + spawnPos.z;
            player->setPos(worldX, worldY, worldZ);
            // 强制同步旧位置，避免插值错误
            player->xOld = player->x;
            player->yOld = player->y;
            player->zOld = player->z;
        }
    }
}
