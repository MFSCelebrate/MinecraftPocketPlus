#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__Feature_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__Feature_H__

//package net.minecraft.world.level.levelgen.feature;

#include "../../Level.h"
#include <cstdint>
class Random;

class Feature
{
public:
	Feature(bool doUpdate = false);
    virtual ~Feature() {}
    virtual bool place(Level* level, Random* random, int64_t x, int y, int64_t z) = 0;
    virtual void init(float v1, float v2, float v3) {}
protected:
	void placeBlock(Level* level, int64_t x, int y, int64_t z, int tile);
	void placeBlock(Level* level, int64_t x, int y, int64_t z, int tile, int data);
private:
	bool doUpdate;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_FEATURE__Feature_H__*/
