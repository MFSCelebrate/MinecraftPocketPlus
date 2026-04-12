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
	: btnClose(NULL),
	bHeader(NULL),
	btnCredits(NULL),
	selectedCategory(0) {
}

OptionsScreen::~OptionsScreen() {
	if (btnClose != NULL) {
		delete btnClose;
		btnClose = NULL;
	}
	if (bHeader != NULL) {
		delete bHeader;
		bHeader = NULL;
	}
	if (btnCredits != NULL) {
		delete btnCredits;
		btnCredits = NULL;
	}
	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		if (*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	for (std::vector<OptionsGroup*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if (*it != NULL) {
			delete *it;
			*it = NULL;
		}
	}
	categoryButtons.clear();
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

	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		buttons.push_back(*it);
		tabButtons.push_back(*it);
	}

	generateOptionScreens();
	selectCategory(0);
}

void OptionsScreen::setupPositions() {
	int buttonHeight = btnClose->height;

	btnClose->x = width - btnClose->width;
	btnClose->y = 0;

	int offsetNum = 1;
	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		(*it)->x = 0;
		(*it)->y = offsetNum * buttonHeight;
		(*it)->selected = false;
		offsetNum++;
	}

	bHeader->x = 0;
	bHeader->y = 0;
	bHeader->width = width - btnClose->width;
	bHeader->height = btnClose->height;

	if (btnCredits != NULL) {
		btnCredits->x = width - btnCredits->width;
		btnCredits->y = height - btnCredits->height;
	}

	for (std::vector<OptionsGroup*>::iterator it = optionPanes.begin(); it != optionPanes.end(); ++it) {
		if (categoryButtons.size() > 0 && categoryButtons[0] != NULL) {
			(*it)->x = categoryButtons[0]->width;
			(*it)->y = bHeader->height;
			(*it)->width = width - categoryButtons[0]->width;
			(*it)->setupPositions();
		}
	}

	// 计算滚动相关边界
	updateMaxScrollOffset();
	applyScrollLimits();
}

void OptionsScreen::render(int xm, int ym, float a) {
	renderBackground();

	int xmm = xm * width / minecraft->width;
	int ymm = ym * height / minecraft->height - 1;

	if (currentOptionsGroup != NULL) {
		// 1. 计算裁剪区域（屏幕坐标系，原点左上角）
		int scissorX = currentOptionsGroup->x;
		int scissorY = currentOptionsGroup->y;
		int scissorW = currentOptionsGroup->width;
		// 可视区域高度：从面板顶部到屏幕底部，但要为底部按钮留出空间
		int bottomPadding = (btnCredits ? btnCredits->height + 5 : 0);
		int scissorH = height - scissorY - bottomPadding;
		if (scissorH < 0) scissorH = 0;

		// 2. 保存当前的裁剪状态，避免影响后续渲染
		GLboolean scissorWasEnabled = glIsEnabled(GL_SCISSOR_TEST);
		GLint oldScissorBox[4];
		glGetIntegerv(GL_SCISSOR_BOX, oldScissorBox);

		// 3. 启用裁剪并设置区域（注意Y坐标转换：OpenGL原点在左下角）
		glEnable(GL_SCISSOR_TEST);
		// OpenGL 裁剪区域：x, y(左下角), width, height
		int glScissorY = minecraft->height - (scissorY + scissorH);
		glScissor(scissorX, glScissorY, scissorW, scissorH);

		// 4. 应用滚动偏移，渲染面板内容
		glPushMatrix();
		glTranslatef(0.0f, -scrollOffset, 0.0f);

		// 注意：传递给 currentOptionsGroup 的鼠标坐标可能需要调整，但 render 中主要影响悬停效果，可忽略或稍后修正
		currentOptionsGroup->render(minecraft, xmm, ymm + (int)scrollOffset);

		glPopMatrix();

		// 5. 恢复之前的裁剪状态
		if (!scissorWasEnabled) {
			glDisable(GL_SCISSOR_TEST);
		} else {
			glScissor(oldScissorBox[0], oldScissorBox[1], oldScissorBox[2], oldScissorBox[3]);
		}
	}
	
	// 渲染其他UI（分类按钮、关闭按钮、Credits等，这些不应被裁剪）
	super::render(xm, ym, a);
}

