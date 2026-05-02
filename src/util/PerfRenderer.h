#ifndef NET_UTIL__PerfRenderer_H__
#define NET_UTIL__PerfRenderer_H__

#include <vector>
#include <string>
#include <sstream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>

#include "PerfTimer.h"

class Minecraft;
class Font;

class PerfRenderer {
public:
    PerfRenderer( Minecraft* mc, Font* font );
    ~PerfRenderer();

    void debugFpsMeterKeyPress(int key);
    void renderFpsMeter(float tickTime);

private:
    void workerLoop();

    std::string toPercentString(float percentage);

    Minecraft* _mc;
    Font*      _font;
    std::string _debugPath;

    std::vector<float> frameTimes;
    std::vector<float> tickTimes;
    int frameTimePos;
    float lastTimer;

    // 后台线程
    std::thread _worker;
    std::mutex  _mutex;
    std::condition_variable _cv;
    std::atomic<bool> _workerRunning{false};
    std::atomic<bool> _needsUpdate{false};

    // 共享数据
    std::vector<PerfTimer::ResultField> _latestList;
    PerfTimer::ResultField _latestNode;

    // 后备数据（防消失）
    std::vector<PerfTimer::ResultField> _lastValidList;
    PerfTimer::ResultField _lastValidNode{"", 0, 0};
    bool _hasValidData = false;
};

#endif
