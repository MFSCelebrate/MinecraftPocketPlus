#include "PerfTimer.h"
#include "../platform/time.h"
#include <algorithm>

bool PerfTimer::enabled = false;
int  PerfTimer::s_frameCounter = 0;
int  PerfTimer::s_warmupFrames = 0;

std::vector<std::string> PerfTimer::paths;
std::vector<float> PerfTimer::startTimes;
std::string PerfTimer::path;
PerfTimer::TimeMap PerfTimer::times;
std::mutex PerfTimer::s_timesMutex;

void PerfTimer::tickFrame() {
    s_frameCounter++;
    if (enabled && s_warmupFrames > 0) s_warmupFrames--;
}

void PerfTimer::reset() {
    times.clear();
    paths.clear();
    startTimes.clear();
    path.clear();
    s_frameCounter = 0;
    s_warmupFrames = 64;
}

// 线程安全拷贝
PerfTimer::TimeMap PerfTimer::getTimesCopy() {
    std::lock_guard<std::mutex> lock(s_timesMutex);
    return times;            // 值拷贝
}

void PerfTimer::push( const std::string& name ) {
    if (path.length() > 0) path += ".";
    path += name;
    paths.push_back(path);
    startTimes.push_back(getTimeS());
}

void PerfTimer::pop() {
    if (paths.empty()) return;
    float endTime = getTimeS();
    float startTime = startTimes.back();
    paths.pop_back();
    startTimes.pop_back();
    float time = endTime - startTime;
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

    std::string path = rawPath;
    TimeMap::const_iterator itRoot = times.find("root");
    float globalTime = (itRoot != times.end())? itRoot->second : 0;
    TimeMap::const_iterator itPath = times.find(path);
    float totalTime2 = (itRoot != times.end())? itRoot->second : -1;

    std::vector<ResultField> result;
    if (path.length() > 0) path += ".";
    float totalTime = 0;

    for (const auto& kv : times) {
        const std::string& key = kv.first;
        if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
            totalTime += kv.second;
        }
    }

    float oldTime = totalTime;
    if (totalTime < totalTime2) totalTime = totalTime2;
    if (globalTime < totalTime) globalTime = totalTime;

    for (const auto& kv : times) {
        const std::string& key = kv.first;
        if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
            float time = kv.second;
            float timePercentage = time * 100.0f / totalTime;
            float globalPercentage = time * 100.0f / globalTime;
            result.push_back(ResultField(key.substr(path.length()), timePercentage, globalPercentage));
        }
    }

    // 自适应压缩：大表时清除微小条目（在锁内进行）
    {
        std::lock_guard<std::mutex> lock(s_timesMutex);
        if (times.size() > 128) {
            float totalSum = 0.0f;
            for (auto& p : times) totalSum += p.second;
            float threshold = totalSum * 0.0003f;
            for (auto it = times.begin(); it != times.end(); ) {
                it->second *= 0.999f;          // 仍然进行衰减
                if (it->second < threshold) {
                    it = times.erase(it);
                } else {
                    ++it;
                }
            }
        } else {
            for (auto& p : times) p.second *= 0.999f;   // 小表仅衰减
        }
    }

    if (totalTime > oldTime)
        result.push_back(ResultField("unspecified", (totalTime - oldTime) * 100.0f / totalTime, (totalTime - oldTime) * 100.0f / globalTime));

    std::sort(result.begin(), result.end());
    result.insert(result.begin(), ResultField(rawPath, 100, totalTime * 100.0f / globalTime));
    return result;
}
