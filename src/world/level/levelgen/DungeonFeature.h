#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__DungeonFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__DungeonFeature_H__

#include "LargeFeature.h"
#include <cstdint>

class DungeonFeature: public LargeFeature {
public:
    // ✅
void addFeature(Level* level, int64_t x, int64_t z, int64_t xOffs, int64_t zOffs,
    unsigned char* blocks, int blocksSize) override;
protected:
    void addRoom(int xOffs, int zOffs, unsigned char* blocks, float xRoom, float yRoom, float zRoom);
    void addTunnel(int xOffs, int zOffs, unsigned char* blocks, float xCave, float yCave, float zCave,
                   float thickness, float yRot, float xRot, int step, int dist, float yScale);
};

#endif
