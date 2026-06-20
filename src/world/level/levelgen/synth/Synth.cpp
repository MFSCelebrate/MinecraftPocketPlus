#include "Synth.h"

template<typename T>
SynthT<T>::~SynthT() {}

template<typename T>
int SynthT<T>::getDataSize(int width, int height) {
    return width * height * sizeof(T);
}

template<typename T>
void SynthT<T>::create(int width, int height, T* result) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            result[x + y * width] = getValue((T)x, (T)y);
        }
    }
}

// 显式实例化
template class SynthT<double>;
template class SynthT<float>;