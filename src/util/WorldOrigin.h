#ifndef WORLDORIGIN_H__
#define WORLDORIGIN_H__

#include "WorldCoordinate.h"

class WorldOrigin {
    BigWorldCoordinate m_originX = 0;
    BigWorldCoordinate m_originY = 0;
    BigWorldCoordinate m_originZ = 0;

public:
    BigWorldCoordinate absX(double localX) const {
        return m_originX + BigWorldCoordinate(localX);
    }
    BigWorldCoordinate absY(double localY) const {
        return m_originY + BigWorldCoordinate(localY);
    }
    BigWorldCoordinate absZ(double localZ) const {
        return m_originZ + BigWorldCoordinate(localZ);
    }

    // 手动偏移（存档加载时用）
    void setOrigin(double ox, double oy, double oz) {
        m_originX = BigWorldCoordinate(ox);
        m_originY = BigWorldCoordinate(oy);
        m_originZ = BigWorldCoordinate(oz);
    }

    // 玩家传送时更新原点（不修改 local）
    void onTeleport(double oldLocal, double newLocal, BigWorldCoordinate& origin) {
        double delta = newLocal - oldLocal;
        double step = BIG_THRESHOLD;
        if (std::abs(delta) > step) {
            int64_t offset = (int64_t)(delta / step);
            origin += BigWorldCoordinate(offset * step);
        }
    }
};

#endif
