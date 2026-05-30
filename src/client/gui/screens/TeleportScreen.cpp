#include "TeleportScreen.h"
#include "../../Minecraft.h"
#include "../../player/LocalPlayer.h"
#include "../Font.h"
#include "../../../platform/input/Keyboard.h"
#include "../../../world/level/Level.h"
#include <sstream>
#include <sstream>
#include <string>
#include <cstdlib>   // 如果需要 std::stod

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
    } else if (textBox) {
        textBox->keyPressed(minecraft, key);
    }
    // 不再拦截 KEY_ESCAPE，由系统处理
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

    // 按空格分割
    std::stringstream ss(input);
    std::vector<std::string> parts;
    std::string part;
    while (ss >> part) {
        parts.push_back(part);
    }
    if (parts.size() < 3) return;  // 至少需要 X Y Z

    LocalPlayer* player = minecraft->player;
    if (!player) return;

    double x = player->x;
    double y = player->y;
    double z = player->z;

    // 解析 X
    if (parts[0][0] == '~') {
        double off = 0.0;
        if (parts[0].length() > 1)
            off = std::stod(parts[0].substr(1));
        x += off;
    } else {
        x = std::stod(parts[0]);
    }

    // 解析 Y
    if (parts[1][0] == '~') {
        double off = 0.0;
        if (parts[1].length() > 1)
            off = std::stod(parts[1].substr(1));
        y += off;
    } else {
        y = std::stod(parts[1]);
    }

    // 解析 Z
    if (parts[2][0] == '~') {
        double off = 0.0;
        if (parts[2].length() > 1)
            off = std::stod(parts[2].substr(1));
        z += off;
    } else {
        z = std::stod(parts[2]);
    }

    // 传送玩家
    player->setPos(x, y, z);
    player->xOld = player->x;
    player->yOld = player->y;
    player->zOld = player->z;
}
