#include "PerfTimer.h"
#include "../platform/time.h"
#include <algorithm>

/*static*/
bool
	PerfTimer::enabled = false;

int PerfTimer::s_frameCounter = 0;
std::vector<PerfTimer::ResultField> PerfTimer::s_cachedLog;
std::string PerfTimer::s_cachedPath;

std::vector<std::string>
	PerfTimer::paths;

std::vector<float>
	PerfTimer::startTimes;

std::string
	PerfTimer::path;

PerfTimer::TimeMap
	PerfTimer::times;


/*static*/
void PerfTimer::reset()
{
	times.clear();
}

/*static*/
void PerfTimer::push( const std::string& name )
{
	if (!enabled) return;
	if (path.length() > 0) path += ".";
	path += name;
	paths.push_back(path);
	startTimes.push_back(getTimeS());
}

/*static*/
void PerfTimer::pop()
{
	if (!enabled) return;
	float endTime = getTimeS();
	float startTime = startTimes.back();

	paths.pop_back();
	startTimes.pop_back();

	float time = endTime - startTime;

	TimeMap::iterator it = times.find(path);
	if (it != times.end()) {
		it->second += time;
	} else {
		times.insert(std::make_pair(path, time));
	}

	path = paths.size() > 0 ? paths.back() : "";

	//if (paths.size() == 0) {
	//	for (TimeMap::iterator it = times.begin(); it != times.end(); ++it) {
	//		LOGI("p: %s t: %f\n", it->first.c_str(), it->second);
	//	}
	//}
}

/*static*/
void PerfTimer::popPush( const std::string& name )
{
	pop();
	push(name);
}

/*static*/
std::vector<PerfTimer::ResultField> PerfTimer::getLog(const std::string& rawPath, bool forceUpdate) {
    if (!enabled) return std::vector<ResultField>();

    s_frameCounter++;

    // 非强制更新，且每4帧才重新计算，其余返回缓存
    if (!forceUpdate && (s_frameCounter % 4 != 0) && (s_cachedPath == rawPath) && !s_cachedLog.empty()) {
        return s_cachedLog;
    }

    // ---------- 以下为原始计算逻辑（完全不变） ----------
    std::string path = rawPath;

    TimeMap::const_iterator itRoot = times.find("root");
    float globalTime = (itRoot != times.end())? itRoot->second : 0;

    TimeMap::const_iterator itPath = times.find(path);
    float totalTime2 = (itRoot != times.end())? itRoot->second : -1;

    std::vector<ResultField> result;

    if (path.length() > 0) path += ".";
    float totalTime = 0;

    for (TimeMap::const_iterator cit = times.begin(); cit != times.end(); ++cit) {
        const std::string& key = cit->first;
        const float& time = cit->second;
        if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
            totalTime += time;
        }
    }

    float oldTime = totalTime;
    if (totalTime < totalTime2) totalTime = totalTime2;
    if (globalTime < totalTime) globalTime = totalTime;

    for (TimeMap::const_iterator cit = times.begin(); cit != times.end(); ++cit) {
        const std::string& key = cit->first;
        if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
            float time = times.find(key)->second;
            float timePercentage = time * 100.0f / totalTime;
            float globalPercentage = time * 100.0f / globalTime;
            std::string name = key.substr(path.length());
            result.push_back(ResultField(name, timePercentage, globalPercentage));
        }
    }

    for (TimeMap::iterator it = times.begin(); it != times.end(); ++it)
        it->second *= 0.999f;

    if (totalTime > oldTime)
        result.push_back(ResultField("unspecified", (totalTime - oldTime) * 100.0f / totalTime, (totalTime - oldTime) * 100.0f / globalTime));

    std::sort(result.begin(), result.end());
    result.insert(result.begin(), ResultField(rawPath, 100, totalTime * 100.0f / globalTime));

    // 更新缓存
    s_cachedLog = result;
    s_cachedPath = rawPath;
    return result;
}
