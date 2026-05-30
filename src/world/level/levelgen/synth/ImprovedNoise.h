#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__

//package net.minecraft.world.level.levelgen.synth;

#include "Synth.h"
class Random;

class ImprovedNoise: public Synth
{
public:
    ImprovedNoise();

    ImprovedNoise(Random* random);
	
	void init(Random* random);

    double noise(double _x, double _y, double _z) const;

    const double lerp(double t, double a, double b) const;

    const double grad2(int hash, double x, double z) const;
    const double grad(int hash, double x, double y, double z) const;
    double getValue(double x, double y) override;          // 满足 Synth 纯虚函数
    double getValue(double x, double y) const;
    double getValue(double x, double y, double z) const;
    double getValue(double x, double y, double z) const;
    double getValue(double x, double y) const;          // 2D double 版本

    void add(double* buffer, double _x, double _y, double _z, int xSize, int ySize, int zSize, double xs, double ys, double zs, double pow);

    int hashCode();

	double scale;
	double xo, yo, zo;
    int p[512];
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__*/
