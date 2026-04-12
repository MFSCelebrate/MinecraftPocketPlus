#include "OptionsScreen.h"
#include "StartMenuScreen.h"
#include "UsernameScreen.h"
#include "DialogDefinitions.h"
#include "../../Minecraft.h"
#include "../../../AppPlatform.h"
#include "CreditsScreen.h"
#include "../components/ImageButton.h"
#include "../components/OptionsGroup.h"
#include "../components/TextOption.h"
#include "../components/OptionsItem.h"
#include "platform/input/Keyboard.h"
#include <cmath>
#include <algorithm>

OptionsScreen::OptionsScreen()
	: btnClose(NULL), bHeader(NULL), btnCredits(NULL), selectedCategory(0),
	  scrollOffset(0.0f), maxScrollOffset(0.0f), scrollVelocity(0.0f),
	  lastMouseY(0.0f), isDragging(false), isScrollbarVisible(false)
{ }

OptionsScreen::~OptionsScreen() {
	if (btnClose) delete btnClose;
	if (bHeader) delete bHeader;
	if (btnCredits) delete btnCredits;
	for (auto btn : categoryButtons) delete btn;
	for (auto pane : optionPanes) delete pane;
}

void OptionsScreen::init() {
	bHeader = new Touch::THeader(0, "Options");
	btnClose = new ImageButton(1, "");
	ImageDef def;
	def.name = "gui/touchgui.png";
	def.width = 34;
	def.height = 26;
	def.setSrc(IntRectangle(150, 0, (int)def.width, (int)def.height));
	btnClose->setImageDef(def, true);

	categoryButtons.push_back(new Touch::TButton(2, "General"));
	categoryButtons.push_back(new Touch::TButton(3, "Game"));
	categoryButtons.push_back(new Touch::TButton(4, "Controls"));
	categoryButtons.push_back(new Touch::TButton(5, "Graphics"));
	categoryButtons.push_back(new Touch::TButton(6, "Tweaks"));
	categoryButtons.push_back(new Touch::TButton(7, "World"));

	btnCredits = new Touch::TButton(11, "Credits");

	buttons.push_back(bHeader);
	buttons.push_back(btnClose);
	buttons.push_back(btnCredits);
	for (auto btn : categoryButtons) {
		buttons.push_back(btn);
		tabButtons.push_back(btn);
	}

	generateOptionScreens();
	selectCategory(0);
}

void OptionsScreen::setupPositions() {
	int buttonHeight = btnClose->height;
	btnClose->x = width - btnClose->width;
	btnClose->y = 0;

	int offsetNum = 1;
	for (auto btn : categoryButtons) {
		btn->x = 0;
		btn->y = offsetNum * buttonHeight;
		btn->selected = false;
		offsetNum++;
	}

	bHeader->x = 0;
	bHeader->y = 0;
	bHeader->width = width - btnClose->width;
	bHeader->height = btnClose->height;

	if (btnCredits) {
		btnCredits->x = width - btnCredits->width;
		btnCredits->y = height - btnCredits->height;
	}

	for (auto pane : optionPanes) {
		if (!categoryButtons.empty() && categoryButtons[0]) {
			pane->x = categoryButtons[0]->width;
			pane->y = bHeader->height;
			pane->width = width - categoryButtons[0]->width;
			pane->setupPositions();
		}
	}
	updateMaxScrollOffset();
	applyScrollLimits();
}

void OptionsScreen::render(int xm, int ym, float a) {
	renderBackground();

	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;

	if (currentOptionsGroup) {
		// 1. 计算裁剪区域（屏幕坐标系，原点左上角）
		int scissorX = currentOptionsGroup->x;
		int scissorY = currentOptionsGroup->y;
		int scissorW = currentOptionsGroup->width;
		int bottomPadding = (btnCredits ? btnCredits->height + 5 : 0);
		int scissorH = height - scissorY - bottomPadding;
		if (scissorH < 0) scissorH = 0;

		// 2. 保存旧裁剪状态
		GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
		GLint oldScissor[4];
		glGetIntegerv(GL_SCISSOR_BOX, oldScissor);

		// 3. 设置新裁剪（OpenGL 原点在左下角）
		glEnable(GL_SCISSOR_TEST);
		int glY = minecraft->height - (scissorY + scissorH);  // 关键转换！
		glScissor(scissorX, glY, scissorW, scissorH);

		// 4. 应用滚动偏移并渲染
		glPushMatrix();
		glTranslatef(0.0f, -scrollOffset, 0.0f);
		// 传入的鼠标 Y 坐标也需加上滚动偏移（因为子控件坐标是绝对屏幕坐标）
		currentOptionsGroup->render(minecraft, xmm, ymm + (int)scrollOffset);
		glPopMatrix();

		// 5. 恢复裁剪状态
		if (!scissorWasEnabled) glDisable(GL_SCISSOR_TEST);
		else glScissor(oldScissor[0], oldScissor[1], oldScissor[2], oldScissor[3]);
	}

	// 渲染其他 UI（不被裁剪）
	super::render(xm, ym, a);
}

