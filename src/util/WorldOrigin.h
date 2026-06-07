#ifndef WORLDORIGIN_H__
#define WORLDORIGIN_H__

#include <cstdint>
#include "WorldCoordinate.h"

class WorldOrigin {
    BigWorldCoordinate m_originX = 0;
    BigWorldCoordinate m_originY = 0;
    BigWorldCoordinate m_originZ = 0;

    static constexpr double STEP = BIG_THRESHOLD; // 2^48

public:
    // 绝对坐标
    BigWorldCoordinate absX(double localX) const {
        return m_originX + BigWorldCoordinate(localX);
    }
    BigWorldCoordinate absY(double localY) const {
        return m_originY + BigWorldCoordinate(localY);
    }
    BigWorldCoordinate absZ(double localZ) const {
        return m_originZ + BigWorldCoordinate(localZ);
    }

    // 触发重置，返回需要从 local 坐标减去的整步长
    double shiftOrigin(double& local, BigWorldCoordinate& origin) {
        if (std::abs(local) < STEP) return 0.0;
        int64_t step = (int64_t)(local / STEP);
        double offset = step * STEP;
        local -= offset;
        origin += BigWorldCoordinate((int64_t)offset);
        return offset;
    }

    // 同时处理 X 和 Z
    void update(double& px, double& pz) {
        shiftOrigin(px, m_originX);
        shiftOrigin(pz, m_originZ);
    }

    // 手动设置 Y 原点（传送时使用）
    void updateY(double& py) {
        shiftOrigin(py, m_originY);
    }
};

#endif
