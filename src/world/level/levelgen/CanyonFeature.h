#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__CanyonFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__CanyonFeature_H__

#include "LargeFeature.h"
#include <cstdint>

class CanyonFeature: public LargeFeature {
public:
    // 暴露 addFeature，允许外部直接调用
    void addFeature(Level* level, int64_t x, int64_t z, int64_t xOffs, int64_t zOffs,
                            unsigned char* blocks, int blocksSize) override;
protected:
    void addTunnel(int xOffs, int zOffs, unsigned char* blocks, float xCave, float yCave, float zCave,
                   float thickness, float yRot, float xRot, int step, int dist, float yScale);
};

#endif
