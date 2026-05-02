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
    void startWorkerThread();
    void stopWorkerThread();
    void workerLoop();

    std::string toPercentString(float percentage);

    Minecraft* _mc;
    Font*      _font;
    std::string _debugPath;

    std::vector<float> frameTimes;
    std::vector<float> tickTimes;
    int frameTimePos;
    float lastTimer;

    // 后台线程相关
    std::thread         _workerThread;
    std::mutex          _dataMutex;
    std::condition_variable _cv;
    std::atomic<bool>   _workerRunning{false};
    bool                _needsUpdate{true};

    // 线程间共享的最新数据
    std::vector<PerfTimer::ResultField> _latestList;
    PerfTimer::ResultField              _latestNode{"", 0.0f, 0.0f};
};

#endif
