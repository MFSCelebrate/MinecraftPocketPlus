// PerfTimer.h
#ifndef NET_UTIL__PerfTimer_H__
#define NET_UTIL__PerfTimer_H__

#include <map>
#include <vector>
#include "StringUtils.h"

// 剖析器总开关，由 PerfRenderer 控制
extern bool gPerfTimerEnabled;

#ifdef PROFILER
    #define TIMER_PUSH(x)       do { if (gPerfTimerEnabled) PerfTimer::push(x); } while(0)
    #define TIMER_POP()         do { if (gPerfTimerEnabled) PerfTimer::pop(); } while(0)
    #define TIMER_POP_PUSH(x)   do { if (gPerfTimerEnabled) { PerfTimer::pop(); PerfTimer::push(x); } } while(0)
#else
    #define TIMER_PUSH(x)       ((void*)0)
    #define TIMER_POP()         ((void*)0)
    #define TIMER_POP_PUSH(x)   ((void*)0)
#endif

class PerfTimer
{
    typedef std::map<std::string, float> TimeMap;
public:
    class ResultField {
    public:
        float percentage;
        float globalPercentage;
        std::string name;

        ResultField(const std::string& name, float percentage, float globalPercentage)
            : name(name), percentage(percentage), globalPercentage(globalPercentage) {}

        bool operator<(const ResultField& rf) const {
            if (percentage != rf.percentage) return percentage > rf.percentage;
            return name > rf.name;
        }
        int getColor() const {
            return (Util::hashCode(name) & 0xaaaaaa) + 0x444444;
        }
    };

    static void reset();
    static void push(const std::string& name);
    static void pop();
    static void popPush(const std::string& name);
    static std::vector<ResultField> getLog(const std::string& path);

private:
    static std::vector<std::string> paths;
    static std::vector<float> startTimes;
    static std::string path;
    static TimeMap times;
};

#endif
