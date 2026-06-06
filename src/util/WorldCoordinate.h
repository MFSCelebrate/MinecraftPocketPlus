#ifndef NET_MINECRAFT_WORLD__WorldCoordinate_H__
#define NET_MINECRAFT_WORLD__WorldCoordinate_H__

#include <cstdint>
#include <string>
#include <cstdio>
#include <cstring>
#include <limits>
#include <boost/multiprecision/cpp_int.hpp>

// =====================================================================
//  类型定义
// =====================================================================

/// 日常存储/计算使用的 64 位坐标（快速）
typedef int64_t WorldCoordinate;

/// 中间安全计算使用的任意精度大整数（防溢出，算完即丢）
typedef boost::multiprecision::cpp_int BigWorldCoordinate;

// =====================================================================
//  定点数工具（用于 WorldScale / WorldOffset）
//  内部存储为 WorldCoordinate × FIXED_SCALE
// =====================================================================

/// 定点数因子：小数点后 12 位
static constexpr int64_t FIXED_SCALE = 1000000000000LL;     // 10^12
static constexpr double  FIXED_SCALE_D = 1.0e12;
static constexpr double  BIG_THRESHOLD = 281474976710656.0; // 2^48

// ── 辅助：清洗字符串，去掉 + / 科学计数法 / 前导零 ──
inline std::string sanitizeNumberString(const std::string& raw) {
    std::string s = raw;
    // 1. 去掉前导 +
    if (!s.empty() && s[0] == '+') s.erase(0, 1);
    // 2. 去掉前导零（至少保留一位数字）
    size_t start = (s[0] == '-') ? 1 : 0;
    while (start < s.length() - 1 && s[start] == '0') {
        s.erase(start, 1);
    }
    // 3. 如果全空或只剩负号，返回 "0"
    if (s.empty() || s == "-") s = "0";
    return s;
}

// ── 字符串 → WorldCoordinate ──
inline WorldCoordinate worldCoordFromString(const std::string& s) {
    if (s.empty()) return 0;
    // 先清洗
    std::string clean = sanitizeNumberString(s);

    BigWorldCoordinate val;
    size_t dot = clean.find('.');
    if (dot == std::string::npos) {
        // 整数
        val = BigWorldCoordinate(clean) * BigWorldCoordinate(FIXED_SCALE);
    } else {
        std::string iPart = clean.substr(0, dot);
        std::string fPart = clean.substr(dot + 1);
        if (fPart.length() > 12) fPart.resize(12);
        else fPart.append(12 - fPart.length(), '0');
        std::string combined = sanitizeNumberString(iPart + fPart);
        val = BigWorldCoordinate(combined);
    }

    const BigWorldCoordinate maxV = (std::numeric_limits<WorldCoordinate>::max)();
    const BigWorldCoordinate minV = (std::numeric_limits<WorldCoordinate>::min)();
    if (val > maxV) return (std::numeric_limits<WorldCoordinate>::max)();
    if (val < minV) return (std::numeric_limits<WorldCoordinate>::min)();
    return static_cast<WorldCoordinate>(val);
}

// ── 定点 WorldCoordinate → double ──
inline double worldCoordToDouble(WorldCoordinate v) {
    return (double)v / FIXED_SCALE_D;
}

// ── 定点 WorldCoordinate → 格式化字符串 ──
inline std::string worldCoordToString(WorldCoordinate v) {
    bool neg = (v < 0);
    if (neg) v = -v;
    int64_t ip = v / FIXED_SCALE;
    int64_t fp = v % FIXED_SCALE;
    char buf[64];
    snprintf(buf, sizeof(buf), "%s%lld.%012lld",
             neg ? "-" : "", (long long)ip, (long long)fp);
    // 去掉末尾多余的 0
    char* p = buf + strlen(buf) - 1;
    while (p > buf && *p == '0') --p;
    if (*p == '.') *p = '\0';
    else *(p + 1) = '\0';
    return std::string(buf);
}

