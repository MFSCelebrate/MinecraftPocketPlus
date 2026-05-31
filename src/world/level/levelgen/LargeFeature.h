#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeFeature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeFeature_H__

//package net.minecraft.world.level.levelgen;

#include "../../../util/Random.h"
#include <cstdint>

class Random;
class Level;
class ChunkSource;

class LargeFeature
{
public:
	LargeFeature();
	virtual ~LargeFeature();

    virtual void apply(ChunkSource* chunkSource, Level* level, int64_t xOffs, int64_t zOffs, unsigned char* blocks, int blocksSize);

protected:
    virtual void addFeature(Level* level, int64_t x, int64_t z, int64_t xOffs, int64_t zOffs, unsigned char* blocks, int blocksSize) = 0;

	int radius;
    Random random;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__LargeFeature_H__*/
