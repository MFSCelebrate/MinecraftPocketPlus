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

double PerlinNoise::getValue(double x, double y, double z) const {
    double value = 0;
    double pow = 1;
    for (int i = 0; i < levels; i++) {
        value += noiseLevels[i]->getValue(x * pow, y * pow, z * pow) / pow;
        pow /= 2;
    }
    return value;
}

double* PerlinNoise::getRegion( double* buffer, double x, double y, double z, int xSize, int ySize, int zSize, double xScale, double yScale, double zScale )
{
	const int size = xSize * ySize * zSize;
	if (buffer == 0) {
		buffer = new double[size];
	}
	for (int i = 0; i < size; i++)
		buffer[i] = 0;

	double pow = 1;

	for (int i = 0; i < levels; i++) {
		noiseLevels[i]->add(buffer, x, y, z, xSize, ySize, zSize, xScale * pow, yScale * pow, zScale * pow, pow);
		pow /= 2;
	}

	return buffer;
}

double* PerlinNoise::getRegion( double* sr, int x, int z, int xSize, int zSize, double xScale, double zScale, double pow )
{
	return getRegion(sr, (double)x, 10.0f, (double)z, xSize, 1, zSize, xScale, 1, zScale);
}

int PerlinNoise::hashCode() {
    int x = 4711;
    for (int i = 0; i < levels; ++i)
        x *= noiseLevels[i]->hashCode();
    return x;
}