inline std::string worldCoordBigToString(const BigWorldCoordinate& val) {
    std::string raw = val.str();
    // 补到至少 13 位（12 位小数 + 1 位整数）
    while ((int64_t)raw.length() <= 12)
        raw = "0" + raw;
    size_t dotPos = raw.length() - 12;
    std::string s = raw.substr(0, dotPos) + "." + raw.substr(dotPos);
    // 去掉末尾多余的 0
    while (!s.empty() && s.back() == '0') s.pop_back();
    if (!s.empty() && s.back() == '.') s.pop_back();
    return s;
}

// =====================================================================
//  BigWorldCoordinate 精确计算（防溢出）
// =====================================================================

/// 判断是否需要启用大整数路径
inline bool needsBigWorldCoord(double v) {
    return std::abs(v) >= BIG_THRESHOLD;
}

/// 计算 worldCoord = px * scale + offset * scale
/// pxBig 是玩家坐标 × FIXED_SCALE，scale/offset 是存储的定点数
inline BigWorldCoordinate computeWorldCoordBig(
    const BigWorldCoordinate& pxBig,
    WorldCoordinate scaleFixed,
    WorldCoordinate offsetFixed)
{
    BigWorldCoordinate s(scaleFixed);
    BigWorldCoordinate o(offsetFixed);
    return pxBig * s + o * s;
}

/// BigWorldCoordinate → double
inline double worldCoordBigToDouble(const BigWorldCoordinate& v) {
    return v.convert_to<double>() / FIXED_SCALE_D;
}

// ── double → BigWorldCoordinate ──
inline BigWorldCoordinate doubleToWorldCoordBig(double v) {
    // 用足够大的缓冲区，%.12f 对大值也不会溢出
    char buf[128];
    snprintf(buf, sizeof(buf), "%.12f", v);
    std::string s(buf);
    // 找到小数点并删除
    size_t dot = s.find('.');
    if (dot != std::string::npos) {
        s.erase(dot, 1);
    }
    // 清洗：+ 号、前导零
    s = sanitizeNumberString(s);
    return BigWorldCoordinate(s);
}

// =====================================================================
//  整数坐标工具（参考代码）
// =====================================================================

inline bool canConvertWorldCoordinateToInt(WorldCoordinate value) {
    return value >= static_cast<WorldCoordinate>((std::numeric_limits<int>::min)())
        && value <= static_cast<WorldCoordinate>((std::numeric_limits<int>::max)());
}

inline int clampWorldCoordinateToInt(WorldCoordinate value) {
    if (value > static_cast<WorldCoordinate>((std::numeric_limits<int>::max)())) {
        return (std::numeric_limits<int>::max)();
    }
    if (value < static_cast<WorldCoordinate>((std::numeric_limits<int>::min)())) {
        return (std::numeric_limits<int>::min)();
    }
    return static_cast<int>(value);
}

inline WorldCoordinate floorDivWorldCoordinate(WorldCoordinate value, WorldCoordinate divisor) {
    WorldCoordinate quotient = value / divisor;
    WorldCoordinate remainder = value % divisor;
    if (remainder != 0 && ((remainder > 0) != (divisor > 0))) {
        --quotient;
    }
    return quotient;
}

inline WorldCoordinate blockToChunkCoordinate(WorldCoordinate blockCoordinate) {
    return floorDivWorldCoordinate(blockCoordinate, 16);
}

inline int localBlockCoordinate(WorldCoordinate blockCoordinate) {
    WorldCoordinate local = blockCoordinate % 16;
    if (local < 0) {
        local += 16;
    }
    return static_cast<int>(local);
}

inline WorldCoordinate clampBigWorldCoordinateToWorld(const BigWorldCoordinate& value) {
    const BigWorldCoordinate maxValue = (std::numeric_limits<WorldCoordinate>::max)();
    const BigWorldCoordinate minValue = (std::numeric_limits<WorldCoordinate>::min)();
    if (value > maxValue) {
        return (std::numeric_limits<WorldCoordinate>::max)();
    }
    if (value < minValue) {
        return (std::numeric_limits<WorldCoordinate>::min)();
    }
    return static_cast<WorldCoordinate>(value);
}

#endif /* NET_MINECRAFT_WORLD__WorldCoordinate_H__ */

