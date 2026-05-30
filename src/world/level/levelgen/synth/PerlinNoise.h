#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__

//package net.minecraft.world.level.levelgen.synth;

#include "../../../../util/Random.h"
#include "Synth.h"

class ImprovedNoise;

class PerlinNoise: public Synth
{
public:
    PerlinNoise(int levels);
    PerlinNoise(Random* random, int levels);
	~PerlinNoise();

    double getValue(double x, double y) const;
    double getValue(double x, double y, double z) const;
    double getValue(double x, double y, double z) const;

    // 新增：非 const 版本，满足基类 Synth 纯虚函数
    inline double getValue(double x, double y) override {
        return static_cast<const PerlinNoise*>(this)->getValue(x, y);
    }
    inline double getValue(double x, double y, double z) {
        return static_cast<const PerlinNoise*>(this)->getValue(x, y, z);
    }
    inline double getValue(double x, double y, double z) {
        return static_cast<const PerlinNoise*>(this)->getValue(x, y, z);
	}

	//double[] getRegion(double[] buffer, double x, double y, double z, int xSize, int ySize, int zSize, double xScale, double yScale, double zScale) {
    double* getRegion(double* buffer, double x, double y, double z, int xSize, int ySize, int zSize, double xScale, double yScale, double zScale);
    double* getRegion(double* sr, int x, int z, int xSize, int zSize, double xScale, double zScale, double pow);

    int hashCode();

private:
    ImprovedNoise** noiseLevels;
    int levels;

    Random _random;
    Random* _rndPtr;

    void init(int levels);
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__*/
