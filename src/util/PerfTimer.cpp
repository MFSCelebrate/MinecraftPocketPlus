#include "PerfTimer.h"
#include "../platform/time.h"
#include <algorithm>

bool PerfTimer::enabled = false;
int  PerfTimer::s_frameCounter = 0;
int  PerfTimer::s_warmupFrames = 0;
std::mutex PerfTimer::s_timesMutex;

std::vector<std::string> PerfTimer::paths;
std::vector<float> PerfTimer::startTimes;
std::string PerfTimer::path;
PerfTimer::TimeMap PerfTimer::times;

void PerfTimer::tickFrame() {
    // 计数器没有竞争，主线程独占
    s_frameCounter++;
    if (enabled && s_warmupFrames > 0) s_warmupFrames--;
}

void PerfTimer::reset() {
    std::lock_guard<std::mutex> lock(s_timesMutex);
    times.clear();
    // paths 只在主线程用，reset 也在主线程，安全
    paths.clear();
    startTimes.clear();
    path.clear();
    s_frameCounter = 0;
    s_warmupFrames = 64;
}

void PerfTimer::push( const std::string& name ) {
    // 主线程独享，不用锁
    if (path.length() > 0) path += ".";
    path += name;
    paths.push_back(path);
    startTimes.push_back(getTimeS());
}

void PerfTimer::pop() {
    // 主线程独享
    if (paths.empty()) return;
    float endTime = getTimeS();
    float startTime = startTimes.back();
    paths.pop_back();
    startTimes.pop_back();
    float time = endTime - startTime;

    // 只有修改 times 时才加锁
    {
        std::lock_guard<std::mutex> lock(s_timesMutex);
        TimeMap::iterator it = times.find(path);
        if (it != times.end()) {
            it->second += time;
        } else {
            times.insert(std::make_pair(path, time));
        }
    }
    path = paths.size() > 0 ? paths.back() : "";
}

void PerfTimer::popPush( const std::string& name ) {
    pop();
    push(name);
}

std::vector<PerfTimer::ResultField> PerfTimer::getLog(const std::string& rawPath, bool /*forceUpdate*/) {
    if (!enabled) return std::vector<ResultField>();

    // 1. 快速加锁，复制 times 表和相关数据
    TimeMap timesCopy;
    float globalTime = 0.0f;
    float totalTime2 = -1.0f;
    {
        std::lock_guard<std::mutex> lock(s_timesMutex);
        timesCopy = times;   // 复制一份
        TimeMap::const_iterator itRoot = times.find("root");
        globalTime = (itRoot != times.end()) ? itRoot->second : 0.0f;
        TimeMap::const_iterator itPath = times.find(rawPath);
        totalTime2 = (itRoot != times.end()) ? itRoot->second : -1.0f;
    }
    // 锁已释放，现在用副本计算，不再阻塞主线程记录

    std::string path = rawPath;
    std::vector<ResultField> result;

    if (path.length() > 0) path += ".";
    float totalTime = 0.0f;
    for (TimeMap::const_iterator cit = timesCopy.begin(); cit != timesCopy.end(); ++cit) {
        const std::string& key = cit->first;
        const float& time = cit->second;
        if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
            totalTime += time;
        }
    }

    float oldTime = totalTime;
    if (totalTime < totalTime2) totalTime = totalTime2;
    if (globalTime < totalTime) globalTime = totalTime;

    for (TimeMap::const_iterator cit = timesCopy.begin(); cit != timesCopy.end(); ++cit) {
        const std::string& key = cit->first;
        if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
            float time = timesCopy.find(key)->second;
            float timePercentage = time * 100.0f / totalTime;
            float globalPercentage = time * 100.0f / globalTime;
            std::string name = key.substr(path.length());
            result.push_back(ResultField(name, timePercentage, globalPercentage));
        }
    }

    // 清理工作仍在锁内进行（简单快速）
    {
        std::lock_guard<std::mutex> lock(s_timesMutex);
        // 自适应清理
        if (times.size() > 128) {
            float totalSum = 0.0f;
            for (auto& p : times) totalSum += p.second;
            float threshold = totalSum * 0.0001f;
            for (auto it = times.begin(); it != times.end(); ) {
                if (it->second < threshold) {
                    it = times.erase(it);
                } else {
                    ++it;
                }
            }
        }
        // 衰减
        for (TimeMap::iterator it = times.begin(); it != times.end(); ++it)
            it->second *= 0.999f;
    }

    if (totalTime > oldTime)
        result.push_back(ResultField("unspecified", (totalTime - oldTime) * 100.0f / totalTime, (totalTime - oldTime) * 100.0f / globalTime));

    std::sort(result.begin(), result.end());
    result.insert(result.begin(), ResultField(rawPath, 100, totalTime * 100.0f / globalTime));
    return result;
}
