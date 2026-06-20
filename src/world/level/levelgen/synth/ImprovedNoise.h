#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__ImprovedNoise_H__

#include "Synth.h"
#include <cstdint>

class Random;

template<typename T>
class ImprovedNoiseT : public SynthT<T>
{
public:
    ImprovedNoiseT();
    ImprovedNoiseT(Random* random);

    void init(Random* random);

    T noise(T _x, T _y, T _z) const;

    T lerp(T t, T a, T b) const;
    T grad2(int hash, T x, T z) const;
    T grad(int hash, T x, T y, T z) const;

    virtual T getValue(T x, T y) override;
    T getValue(T x, T y) const;
    T getValue(T x, T y, T z) const;

    void add(T* buffer, T _x, T _y, T _z,
             int xSize, int ySize, int zSize,
             T xs, T ys, T zs, T pow);

    int hashCode();

    T xo, yo, zo;
    int p[512];
};

using ImprovedNoise = ImprovedNoiseT<double>;

#endif