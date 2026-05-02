#ifndef NET_UTIL__PerfTimer_H__
#define NET_UTIL__PerfTimer_H__

#include <map>
#include <vector>
#include "StringUtils.h"

//package util;
#ifdef PROFILER
    #define PERF_TIMER_SKIP_FRAMES 8

    #define TIMER_PUSH(x)   do { \
        static int perfSkip = 0; \
        if ((++perfSkip % PERF_TIMER_SKIP_FRAMES) == 0) { \
            PerfTimer::push(x); \
        } \
    } while(0)

    #define TIMER_POP()      do { \
        static int perfSkip = 0; \
        if ((++perfSkip % PERF_TIMER_SKIP_FRAMES) == 0) { \
            PerfTimer::pop(); \
        } \
    } while(0)

    #define TIMER_POP_PUSH(x) do { \
        static int perfSkip = 0; \
        if ((++perfSkip % PERF_TIMER_SKIP_FRAMES) == 0) { \
            PerfTimer::popPush(x); \
        } \
    } while(0)
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
		:	name(name),
			percentage(percentage),
			globalPercentage(globalPercentage)
		{}

        bool operator<(const ResultField& rf) const {
			if (percentage != rf.percentage)
				return percentage > rf.percentage;
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

    static bool enabled;
private:
    static std::vector<std::string> paths;
    static std::vector<float> startTimes;
    static std::string path;
    static TimeMap times;
};

#endif /*NET_UTIL__PerfTimer_H__*/