void OptionsScreen::removed() { }

void OptionsScreen::buttonClicked(Button* button) {
	if (button == btnClose) {
		minecraft->options.save();
		if (minecraft->screen)
			minecraft->setScreen(NULL);
		else
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
	} else if (button->id >= categoryButtons[0]->id && button->id <= categoryButtons.back()->id) {
		int idx = button->id - categoryButtons[0]->id;
		selectCategory(idx);
	} else if (button == btnCredits) {
		minecraft->setScreen(new CreditsScreen());
	}
}

void OptionsScreen::selectCategory(int index) {
	for (size_t i = 0; i < categoryButtons.size(); ++i)
		categoryButtons[i]->selected = (int)i == index;
	if (index < (int)optionPanes.size()) {
		currentOptionsGroup = optionPanes[index];
		scrollOffset = 0.0f;
		scrollVelocity = 0.0f;
		updateMaxScrollOffset();
	}
}

void OptionsScreen::generateOptionScreens() {
	optionPanes.push_back(new OptionsGroup("options.group.general"));
	optionPanes.push_back(new OptionsGroup("options.group.game"));
	optionPanes.push_back(new OptionsGroup("options.group.controls"));
	optionPanes.push_back(new OptionsGroup("options.group.graphics"));
	optionPanes.push_back(new OptionsGroup("options.group.tweaks"));
	optionPanes.push_back(new OptionsGroup("options.group.world"));

	// General
	optionPanes[0]->addOptionItem(OPTIONS_USERNAME, minecraft)
		.addOptionItem(OPTIONS_SENSITIVITY, minecraft);
	// Game
	optionPanes[1]->addOptionItem(OPTIONS_DIFFICULTY, minecraft)
		.addOptionItem(OPTIONS_SERVER_VISIBLE, minecraft)
		.addOptionItem(OPTIONS_THIRD_PERSON_VIEW, minecraft)
		.addOptionItem(OPTIONS_GUI_SCALE, minecraft)
		.addOptionItem(OPTIONS_SENSITIVITY, minecraft)
		.addOptionItem(OPTIONS_MUSIC_VOLUME, minecraft)
		.addOptionItem(OPTIONS_SOUND_VOLUME, minecraft)
		.addOptionItem(OPTIONS_SMOOTH_CAMERA, minecraft)
		.addOptionItem(OPTIONS_DESTROY_VIBRATION, minecraft)
		.addOptionItem(OPTIONS_IS_LEFT_HANDED, minecraft);
	// Controls
	optionPanes[2]->addOptionItem(OPTIONS_INVERT_Y_MOUSE, minecraft)
		.addOptionItem(OPTIONS_USE_TOUCHSCREEN, minecraft)
		.addOptionItem(OPTIONS_AUTOJUMP, minecraft);
	for (int i = OPTIONS_KEY_FORWARD; i <= OPTIONS_KEY_USE; ++i)
		optionPanes[2]->addOptionItem((OptionId)i, minecraft);
	// Graphics
	optionPanes[3]->addOptionItem(OPTIONS_FANCY_GRAPHICS, minecraft)
		.addOptionItem(OPTIONS_LIMIT_FRAMERATE, minecraft)
		.addOptionItem(OPTIONS_VSYNC, minecraft)
		.addOptionItem(OPTIONS_RENDER_DEBUG, minecraft)
		.addOptionItem(OPTIONS_ANAGLYPH_3D, minecraft)
		.addOptionItem(OPTIONS_VIEW_BOBBING, minecraft)
		.addOptionItem(OPTIONS_AMBIENT_OCCLUSION, minecraft);
	optionPanes[3]->addOptionItem(OPTIONS_DEBUG_SCREEN_SIZE, minecraft);
	// Tweaks
	optionPanes[4]->addOptionItem(OPTIONS_ALLOW_SPRINT, minecraft)
		.addOptionItem(OPTIONS_BAR_ON_TOP, minecraft)
		.addOptionItem(OPTIONS_RPI_CURSOR, minecraft);
	// World
	optionPanes[5]->addOptionItem(OPTIONS_WORLD_SCALE_X, minecraft)
		.addOptionItem(OPTIONS_WORLD_SCALE_Z, minecraft)
		.addOptionItem(OPTIONS_WORLD_OFFSET_X, minecraft)
		.addOptionItem(OPTIONS_WORLD_OFFSET_Z, minecraft)
		.addOptionItem(OPTIONS_POSTPONED_FRINGE, minecraft)
		.addOptionItem(OPTIONS_PROGRESSIVE_FARLANDS, minecraft)
		.addOptionItem(OPTIONS_SEA_LEVEL, minecraft)
		.addOptionItem(OPTIONS_STRIPE_REPAIR, minecraft)
		.addOptionItem(OPTIONS_TELEPORT, minecraft);
}

