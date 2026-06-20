#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__PerlinNoise_H__

#include "../../../../util/Random.h"
#include "ImprovedNoise.h"

template<typename T>
class PerlinNoiseT : public SynthT<T>
{
public:
    PerlinNoiseT(int levels);
    PerlinNoiseT(Random* random, int levels);
    ~PerlinNoiseT();

    virtual T getValue(T x, T y) override;
    T getValue(T x, T y, T z);
    T getValue(T x, T y) const;
    T getValue(T x, T y, T z) const;

    T* getRegion(T* buffer, T x, T y, T z,
                 int xSize, int ySize, int zSize,
                 T xScale, T yScale, T zScale);
    T* getRegion(T* sr, int x, int z,
                 int xSize, int zSize,
                 T xScale, T zScale, T pow);

    int hashCode();

private:
    void init(int levels);

    ImprovedNoiseT<T>** noiseLevels;
    int levels;

    Random _random;
    Random* _rndPtr;
};

using PerlinNoise = PerlinNoiseT<double>;

#endif