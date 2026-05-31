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
    virtual double getValue(double x, double y) override;               // 非 const
double getValue(double x, double y, double z);                      // 非 const（可选）
double getValue(double x, double y) const;                          // const 版本
double getValue(double x, double y, double z) const;                // const 版本
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
