#include "PerlinNoise.h"
#include "ImprovedNoise.h"
#include <cmath>

// ============================================================
// 构造函数 & 析构函数
// ============================================================

PerlinNoise::PerlinNoise(int levels) {
    _rndPtr = &_random;
    init(levels);
}

PerlinNoise::PerlinNoise(Random* random, int levels) {
    _rndPtr = random;
    init(levels);
}

PerlinNoise::~PerlinNoise() {
    for (int i = 0; i < levels; ++i)
        delete noiseLevels[i];
    delete[] noiseLevels;
}

void PerlinNoise::init(int levels) {
    this->levels = levels;
    noiseLevels = new ImprovedNoise*[levels];
    for (int i = 0; i < levels; i++) {
        noiseLevels[i] = new ImprovedNoise(_rndPtr);
    }
}

// ============================================================
// float 版 getValue（保留原版行为）
// ============================================================

float PerlinNoise::getValue(float x, float y) {
    return getValue(x, y, 0.0f);
}

float PerlinNoise::getValue(float x, float y) const {
    return getValue(x, y, 0.0f);
}

float PerlinNoise::getValue(float x, float y, float z) const {
    float value = 0.0f;
    float pow = 1.0f;
    for (int i = 0; i < levels; i++) {
        // 调用 ImprovedNoise::getValue(float, float, float) —— 原版 float 路径
        // 该路径不受 disableFringe 影响，保留边缘之地
        value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
        pow /= 2.0f;
    }
    return value;
}

// ============================================================
// double 版 getValueDouble（新增高精度接口）
// ============================================================

float PerlinNoise::getValueDouble(double x, double y) const {
    return getValueDouble(x, y, 0.0);
}

float PerlinNoise::getValueDouble(double x, double y, double z) const {
    float value = 0.0f;
    float pow = 1.0f;
    for (int i = 0; i < levels; i++) {
        // ⚠️ 关键：传入 double，匹配 ImprovedNoise::getValue(double, double, double)
        // 该函数已包含 OPTIONS_DISABLED_FRINGE_LANDS 判断逻辑
        value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
        pow /= 2.0f;
    }
    return value;
}

// ============================================================
// getRegion：批量生成噪声值（保持原样，内部调用 add 已改为 double 入口）
// ============================================================

float* PerlinNoise::getRegion(float* buffer, float x, float y, float z,
                              int xSize, int ySize, int zSize,
                              float xScale, float yScale, float zScale) {
    const int size = xSize * ySize * zSize;
    if (buffer == nullptr) {
        buffer = new float[size];
    }
    for (int i = 0; i < size; i++) {
        buffer[i] = 0.0f;
    }

    float pow = 1.0f;
    for (int i = 0; i < levels; i++) {
        // add 函数签名已改为接受 double，float 参数会隐式转换
        noiseLevels[i]->add(buffer,
                            (double)x, (double)y, (double)z,
                            xSize, ySize, zSize,
                            xScale * pow, yScale * pow, zScale * pow,
                            pow);
        pow /= 2.0f;
    }
    return buffer;
}

float* PerlinNoise::getRegion(float* sr, int x, int z,
                              int xSize, int zSize,
                              float xScale, float zScale, float pow) {
    return getRegion(sr, (float)x, 10.0f, (float)z,
                     xSize, 1, zSize,
                     xScale, 1.0f, zScale);
}

// ============================================================
// hashCode（用于调试/缓存）
// ============================================================

int PerlinNoise::hashCode() {
    int x = 4711;
    for (int i = 0; i < levels; ++i)
        x *= noiseLevels[i]->hashCode();
    return x;
}
