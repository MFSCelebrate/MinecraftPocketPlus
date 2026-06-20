#include "PerlinNoise.h"
#include <cstdlib>

// ============ 构造/初始化 ============

template<typename T>
void PerlinNoiseT<T>::init(int levels)
{
    this->levels = levels;
    noiseLevels = new ImprovedNoiseT<T>*[levels];
    for (int i = 0; i < levels; i++) {
        noiseLevels[i] = new ImprovedNoiseT<T>(_rndPtr);
    }
}

template<typename T>
PerlinNoiseT<T>::~PerlinNoiseT()
{
    for (int i = 0; i < levels; ++i)
        delete noiseLevels[i];
    delete[] noiseLevels;
}

template<typename T>
PerlinNoiseT<T>::PerlinNoiseT(int levels)
{
    _rndPtr = &_random;
    init(levels);
}

template<typename T>
PerlinNoiseT<T>::PerlinNoiseT(Random* random, int levels)
{
    _rndPtr = random;
    init(levels);
}

// ============ getValue — 非 const 2D ============

template<typename T>
T PerlinNoiseT<T>::getValue(T x, T y) {
    T value = 0;
    T pow = 1;
    for (int i = 0; i < levels; i++) {
        value += noiseLevels[i]->getValue(x * pow, y * pow) / pow;
        pow /= 2;
    }
    return value;
}

// ============ getValue — 非 const 3D ============

template<typename T>
T PerlinNoiseT<T>::getValue(T x, T y, T z) {
    T value = 0;
    T pow = 1;
    for (int i = 0; i < levels; i++) {
        value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
        pow /= 2;
    }
    return value;
}

// ============ getValue — const 2D ============

template<typename T>
T PerlinNoiseT<T>::getValue(T x, T y) const {
    T value = 0;
    T pow = 1;
    for (int i = 0; i < levels; i++) {
        value += noiseLevels[i]->getValue(x * pow, y * pow) / pow;
        pow /= 2;
    }
    return value;
}

// ============ getValue — const 3D ============

template<typename T>
T PerlinNoiseT<T>::getValue(T x, T y, T z) const {
    T value = 0;
    T pow = 1;
    for (int i = 0; i < levels; i++) {
        value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
        pow /= 2;
    }
    return value;
}

// ============ getRegion — 3D ============

template<typename T>
T* PerlinNoiseT<T>::getRegion(T* buffer, T x, T y, T z,
                               int xSize, int ySize, int zSize,
                               T xScale, T yScale, T zScale)
{
    const int size = xSize * ySize * zSize;
    if (buffer == 0) {
        buffer = new T[size];
    }
    for (int i = 0; i < size; i++)
        buffer[i] = 0;

    T pow = 1;

    for (int i = 0; i < levels; i++) {
        noiseLevels[i]->add(buffer, x, y, z, xSize, ySize, zSize,
                            xScale * pow, yScale * pow, zScale * pow, pow);
        pow /= 2;
    }

    return buffer;
}

// ============ getRegion — 2D ============

template<typename T>
T* PerlinNoiseT<T>::getRegion(T* sr, int x, int z,
                               int xSize, int zSize,
                               T xScale, T zScale, T pow)
{
    return getRegion(sr, (T)x, (T)10.0, (T)z, xSize, 1, zSize, xScale, (T)1, zScale);
}

// ============ hashCode ============

template<typename T>
int PerlinNoiseT<T>::hashCode() {
    int x = 4711;
    for (int i = 0; i < levels; ++i)
        x *= noiseLevels[i]->hashCode();
    return x;
}

// ============ 显式实例化 ============

template class PerlinNoiseT<double>;
template class PerlinNoiseT<float>;