#ifndef NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__
#define NET_MINECRAFT_CLIENT_RENDERER_CULLING__FrustumCuller_H__

#include "FrustumData.h"
#include "Frustum.h"

class FrustumCuller: public Culler {
    
private:
    FrustumData frustum;
    float xOff, yOff, zOff;

public:
    FrustumCuller() {
        frustum = Frustum::getFrustum();
    }

    void prepare(float xOff, float yOff, float zOff) {
        this->xOff = xOff;
        this->yOff = yOff;
        this->zOff = zOff;
    }

    bool cubeFullyInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeFullyInFrustum(x0 - xOff, y0 - yOff, z0 - zOff, x1 - xOff, y1 - yOff, z1 - zOff);
    }

    bool cubeInFrustum(float x0, float y0, float z0, float x1, float y1, float z1) {
        return frustum.cubeInFrustum(x0 - xOff, y0 - yOff, z0 - zOff, x1 - xOff, y1 - yOff, z1 - zOff);
    }

    // 修改：接受 double 的 AABB，转换为 float 传递
    bool isVisible(const AABB& bb) {
        return cubeInFrustum((float)bb.x0, (float)bb.y0, (float)bb.z0,
                             (float)bb.x1, (float)bb.y1, (float)bb.z1);
    }
};

#endif
