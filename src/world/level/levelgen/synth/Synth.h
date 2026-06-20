#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__Synth_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN_SYNTH__Synth_H__

template<typename T>
class SynthT
{
public:
    virtual ~SynthT();

    int getDataSize(int width, int height);

    virtual T getValue(T x, T y) = 0;

    void create(int width, int height, T* result);
};

using Synth = SynthT<double>;

#endif