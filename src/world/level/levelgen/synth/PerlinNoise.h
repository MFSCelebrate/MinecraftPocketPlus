#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__

#include "../../../../util/Random.h"
#include "Synth.h"

class ImprovedNoise;

class PerlinNoise : public Synth
{
public:
    PerlinNoise(int levels);
    PerlinNoise(Random* random, int levels);
    ~PerlinNoise();

    // 实现 Synth 纯虚函数
    virtual double getValue(double x, double y) override;

    // 3D 噪声查询（const 版本，供内部使用）
    double getValue(double x, double y, double z) const;

    // 批量生成区域（double 缓冲区）
    double* getRegion(double* buffer, double x, double y, double z,
                      int xSize, int ySize, int zSize,
                      double xScale, double yScale, double zScale);
    double* getRegion(double* sr, int x, int z,
                      int xSize, int zSize,
                      double xScale, double zScale, double pow);

    int hashCode();

private:
    void init(int levels);

    ImprovedNoise** noiseLevels;
    int levels;

    Random _random;
    Random* _rndPtr;
};

#endif
