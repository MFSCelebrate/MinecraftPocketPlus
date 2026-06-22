#include "PerlinNoise.h"
#include "ImprovedNoise.h"

void PerlinNoise::init( int levels )
{
	this->levels = levels;
	noiseLevels = new ImprovedNoise* [levels];
	for (int i = 0; i < levels; i++) {
		noiseLevels[i] = new ImprovedNoise(_rndPtr);
	}
}

PerlinNoise::~PerlinNoise()
{
	for (int i = 0; i < levels; ++i)
		delete noiseLevels[i];
	delete[] noiseLevels;
}

PerlinNoise::PerlinNoise( int levels )
{
	_rndPtr = &_random;
	init(levels);
}

PerlinNoise::PerlinNoise( Random* random, int levels )
{
	_rndPtr = random;
	init(levels);
}

float PerlinNoise::getValue( float x, float y ) const {
	float value = 0;
	float pow = 1;

	for (int i = 0; i < levels; i++) {
		value += noiseLevels[i]->getValue(x * pow, y * pow) / pow;
		pow /= 2;
	}

	return value;
}

float PerlinNoise::getValue( float x, float y, float z ) const {
	float value = 0;
	float pow = 1;

	for (int i = 0; i < levels; i++) {
		value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
		pow /= 2;
	}

	return value;
}

// ===== double 重载（高精度表面噪声） =====
float PerlinNoise::getValue(double x, double y) const {
    return getValue(x, y, 0.0);
}

float PerlinNoise::getValue(double x, double y, double z) const {
    float value = 0.0f;
    float pow = 1.0f;
    for (int i = 0; i < levels; i++) {
        // ⚠️ 关键：x * pow 是 double，将匹配 ImprovedNoise::getValue(double, double, double)
        // 从而进入我们之前改造的高精度双路径（受 disableFringe 控制）
        value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
        pow /= 2.0f;
    }
    return value;
}

float* PerlinNoise::getRegion(float *buffer, double x, double y, double z,
                              int xSize, int ySize, int zSize,
                              float xScale, float yScale, float zScale) {
    const int size = xSize * ySize * zSize;
    if (buffer == nullptr) buffer = new float[size];
    for (int i = 0; i < size; i++) buffer[i] = 0;

    float pow = 1;
    for (int i = 0; i < levels; i++) {
        noiseLevels[i]->add(buffer, x, y, z, xSize, ySize, zSize,
                            xScale * pow, yScale * pow, zScale * pow, pow);
        pow /= 2;
    }
    return buffer;
}

float* PerlinNoise::getRegion( float* sr, double x, double z, int xSize, int zSize, float xScale, float zScale, float pow )
{
	return getRegion(sr, (double)x, 10.0f, (double)z, xSize, 1, zSize, xScale, 1, zScale);
}

int PerlinNoise::hashCode() {
    int x = 4711;
    for (int i = 0; i < levels; ++i)
        x *= noiseLevels[i]->hashCode();
    return x;
}

