#ifndef NET_UTIL__PerfRenderer_H__
#define NET_UTIL__PerfRenderer_H__

#include <vector>
#include <string>
#include <sstream>

class Minecraft;
class Font;

//package net.minecraft.client;
class PerfRenderer
{
public:
    PerfRenderer( Minecraft* mc, Font* font);

    void debugFpsMeterKeyPress(int key);

    void renderFpsMeter(float tickTime);
    void setPieVisible(bool vis) { m_pieVisible = vis; }
    bool isPieVisible() const { return m_pieVisible; }

private:
    std::string toPercentString(float percentage);

	Minecraft*	_mc;
    Font*		_font;
    std::string _debugPath;

	std::vector<float> frameTimes;
	std::vector<float> tickTimes;
	int frameTimePos;
	float lastTimer;
    bool m_pieVisible = false;          // 饼图可见性
    int  m_lastFrameUpdated = -1;       // 上次更新饼图的帧号
    GLuint m_pieVbo = 0;               // 饼图VBO
    int    m_pieVertexCount = 0;        // VBO中的顶点数
    std::vector<float> m_cachedPercentages; // 缓存的上次百分比数据
};

#endif /*NET_UTIL__PerfRenderer_H__*/
