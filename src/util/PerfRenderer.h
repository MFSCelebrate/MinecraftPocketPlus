#ifndef NET_UTIL__PerfRenderer_H__
#define NET_UTIL__PerfRenderer_H__

#include <vector>
#include <string>
#include <sstream>
#include "PerfTimer.h"

class Minecraft;
class Font;

class PerfRenderer {
public:
    PerfRenderer( Minecraft* mc, Font* font );
    void debugFpsMeterKeyPress(int key);
    void renderFpsMeter(float tickTime);
    void navigateBack();
    bool isPieVisible() const { return m_pieVisible; }

private:
    void togglePie();
    std::string toPercentString(float percentage);

    Minecraft* _mc;
    Font*      _font;
    std::string _debugPath;

    std::vector<float> frameTimes;
    std::vector<float> tickTimes;
    int frameTimePos;
    float lastTimer;

    bool m_pieVisible = false;
};

#endif
