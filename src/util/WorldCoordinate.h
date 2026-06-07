#ifndef NET_MINECRAFT_WORLD__WorldCoordinate_H__
#define NET_MINECRAFT_WORLD__WorldCoordinate_H__

#include <cstdint>
#include <string>
#include <cstdio>
#include <cstring>
#include <cmath>
#include <limits>
#include <boost/multiprecision/cpp_dec_float.hpp>

// =====================================================================
//  类型定义
// =====================================================================

/// 日常存储/计算使用的双精度坐标（快速）
typedef double WorldCoordinate;

/// 中间安全计算使用的任意精度十进制浮点（防溢出/防 inf，算完即丢）
typedef boost::multiprecision::number<
    boost::multiprecision::cpp_dec_float<50>,
    boost::multiprecision::et_off
> BigWorldCoordinate;

// =====================================================================
//  阈值常量
// =====================================================================

/// |坐标| ≥ 2^48 时切换 BigWorldCoordinate 精确路径
static constexpr double BIG_THRESHOLD = 281474976710656.0;

// =====================================================================
//  阈值判断
// =====================================================================

inline bool needsBigWorldCoord(double v) {
    return !std::isfinite(v) || std::abs(v) >= BIG_THRESHOLD;
}

// =====================================================================
//  BigWorldCoordinate 计算
// =====================================================================

/// double → BigWorldCoordinate
inline BigWorldCoordinate worldCoordToBig(double v) {
    return BigWorldCoordinate(v);
}

/// 精确计算 pxo = px * scale + offset * scale
inline BigWorldCoordinate computeWorldCoordBig(
    double px,
    WorldCoordinate scale,
    WorldCoordinate offset)
{
    BigWorldCoordinate bpx(px);
    BigWorldCoordinate bs(scale);
    BigWorldCoordinate bo(offset);
    return bpx * bs + bo * bs;
}

/// BigWorldCoordinate → double
inline double worldCoordBigToDouble(const BigWorldCoordinate& v) {
    return v.convert_to<double>();
}

/// BigWorldCoordinate → 字符串
inline std::string worldCoordBigToString(const BigWorldCoordinate& v) {
    return v.str();
}

// =====================================================================
//  整数坐标工具（参考代码 - 保留用于区块相关计算）
// =====================================================================

typedef int64_t IntWorldCoordinate;

inline bool canConvertWorldCoordinateToInt(IntWorldCoordinate value) {
    return value >= static_cast<IntWorldCoordinate>((std::numeric_limits<int>::min)())
        && value <= static_cast<WorldCoordinate>((std::numeric_limits<int>::max)());
}

inline int clampWorldCoordinateToInt(IntWorldCoordinate value) {
    if (value > static_cast<IntWorldCoordinate>((std::numeric_limits<int>::max)())) {
        return (std::numeric_limits<int>::max)();
    }
    if (value < static_cast<IntWorldCoordinate>((std::numeric_limits<int>::min)())) {
        return (std::numeric_limits<int>::min)();
    }
    return static_cast<int>(value);
}

inline IntWorldCoordinate floorDivWorldCoordinate(IntWorldCoordinate value, IntWorldCoordinate divisor) {
    IntWorldCoordinate quotient = value / divisor;
    IntWorldCoordinate remainder = value % divisor;
    if (remainder != 0 && ((remainder > 0) != (divisor > 0))) {
        --quotient;
    }
    return quotient;
}

inline IntWorldCoordinate blockToChunkCoordinate(IntWorldCoordinate blockCoordinate) {
    return floorDivWorldCoordinate(blockCoordinate, 16);
}

inline int localBlockCoordinate(IntWorldCoordinate blockCoordinate) {
    IntWorldCoordinate local = blockCoordinate % 16;
    if (local < 0) {
        local += 16;
    }
    return static_cast<int>(local);
}

inline IntWorldCoordinate clampBigWorldCoordinateToInt(const BigWorldCoordinate& value) {
    const BigWorldCoordinate maxValue = (std::numeric_limits<IntWorldCoordinate>::max)();
    const BigWorldCoordinate minValue = (std::numeric_limits<IntWorldCoordinate>::min)();
    if (value > maxValue) return (std::numeric_limits<IntWorldCoordinate>::max)();
    if (value < minValue) return (std::numeric_limits<IntWorldCoordinate>::min)();
    return static_cast<IntWorldCoordinate>((int64_t)value.convert_to<long long>());
}

#endif /* NET_MINECRAFT_WORLD__WorldCoordinate_H__ */
