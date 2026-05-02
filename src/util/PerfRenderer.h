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
    void startWorker();
    void stopWorker();
    void workerLoop();

    std::string toPercentString(float percentage);

    Minecraft* _mc;
    Font*      _font;
    std::string _debugPath;

    std::vector<float> frameTimes;
    std::vector<float> tickTimes;
    int frameTimePos;
    float lastTimer;

    // 多线程数据
    std::thread             _worker;
    std::mutex              _dataMutex;
    std::condition_variable _cv;
    std::atomic<bool>       _running{false};
    std::atomic<bool>       _newDataReady{false};

    // 后台缓冲区
    std::vector<PerfTimer::ResultField> _backList;
    PerfTimer::ResultField              _backNode{"", 0.0f, 0.0f};

    // 前台缓冲区（主线程绘制用）
    std::vector<PerfTimer::ResultField> _frontList;
    PerfTimer::ResultField              _frontNode{"", 0.0f, 0.0f};
};

#endif
