#ifndef NET_UTIL__PerfTimer_H__
#define NET_UTIL__PerfTimer_H__

#include <map>
#include <vector>
#include <string>
#include <mutex>
#include "StringUtils.h"

#define PERF_TIMER_SKIP_FRAMES 8       // 每8帧记录一次

#ifdef PROFILER
    #define TIMER_PUSH(x) do { \
        if (PerfTimer::enabled && (PerfTimer::s_frameCounter % PERF_TIMER_SKIP_FRAMES == 0 || PerfTimer::s_warmupFrames > 0)) \
            PerfTimer::push(x); \
    } while(0)

    #define TIMER_POP() do { \
        if (PerfTimer::enabled && (PerfTimer::s_frameCounter % PERF_TIMER_SKIP_FRAMES == 0 || PerfTimer::s_warmupFrames > 0)) \
            PerfTimer::pop(); \
    } while(0)

    #define TIMER_POP_PUSH(x) do { \
        if (PerfTimer::enabled && (PerfTimer::s_frameCounter % PERF_TIMER_SKIP_FRAMES == 0 || PerfTimer::s_warmupFrames > 0)) \
            PerfTimer::popPush(x); \
    } while(0)
#else
    #define TIMER_PUSH(x)       ((void*)0)
    #define TIMER_POP()         ((void*)0)
    #define TIMER_POP_PUSH(x)   ((void*)0)
#endif

class PerfTimer
{
public:
    class ResultField {
    public:
        float percentage;
        float globalPercentage;
        std::string name;

        ResultField(const std::string& name = "", float percentage = 0, float globalPercentage = 0)
            : name(name), percentage(percentage), globalPercentage(globalPercentage) {}

        bool operator<(const ResultField& rf) const {
            if (percentage != rf.percentage) return percentage > rf.percentage;
            return name > rf.name;
        }
        int getColor() const {
            return (Util::hashCode(name) & 0xaaaaaa) + 0x444444;
        }
    };

    typedef std::map<std::string, float> TimeMap;   // ★ 移到公有
    static void reset();
    static void push(const std::string& name);
    static void pop();
    static void popPush(const std::string& name);
    static std::vector<ResultField> getLog(const std::string& path, bool forceUpdate = false);
    static void tickFrame();

    static bool enabled;
    static int  s_frameCounter;
    static int  s_warmupFrames;

    // 线程安全拷贝当前 times 表，供后台线程使用
    static TimeMap getTimesCopy();

private:
    static std::vector<std::string> paths;
    static std::vector<float> startTimes;
    static std::string path;
    static TimeMap times;
    static std::mutex s_timesMutex;       // 保护 times 的互斥锁
};

#endif
