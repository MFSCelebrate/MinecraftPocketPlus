#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__

#include "Synth.h"
#include "../../../../util/Random.h"

class ImprovedNoise : public Synth {
public:
    ImprovedNoise();
    ImprovedNoise(Random* random);

    void init(Random* random);

    // ---- 原版 float 接口（保留） ----
    float noise(float x, float y, float z) const;
    float getValue(float x, float y) override;
    float getValue(float x, float y) const;
    float getValue(float x, float y, float z) const;

    // ---- 新增 double 接口 ----
    double getValue(double x, double y, double z) const;
    double getValue(double x, double y) const;

    // ---- 核心数学函数（重载 double 版本） ----
    double lerp(double t, double a, double b) const;
    double grad2(int hash, double x, double z) const;
    double grad(int hash, double x, double y, double z) const;

    // ---- 保留原 float 版本（供原版调用） ----
    float lerp(float t, float a, float b) const;
    float grad2(int hash, float x, float z) const;
    float grad(int hash, float x, float y, float z) const;

    void add(float* buffer, double _x, double _y, double _z,
             int xSize, int ySize, int zSize,
             float xs, float ys, float zs, float pow);

    int hashCode();

    float xo, yo, zo;
    int p[512];
};

#endif
