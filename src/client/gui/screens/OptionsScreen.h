#ifndef NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__
#define NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__

#include "../Screen.h"
#include "../components/Button.h"
#include "../components/OptionsGroup.h"

class ImageButton;
class OptionsPane;

class OptionsScreen: public Screen
{
	typedef Screen super;

	void init();
	void generateOptionScreens();

public:
	OptionsScreen();
	~OptionsScreen();

	void setupPositions();
	void buttonClicked(Button* button);
	void render(int xm, int ym, float a);
	void removed();
	void selectCategory(int index);

	virtual void mouseClicked(int x, int y, int buttonNum);
	virtual void mouseReleased(int x, int y, int buttonNum);
	virtual void mouseWheel(int dx, int dy, int xm, int ym) override; // 新增滚轮支持
	virtual void keyPressed(int eventKey);
	virtual void charPressed(char inputChar);
	
	virtual void tick();

private:
	Touch::THeader* bHeader;
	ImageButton* btnClose;
	Button* btnCredits;

	std::vector<Touch::TButton*> categoryButtons;
	std::vector<OptionsGroup*> optionPanes;
	OptionsGroup* currentOptionsGroup;
	int selectedCategory;

	// === 滚动相关成员 ===
	float scrollOffset = 0.0f;          // 当前滚动偏移量（正数表示内容向上滚动）
	float maxScrollOffset = 0.0f;       // 最大滚动偏移量
	float scrollVelocity = 0.0f;        // 滚动速度（用于惯性）
	float lastMouseY = 0.0f;            // 上一次鼠标Y坐标（用于计算拖动增量）
	bool isDragging = false;            // 是否正在拖动滚动区域
	bool isScrollbarVisible = false;    // 是否显示滚动条（自动判断）
	// 辅助函数
	void updateMaxScrollOffset();
	void applyScrollLimits();
	float getContentHeight() const;
	bool isPointInScrollArea(int x, int y) const;
	void transformMouseForScroll(int& x, int& y) const; // 将屏幕坐标转换为内容坐标
};

#endif /*NET_MINECRAFT_CLIENT_GUI_SCREENS__OptionsScreen_H__*/