void OptionsScreen::mouseClicked(int x, int y, int buttonNum) {
	if (currentOptionsGroup && isPointInScrollArea(x, y)) {
		transformMouseForScroll(x, y);
		currentOptionsGroup->mouseClicked(minecraft, x, y, buttonNum);
		if (buttonNum == MouseAction::ACTION_LEFT) {
			isDragging = true;
			lastMouseY = (float)y;
			scrollVelocity = 0.0f;
		}
	} else {
		super::mouseClicked(x, y, buttonNum);
	}
}

void OptionsScreen::mouseReleased(int x, int y, int buttonNum) {
	if (currentOptionsGroup && isDragging) {
		transformMouseForScroll(x, y);
		currentOptionsGroup->mouseReleased(minecraft, x, y, buttonNum);
		isDragging = false;
	} else if (currentOptionsGroup && isPointInScrollArea(x, y)) {
		transformMouseForScroll(x, y);
		currentOptionsGroup->mouseReleased(minecraft, x, y, buttonNum);
	} else {
		super::mouseReleased(x, y, buttonNum);
	}
}

void OptionsScreen::mouseWheel(int dx, int dy, int xm, int ym) {
	if (currentOptionsGroup && isPointInScrollArea(xm, ym)) {
		scrollOffset -= dy * 20.0f;
		applyScrollLimits();
		scrollVelocity = 0.0f;
	} else {
		super::mouseWheel(dx, dy, xm, ym);
	}
}

void OptionsScreen::keyPressed(int eventKey) {
	if (currentOptionsGroup)
		currentOptionsGroup->keyPressed(minecraft, eventKey);
	if (eventKey == Keyboard::KEY_ESCAPE)
		minecraft->options.save();
	super::keyPressed(eventKey);
}

void OptionsScreen::charPressed(char inputChar) {
	if (currentOptionsGroup)
		currentOptionsGroup->charPressed(minecraft, inputChar);
	super::keyPressed(inputChar);
}

void OptionsScreen::tick() {
	if (currentOptionsGroup)
		currentOptionsGroup->tick(minecraft);

	// 惯性滚动
	if (!isDragging && fabs(scrollVelocity) > 0.01f) {
		scrollOffset += scrollVelocity;
		applyScrollLimits();
		scrollVelocity *= 0.92f;
		if (fabs(scrollVelocity) < 0.01f) scrollVelocity = 0.0f;
	}

	// 拖动更新
	if (isDragging) {
		int mx = Mouse::getX();
		int my = Mouse::getY();
		minecraft->screen->toGUICoordinate(mx, my);
		if (isPointInScrollArea(mx, my)) {
			float delta = lastMouseY - my;
			scrollOffset += delta;
			applyScrollLimits();
			scrollVelocity = delta * 0.5f;
			lastMouseY = (float)my;
		}
	}
	super::tick();
}

// ========== 辅助函数 ==========
void OptionsScreen::updateMaxScrollOffset() {
	if (!currentOptionsGroup) {
		maxScrollOffset = 0.0f;
		return;
	}
	float contentH = (float)currentOptionsGroup->height;
	float viewportH = (float)(height - currentOptionsGroup->y - (btnCredits ? btnCredits->height + 5 : 0));
	maxScrollOffset = std::max(0.0f, contentH - viewportH);
}

void OptionsScreen::applyScrollLimits() {
	scrollOffset = Mth::clamp(scrollOffset, 0.0f, maxScrollOffset);
	if (scrollOffset <= 0.0f || scrollOffset >= maxScrollOffset)
		scrollVelocity = 0.0f;
}

float OptionsScreen::getContentHeight() const {
	return currentOptionsGroup ? (float)currentOptionsGroup->height : 0.0f;
}

bool OptionsScreen::isPointInScrollArea(int x, int y) const {
	if (!currentOptionsGroup) return false;
	int bottomPad = (btnCredits ? btnCredits->height + 5 : 0);
	int areaBottom = height - bottomPad;
	return (x >= currentOptionsGroup->x && x < currentOptionsGroup->x + currentOptionsGroup->width &&
	        y >= currentOptionsGroup->y && y < areaBottom);
}

void OptionsScreen::transformMouseForScroll(int& x, int& y) const {
	y += (int)scrollOffset;
}
