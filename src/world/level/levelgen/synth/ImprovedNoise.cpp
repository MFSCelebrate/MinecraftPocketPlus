#include "ImprovedNoise.h"
#include "../../../../util/Random.h"
#include "../../../../client/Minecraft.h"
#include <cstdint>

ImprovedNoise::ImprovedNoise()
{
    Random random(1);
    init(&random);
}

ImprovedNoise::ImprovedNoise( Random* random )
{
    init(random);
}

void ImprovedNoise::init( Random* random )
{
    xo = random->nextFloat() * 256.f;
    yo = random->nextFloat() * 256.f;
    zo = random->nextFloat() * 256.f;
    for (int i = 0; i < 256; i++) {
        p[i] = i;
    }

    for (int i = 0; i < 256; i++) {
        int j = random->nextInt(256 - i) + i;
        int tmp = p[i];
        p[i] = p[j];
        p[j] = tmp;

        p[i + 256] = p[i];
    }
}

float ImprovedNoise::noise( float _x, float _y, float _z )
{
    float x = _x + xo;
    float y = _y + yo;
    float z = _z + zo;

    int xf = (int) x;
    int yf = (int) y;
    int zf = (int) z;

    if (x < xf) xf--;
    if (y < yf) yf--;
    if (z < zf) zf--;

    int X = xf & 255,
        Y = yf & 255,
        Z = zf & 255;

    x -= xf;
    y -= yf;
    z -= zf;

    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }
    if (doClamp) {
        if (x < 0.0f) x = 0.0f;
        if (x > 1.0f) x = 1.0f;
        if (y < 0.0f) y = 0.0f;
        if (y > 1.0f) y = 1.0f;
        if (z < 0.0f) z = 0.0f;
        if (z > 1.0f) z = 1.0f;
    }

    float u = x * x * x * (x * (x * 6 - 15) + 10);
    float v = y * y * y * (y * (y * 6 - 15) + 10);
    float w = z * z * z * (z * (z * 6 - 15) + 10);

    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z,
        B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return lerp(w, lerp(v, lerp(u, grad(p[AA], x, y, z),
                                 grad(p[BA], x - 1, y, z)),
                         lerp(u, grad(p[AB], x, y - 1, z),
                                 grad(p[BB], x - 1, y - 1, z))),
                 lerp(v, lerp(u, grad(p[AA + 1], x, y, z - 1),
                                 grad(p[BA + 1], x - 1, y, z - 1)),
                         lerp(u, grad(p[AB + 1], x, y - 1, z - 1),
                                 grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

const float ImprovedNoise::lerp( float t, float a, float b )
{
    // 如果开启了 Progressive Farlands 选项，则禁用插值，直接返回 a
    if (Minecraft::instance && Minecraft::instance->options.getBooleanValue(OPTIONS_PROGRESSIVE_FARLANDS)) {
        return a;
    }
    return a + t * (b - a);
}

const float ImprovedNoise::grad2( int hash, float x, float z )
{
    int h = hash & 15;
    float u = (1-((h&8)>>3))*x,
          v = h < 4 ? 0 : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

const float ImprovedNoise::grad( int hash, float x, float y, float z )
{
    int h = hash & 15;
    float u = h < 8 ? x : y,
          v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float ImprovedNoise::getValue( float x, float y )
{
    return noise(x, y, 0);
}

float ImprovedNoise::getValue( float x, float y, float z )
{
    return noise(x, y, z);
}

// 以下是使用 32 位 int 坐标的 add 函数（原始行为）
static void add_int(ImprovedNoise* self, float* buffer, float _x, float _y, float _z, int xSize, int ySize, int zSize, float xs, float ys, float zs, float pow)
{
    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        float vv0 = 0, vv2 = 0;
        int pp = 0;
        float scale = 1.0f / pow;
        for (int xx = 0; xx < xSize; xx++) {
            float x = (_x + xx) * xs + self->xo;
            int xf = (int) x;
            if (x < xf) xf--;
            int X = xf & 255;
            x -= xf;
            if (doClamp) {
                if (x < 0.0f) x = 0.0f;
                if (x > 1.0f) x = 1.0f;
            }
            float u = x * x * x * (x * (x * 6 - 15) + 10);

            for (int zz = 0; zz < zSize; zz++) {
                float z = (_z + zz) * zs + self->zo;
                int zf = (int) z;
                if (z < zf) zf--;
                int Z = zf & 255;
                z -= zf;
                if (doClamp) {
                    if (z < 0.0f) z = 0.0f;
                    if (z > 1.0f) z = 1.0f;
                }
                float w = z * z * z * (z * (z * 6 - 15) + 10);

                A = self->p[X] + 0;
                AA = self->p[A] + Z;
                B = self->p[X + 1] + 0;
                BA = self->p[B] + Z;
                vv0 = self->lerp(u, self->grad2(self->p[AA], x, z), self->grad(self->p[BA], x - 1, 0, z));
                vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, 0, z - 1), self->grad(self->p[BA + 1], x - 1, 0, z - 1));

                float val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    float scale = 1 / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    float vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        float x = (_x + xx) * xs + self->xo;
        int xf = (int) x;
        if (x < xf) xf--;
        int X = xf & 255;
        x -= xf;
        if (doClamp) {
            if (x < 0.0f) x = 0.0f;
            if (x > 1.0f) x = 1.0f;
        }
        float u = x * x * x * (x * (x * 6 - 15) + 10);

        for (int zz = 0; zz < zSize; zz++) {
            float z = (_z + zz) * zs + self->zo;
            int zf = (int) z;
            if (z < zf) zf--;
            int Z = zf & 255;
            z -= zf;
            if (doClamp) {
                if (z < 0.0f) z = 0.0f;
                if (z > 1.0f) z = 1.0f;
            }
            float w = z * z * z * (z * (z * 6 - 15) + 10);

            for (int yy = 0; yy < ySize; yy++) {
                float y = (_y + yy) * ys + self->yo;
                int yf = (int) y;
                if (y < yf) yf--;
                int Y = yf & 255;
                y -= yf;
                if (doClamp) {
                    if (y < 0.0f) y = 0.0f;
                    if (y > 1.0f) y = 1.0f;
                }
                float v = y * y * y * (y * (y * 6 - 15) + 10);

                if (yy == 0 || Y != yOld) {
                    yOld = Y;
                    A = self->p[X] + Y;
                    AA = self->p[A] + Z;
                    AB = self->p[A + 1] + Z;
                    B = self->p[X + 1] + Y;
                    BA = self->p[B] + Z;
                    BB = self->p[B + 1] + Z;
                    vv0 = self->lerp(u, self->grad(self->p[AA], x, y, z), self->grad(self->p[BA], x - 1, y, z));
                    vv1 = self->lerp(u, self->grad(self->p[AB], x, y - 1, z), self->grad(self->p[BB], x - 1, y - 1, z));
                    vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, y, z - 1), self->grad(self->p[BA + 1], x - 1, y, z - 1));
                    vv3 = self->lerp(u, self->grad(self->p[AB + 1], x, y - 1, z - 1), self->grad(self->p[BB + 1], x - 1, y - 1, z - 1));
                }

                float v0 = self->lerp(v, vv0, vv1);
                float v1 = self->lerp(v, vv2, vv3);
                float val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// 使用 64 位 int64_t 坐标的 add 函数（无 32 位溢出，模拟无限世界）
static void add_int64(ImprovedNoise* self, float* buffer, float _x, float _y, float _z, int xSize, int ySize, int zSize, float xs, float ys, float zs, float pow)
{
    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        float vv0 = 0, vv2 = 0;
        int pp = 0;
        float scale = 1.0f / pow;
        for (int xx = 0; xx < xSize; xx++) {
            float x = (_x + xx) * xs + self->xo;
            int64_t xf = (int64_t)x;
            if (x < (float)xf) xf--;
            int X = (int)(xf & 255);
            x -= (float)xf;
            if (doClamp) {
                if (x < 0.0f) x = 0.0f;
                if (x > 1.0f) x = 1.0f;
            }
            float u = x * x * x * (x * (x * 6 - 15) + 10);

            for (int zz = 0; zz < zSize; zz++) {
                float z = (_z + zz) * zs + self->zo;
                int64_t zf = (int64_t)z;
                if (z < (float)zf) zf--;
                int Z = (int)(zf & 255);
                z -= (float)zf;
                if (doClamp) {
                    if (z < 0.0f) z = 0.0f;
                    if (z > 1.0f) z = 1.0f;
                }
                float w = z * z * z * (z * (z * 6 - 15) + 10);

                A = self->p[X] + 0;
                AA = self->p[A] + Z;
                B = self->p[X + 1] + 0;
                BA = self->p[B] + Z;
                vv0 = self->lerp(u, self->grad2(self->p[AA], x, z), self->grad(self->p[BA], x - 1, 0, z));
                vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, 0, z - 1), self->grad(self->p[BA + 1], x - 1, 0, z - 1));

                float val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    float scale = 1 / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    float vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        float x = (_x + xx) * xs + self->xo;
        int64_t xf = (int64_t)x;
        if (x < (float)xf) xf--;
        int X = (int)(xf & 255);
        x -= (float)xf;
        if (doClamp) {
            if (x < 0.0f) x = 0.0f;
            if (x > 1.0f) x = 1.0f;
        }
        float u = x * x * x * (x * (x * 6 - 15) + 10);

        for (int zz = 0; zz < zSize; zz++) {
            float z = (_z + zz) * zs + self->zo;
            int64_t zf = (int64_t)z;
            if (z < (float)zf) zf--;
            int Z = (int)(zf & 255);
            z -= (float)zf;
            if (doClamp) {
                if (z < 0.0f) z = 0.0f;
                if (z > 1.0f) z = 1.0f;
            }
            float w = z * z * z * (z * (z * 6 - 15) + 10);

            for (int yy = 0; yy < ySize; yy++) {
                float y = (_y + yy) * ys + self->yo;
                int64_t yf = (int64_t)y;
                if (y < (float)yf) yf--;
                int Y = (int)(yf & 255);
                y -= (float)yf;
                if (doClamp) {
                    if (y < 0.0f) y = 0.0f;
                    if (y > 1.0f) y = 1.0f;
                }
                float v = y * y * y * (y * (y * 6 - 15) + 10);

                if (yy == 0 || Y != yOld) {
                    yOld = Y;
                    A = self->p[X] + Y;
                    AA = self->p[A] + Z;
                    AB = self->p[A + 1] + Z;
                    B = self->p[X + 1] + Y;
                    BA = self->p[B] + Z;
                    BB = self->p[B + 1] + Z;
                    vv0 = self->lerp(u, self->grad(self->p[AA], x, y, z), self->grad(self->p[BA], x - 1, y, z));
                    vv1 = self->lerp(u, self->grad(self->p[AB], x, y - 1, z), self->grad(self->p[BB], x - 1, y - 1, z));
                    vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, y, z - 1), self->grad(self->p[BA + 1], x - 1, y, z - 1));
                    vv3 = self->lerp(u, self->grad(self->p[AB + 1], x, y - 1, z - 1), self->grad(self->p[BB + 1], x - 1, y - 1, z - 1));
                }

                float v0 = self->lerp(v, vv0, vv1);
                float v1 = self->lerp(v, vv2, vv3);
                float val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// 原 add 函数：根据选项调用 32 位或 64 位版本
void ImprovedNoise::add( float* buffer, float _x, float _y, float _z, int xSize, int ySize, int zSize, float xs, float ys, float zs, float pow )
{
    bool use64Bit = false;
    if (Minecraft::instance) {
        use64Bit = Minecraft::instance->options.getBooleanValue(OPTIONS_SIXTYFOUR_FARLANDS);
    }
    if (use64Bit) {
        add_int64(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    } else {
        add_int(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    }
}

int ImprovedNoise::hashCode() {
    int x = 4711;
    for (int i = 0; i < 512; ++i)
        x = x * 37 + p[i];
    return x;
}
