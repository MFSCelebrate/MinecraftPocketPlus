#ifndef MTH_H__
#define MTH_H__

#include <vector>
#include <set>
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace Mth {
    constexpr float PI = 3.1415926535897932384626433832795028841971f;
    constexpr float TWO_PI = 2.0f * PI;
    constexpr float DEGRAD = PI / 180.0f;
    const float RADDEG = 180.0f / PI;

    // ========== double 版本（返回 int64_t）==========
    inline double sqrt(double x) { return std::sqrt(x); }
// Mth.h
inline int floor(double x) {
    return (int)std::floor(x);  // 保留原版行为
}

inline int64_t floor64(double x) {
    return (int64_t)std::floor(x);
}
    inline double sin(double x) { return std::sin(x); }
    inline double cos(double x) { return std::cos(x); }
    inline double atan(double x) { return std::atan(x); }
    inline double atan2(double dy, double dx) { return std::atan2(dy, dx); }
    inline double abs(double a) { return a < 0 ? -a : a; }
    inline double Min(double a, double b) { return a < b ? a : b; }
    inline double Max(double a, double b) { return a > b ? a : b; }
    inline double clamp(double v, double low, double high) {
        if (v < low) return low;
        if (v > high) return high;
        return v;
    }
    inline double lerp(double src, double dst, double alpha) {
        return src + (dst - src) * alpha;
    }
    inline double absDecrease(double value, double with, double min) {
        double absVal = abs(value);
        double absWith = abs(with);
        if (absVal < min) return value;
        double newVal = absVal - absWith;
        if (newVal < min) newVal = min;
        return (value < 0) ? -newVal : newVal;
    }
    inline double absMax(double a, double b) {
        return abs(a) > abs(b) ? a : b;
    }
    inline double absMaxSigned(double a, double b) {
        return (abs(a) > abs(b)) ? a : b;
    }
    inline int64_t abs(int64_t a) { return a < 0 ? -a : a; }

    // ========== 精度丢失检测（新增） ==========
    inline void computePrecisionLoss(double maxCoord, double& doublePrecision, float& floatPrecision) {
        int exp;
        std::frexp(maxCoord, &exp);
        doublePrecision = std::ldexp(1.0, exp - 53);
        floatPrecision  = (float)std::ldexp(1.0, exp - 24);
    }

    inline int getPrecisionColor(double precision) {
        if (precision < 0.03125)
            return 0xFF88FF88;   // 浅绿色
        else if (precision < 2.0)
            return 0xFFFFFF55;   // 浅黄色
        else
            return 0xFFFF5555;   // 浅红色
    }

    // 原有非内联函数的声明
    void initMth();
    float invSqrt(float x);
    float random();
    int random(int n);
    int abs(int a);
    int Min(int a, int b);
    int Max(int a, int b);
    int clamp(int v, int low, int high);
    float absDecrease(float value, float with, float min);
    float absMax(float a, float b);
    float absMaxSigned(float a, float b);
    int intFloorDiv(int a, int b);
};

namespace Util
{
    template <class T>
    int removeAll(std::vector<T>& superset, const std::vector<T>& toRemove) {
        int subSize = (int)toRemove.size();
        int removed = 0;
        for (int i = 0; i < subSize; ++i) {
            T elem = toRemove[i];
            int size = (int)superset.size();
            for (int j = 0; j < size; ++j) {
                if (elem == superset[j]) {
                    superset.erase(superset.begin() + j, superset.begin() + j + 1);
                    ++removed;
                    break;
                }
            }
        }
        return removed;
    }

    template <class T>
    bool remove(std::vector<T>& list, const T& instance) {
        typename std::vector<T>::iterator it = std::find(list.begin(), list.end(), instance);
        if (it == list.end()) return false;
        list.erase(it);
        return true;
    }

    template <class T>
    bool remove(std::set<T>& list, const T& instance) {
        typename std::set<T>::iterator it = std::find(list.begin(), list.end(), instance);
        if (it == list.end()) return false;
        list.erase(it);
        return true;
    }
};

#endif
