#include "TeleportButton.h"
#include "../screens/TeleportScreen.h"

TeleportButton::TeleportButton(int id, Minecraft* minecraft)
    : Touch::TButton(id, "Teleport"), minecraft(minecraft) {}

TeleportButton::~TeleportButton() {}

void TeleportButton::mouseClicked(Minecraft* minecraft, int x, int y, int button) {
    // 确保是同一个实例，避免错误
    if (this->minecraft == minecraft) {
        this->minecraft->setScreen(new TeleportScreen(this->minecraft));
    }
}
