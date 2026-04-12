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
	virtual void mouseWheel(int dx, int dy, int xm, int ym) override;
	virtual void keyPressed(int eventKey);
	virtual void charPressed(char inputChar);
	
	virtual void tick();
	virtual void lostFocus() override;   // 保存输入框内容

private:
	Touch::THeader* bHeader;
	ImageButton* btnClose;
	Button* btnCredits;

	std::vector<Touch::TButton*> categoryButtons;
	std::vector<OptionsGroup*> optionPanes;
	OptionsGroup* currentOptionsGroup;
	int selectedCategory;

	// 滚动相关
	float scrollOffset;
	float maxScrollOffset;
	float scrollVelocity;
	float lastMouseY;
	bool isDragging;
	bool isScrollbarVisible;

	void updateMaxScrollOffset();
	void applyScrollLimits();
	float getContentHeight() const;
	bool isPointInScrollArea(int x, int y) const;
	void transformMouseForScroll(int& x, int& y) const;
};

#endif
