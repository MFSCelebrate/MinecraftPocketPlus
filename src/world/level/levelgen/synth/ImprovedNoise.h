#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__

#include "Synth.h"
class Random;

class ImprovedNoise : public Synth
{
public:
    ImprovedNoise();
    ImprovedNoise(Random* random);

    void init(Random* random);

    // 核心噪声函数
    double noise(double _x, double _y, double _z) const;

    const double lerp(double t, double a, double b) const;
    const double grad2(int hash, double x, double z) const;
    const double grad(int hash, double x, double y, double z) const;

    // 实现 Synth 纯虚函数（2D）
    virtual double getValue(double x, double y) override;

    // const 版本的 2D 和 3D（内部调用）
    double getValue(double x, double y) const;
    double getValue(double x, double y, double z) const;

    // 批量添加噪声（double 缓冲区）
    void add(double* buffer, double _x, double _y, double _z,
             int xSize, int ySize, int zSize,
             double xs, double ys, double zs, double pow);

    int hashCode();

    double scale;
    double xo, yo, zo;
    int p[512];
};

#endif
