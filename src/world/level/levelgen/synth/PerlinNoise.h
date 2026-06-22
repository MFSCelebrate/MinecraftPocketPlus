#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__

#include "../../../../util/Random.h"
#include "Synth.h"

class ImprovedNoise;

class PerlinNoise : public Synth {
public:
    PerlinNoise(int levels);
    PerlinNoise(Random* random, int levels);
    ~PerlinNoise();

    // ---- Synth 虚函数实现（float 接口，保留原版调用） ----
    float getValue(float x, float y) override;
    float getValue(float x, float y) const;

    // ---- 原版 float 3D 接口（保留） ----
    float getValue(float x, float y, float z) const;

    // ---- 新增 double 接口（高精度，用于禁用边缘之地） ----
    float getValueDouble(double x, double y) const;
    float getValueDouble(double x, double y, double z) const;

    // ---- 批量生成接口（保持原样，内部已通过 add 传递 double） ----
    float* getRegion(float* buffer, float x, float y, float z,
                     int xSize, int ySize, int zSize,
                     float xScale, float yScale, float zScale);
    float* getRegion(float* sr, int x, int z,
                     int xSize, int zSize,
                     float xScale, float zScale, float pow);

    int hashCode();

private:
    void init(int levels);

    ImprovedNoise** noiseLevels;
    int levels;

    Random _random;
    Random* _rndPtr;
};

#endif