void OptionsScreen::removed() {
}

void OptionsScreen::buttonClicked(Button* button) {
	if (button == btnClose) {
		minecraft->options.save();
		if (minecraft->screen != NULL) {
			minecraft->setScreen(NULL);
		} else {
			minecraft->screenChooser.setScreen(SCREEN_STARTMENU);
		}
	}
	else if (button->id >= categoryButtons[0]->id && button->id <= categoryButtons.back()->id) {
        int categoryButton = button->id - categoryButtons[0]->id;
        selectCategory(categoryButton);
	}
	else if (button == btnCredits) {
		minecraft->setScreen(new CreditsScreen());
	}
}

void OptionsScreen::selectCategory(int index) {
	int currentIndex = 0;
	for (std::vector<Touch::TButton*>::iterator it = categoryButtons.begin(); it != categoryButtons.end(); ++it) {
		(*it)->selected = (index == currentIndex);
		currentIndex++;
	}
	if (index < (int)optionPanes.size()) {
		currentOptionsGroup = optionPanes[index];
		// 切换分类时重置滚动位置
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

    // General Pane
    optionPanes[0]->addOptionItem(OPTIONS_USERNAME, minecraft)
        .addOptionItem(OPTIONS_SENSITIVITY, minecraft);

    // Game Pane
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

    // Controls Pane
    optionPanes[2]->addOptionItem(OPTIONS_INVERT_Y_MOUSE, minecraft)
        .addOptionItem(OPTIONS_USE_TOUCHSCREEN, minecraft)
        .addOptionItem(OPTIONS_AUTOJUMP, minecraft);
    for (int i = OPTIONS_KEY_FORWARD; i <= OPTIONS_KEY_USE; i++) {
        optionPanes[2]->addOptionItem((OptionId)i, minecraft);
    }

    // Graphics Pane
    optionPanes[3]->addOptionItem(OPTIONS_FANCY_GRAPHICS, minecraft)
        .addOptionItem(OPTIONS_LIMIT_FRAMERATE, minecraft)
        .addOptionItem(OPTIONS_VSYNC, minecraft)
        .addOptionItem(OPTIONS_RENDER_DEBUG, minecraft)
        .addOptionItem(OPTIONS_ANAGLYPH_3D, minecraft)
        .addOptionItem(OPTIONS_VIEW_BOBBING, minecraft)
        .addOptionItem(OPTIONS_AMBIENT_OCCLUSION, minecraft);
	optionPanes[3]->addOptionItem(OPTIONS_DEBUG_SCREEN_SIZE, minecraft);
    
    // Tweaks Pane
    optionPanes[4]->addOptionItem(OPTIONS_ALLOW_SPRINT, minecraft)
        .addOptionItem(OPTIONS_BAR_ON_TOP, minecraft)
        .addOptionItem(OPTIONS_RPI_CURSOR, minecraft);

    // World Pane
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
	// 如果点击在滚动区域内，转换坐标后再传递给 currentOptionsGroup
	if (currentOptionsGroup != NULL && isPointInScrollArea(x, y)) {
		transformMouseForScroll(x, y);
		currentOptionsGroup->mouseClicked(minecraft, x, y, buttonNum);
		// 记录拖动起始位置
		if (buttonNum == MouseAction::ACTION_LEFT) {
			isDragging = true;
			lastMouseY = (float)y;
			scrollVelocity = 0.0f; // 停止惯性
		}
	} else {
		// 否则正常传递给父类处理（分类按钮、关闭按钮等）
		super::mouseClicked(x, y, buttonNum);
	}
}

void OptionsScreen::mouseReleased(int x, int y, int buttonNum) {
	if (currentOptionsGroup != NULL && isDragging) {
		// 如果之前在拖动，先转换坐标再传递释放事件
		transformMouseForScroll(x, y);
		currentOptionsGroup->mouseReleased(minecraft, x, y, buttonNum);
		isDragging = false;
	} else if (currentOptionsGroup != NULL && isPointInScrollArea(x, y)) {
		transformMouseForScroll(x, y);
		currentOptionsGroup->mouseReleased(minecraft, x, y, buttonNum);
	} else {
		super::mouseReleased(x, y, buttonNum);
	}
}

void OptionsScreen::mouseWheel(int dx, int dy, int xm, int ym) {
	if (currentOptionsGroup != NULL && isPointInScrollArea(xm, ym)) {
		// 滚轮滚动：每格滚动 20 像素
		const float scrollStep = 20.0f;
		scrollOffset -= dy * scrollStep;
		applyScrollLimits();
		scrollVelocity = 0.0f; // 滚轮后停止惯性
	} else {
		super::mouseWheel(dx, dy, xm, ym);
	}
}

void OptionsScreen::keyPressed(int eventKey) {
	if (currentOptionsGroup != NULL)
		currentOptionsGroup->keyPressed(minecraft, eventKey);
	if (eventKey == Keyboard::KEY_ESCAPE) 
		minecraft->options.save();
	super::keyPressed(eventKey);
}

void OptionsScreen::charPressed(char inputChar) {
	if (currentOptionsGroup != NULL)
		currentOptionsGroup->charPressed(minecraft, inputChar);
	super::keyPressed(inputChar);
}

void OptionsScreen::tick() {
	if (currentOptionsGroup != NULL) {
		currentOptionsGroup->tick(minecraft);
	}

	// 处理惯性滚动
	if (!isDragging && std::abs(scrollVelocity) > 0.01f) {
		scrollOffset += scrollVelocity;
		applyScrollLimits();
		scrollVelocity *= 0.92f; // 摩擦力
		if (std::abs(scrollVelocity) < 0.01f) {
			scrollVelocity = 0.0f;
		}
	}

	// 如果正在拖动，根据鼠标移动更新滚动偏移
	if (isDragging) {
		int mouseX = Mouse::getX();
		int mouseY = Mouse::getY();
		minecraft->screen->toGUICoordinate(mouseX, mouseY);
		if (isPointInScrollArea(mouseX, mouseY)) {
			float deltaY = lastMouseY - mouseY;
			scrollOffset += deltaY;
			applyScrollLimits();
			// 计算速度（简单差分）
			scrollVelocity = deltaY * 0.5f;
			lastMouseY = (float)mouseY;
		}
	}

	super::tick();
}

// === 辅助函数实现 ===

void OptionsScreen::updateMaxScrollOffset() {
	if (currentOptionsGroup == NULL) {
		maxScrollOffset = 0.0f;
		return;
	}
	float contentHeight = getContentHeight();
	float viewportHeight = (float)(height - currentOptionsGroup->y - (btnCredits ? btnCredits->height + 5 : 0));
	maxScrollOffset = std::max(0.0f, contentHeight - viewportHeight);
}

bool OptionsScreen::isPointInScrollArea(int x, int y) const {
	if (currentOptionsGroup == NULL) return false;
	int bottomPadding = (btnCredits ? btnCredits->height + 5 : 0);
	int areaBottom = height - bottomPadding;
	return (x >= currentOptionsGroup->x && x < currentOptionsGroup->x + currentOptionsGroup->width &&
	        y >= currentOptionsGroup->y && y < areaBottom);
}

void OptionsScreen::transformMouseForScroll(int& x, int& y) const {
	// 将屏幕坐标转换为“内容坐标”：只需加上滚动偏移量
	// 因为子控件存储的是原始屏幕绝对坐标（在 setupPositions 时确定的），
	// 渲染时内容向上平移了 scrollOffset，所以点击测试时需要将鼠标 Y 坐标下移相同量。
	y += (int)scrollOffset;
}

void OptionsScreen::applyScrollLimits() {
	scrollOffset = Mth::clamp(scrollOffset, 0.0f, maxScrollOffset);
	if (scrollOffset <= 0.0f || scrollOffset >= maxScrollOffset) {
		scrollVelocity = 0.0f;
	}
}

float OptionsScreen::getContentHeight() const {
	if (currentOptionsGroup == NULL) return 0.0f;
	// OptionsGroup 的高度在 setupPositions 后已经计算好，存储在 height 成员中
	return (float)currentOptionsGroup->height;
}
