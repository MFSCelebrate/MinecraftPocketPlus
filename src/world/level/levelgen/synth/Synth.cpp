#include "Synth.h"

Synth::~Synth()
{

}

int Synth::getDataSize( int width, int height )
{
	return width * height * sizeof(double);
}

void Synth::create( int width, int height, double* result )
{
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			result[x + y * width] = getValue((double)x, (double)y);
		}
	}
}

