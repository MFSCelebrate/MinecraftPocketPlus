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
    if (textBox) textBox->render(xm, ym, a);
}

void TeleportScreen::tick() {
    if (textBox) textBox->tick();
}

void TeleportScreen::keyPressed(int key) {
    if (key == Keyboard::KEY_RETURN) {
        teleport();
        minecraft->setScreen(nullptr);
    } else if (key == Keyboard::KEY_ESCAPE) {
        minecraft->setScreen(nullptr);
    } else if (textBox) {
        textBox->keyPressed(key);
    }
}

void TeleportScreen::charPressed(char inputChar) {
    if (textBox) textBox->charPressed(inputChar);
}

void TeleportScreen::mouseClicked(int x, int y, int button) {
    if (textBox) textBox->mouseClicked(x, y, button);
}

void TeleportScreen::teleport() {
    std::string input = textBox->getText();
    if (input.empty()) return;

    std::stringstream ss(input);
    float x, y, z;
    if (ss >> x >> y >> z) {
        LocalPlayer* player = minecraft->player;
        if (player) {
            player->setPos(x, y, z);
            player->resetPos();      // 刷新碰撞箱，避免卡在方块中
        }
    }
}
