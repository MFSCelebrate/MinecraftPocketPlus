#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__

#include "FrustumData.h"
#include "Frustum.h"

class FrustumCuller: public Culler {
private:
    FrustumData frustum;

public:
    FrustumCuller() {
        frustum = Frustum::getFrustum();
    }

    // 保留接口兼容，但实际不再需要偏移
    void prepare(float xOff, float yOff, float zOff) {
        // Frustum 平面在计算时已包含相机平移，此处为空即可。
    }

    // 以下三个函数直接使用世界坐标，不减去任何偏移
    bool cubeFullyInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeFullyInFrustum(x0, y0, z0, x1, y1, z1);
    }

    bool cubeInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeInFrustum(x0, y0, z0, x1, y1, z1);
    }

    bool isVisible(const AABB& bb) {
        return cubeInFrustum((float)bb.x0, (float)bb.y0, (float)bb.z0,
                             (float)bb.x1, (float)bb.y1, (float)bb.z1);
    }
};

#endif
