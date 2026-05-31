#include "ImprovedNoise.h"
#include "../../../../util/Random.h"
#include "../../../../client/Minecraft.h"
#include <cstdint>
#include <cmath>

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

double ImprovedNoise::noise( double _x, double _y, double _z ) const {
    double x = _x + xo;
    double y = _y + yo;
    double z = _z + zo;

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

    double u = x * x * x * (x * (x * 6 - 15) + 10);
    double v = y * y * y * (y * (y * 6 - 15) + 10);
    double w = z * z * z * (z * (z * 6 - 15) + 10);

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

const double ImprovedNoise::lerp( double t, double a, double b ) const {
    // 如果开启了 Progressive Farlands 选项，则禁用插值，直接返回 a
    if (Minecraft::instance && Minecraft::instance->options.getBooleanValue(OPTIONS_PROGRESSIVE_FARLANDS)) {
        return a;
    }
    return a + t * (b - a);
}

const double ImprovedNoise::grad2( int hash, double x, double z ) const {
    int h = hash & 15;
    double u = (1-((h&8)>>3))*x,
          v = h < 4 ? 0 : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

const double ImprovedNoise::grad( int hash, double x, double y, double z ) const {
    int h = hash & 15;
    double u = h < 8 ? x : y,
          v = h < 4 ? y : h == 12 || h == 14 ? x : z;
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double ImprovedNoise::getValue(double x, double y) {
    return static_cast<const ImprovedNoise*>(this)->getValue(x, y);
}

double ImprovedNoise::getValue(double x, double y) const {
    return getValue(x, y, 0.0);
}

double ImprovedNoise::getValue(double x, double y, double z) const {
    // 使用与 add_double 相同的 double 坐标处理，避免 int 截断
    double xd = x + xo;
    double yd = y + yo;
    double zd = z + zo;

    int X = ((int64_t)floor(xd)) & 255, Y = ((int64_t)floor(yd)) & 255, Z = ((int64_t)floor(zd)) & 255;
    xd -= floor(xd);
    yd -= floor(yd);
    zd -= floor(zd);

    // 保持原有边界 clamp（如果开启了 POSTPONED_FRINGE）
    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }
    if (doClamp) {
        if (x < 0.0) x = 0.0;
        if (x > 1.0) x = 1.0;
        if (y < 0.0) y = 0.0;
        if (y > 1.0) y = 1.0;
        if (z < 0.0) z = 0.0;
        if (z > 1.0) z = 1.0;
    }
    // … 这里省略 clamp 代码，可直接调用原先的 noise 逻辑但改为 double 参数
    // 为简便，直接调用 grad 和 lerp，它们已经是 double 操作，坐标部分已用 double 避免溢出
    double u = xd * xd * xd * (xd * (xd * 6 - 15) + 10);
    double v = yd * yd * yd * (yd * (yd * 6 - 15) + 10);
    double w = zd * zd * zd * (zd * (zd * 6 - 15) + 10);

    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z,
        B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return lerp(w, lerp(v, lerp(u, grad(p[AA], (double)xd, (double)yd, (double)zd),
                                   grad(p[BA], (double)(xd - 1), (double)yd, (double)zd)),
                           lerp(u, grad(p[AB], (double)xd, (double)(yd - 1), (double)zd),
                                   grad(p[BB], (double)(xd - 1), (double)(yd - 1), (double)zd))),
                   lerp(v, lerp(u, grad(p[AA + 1], (double)xd, (double)yd, (double)(zd - 1)),
                                   grad(p[BA + 1], (double)(xd - 1), (double)yd, (double)(zd - 1))),
                           lerp(u, grad(p[AB + 1], (double)xd, (double)(yd - 1), (double)(zd - 1)),
                                   grad(p[BB + 1], (double)(xd - 1), (double)(yd - 1), (double)(zd - 1)))));
}

// 以下是使用 32 位 int 坐标的 add 函数（原始行为）
static void add_int(ImprovedNoise* self, double* buffer, double _x, double _y, double _z, int xSize, int ySize, int zSize, double xs, double ys, double zs, double pow)
{
    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        double vv0 = 0, vv2 = 0;
        int pp = 0;
        double scale = 1.0f / pow;
        for (int xx = 0; xx < xSize; xx++) {
            double x = (_x + xx) * xs + self->xo;
            int xf = (int) x;
            if (x < xf) xf--;
            int X = xf & 255;
            x -= xf;
            if (doClamp) {
                if (x < 0.0f) x = 0.0f;
                if (x > 1.0f) x = 1.0f;
            }
            double u = x * x * x * (x * (x * 6 - 15) + 10);

            for (int zz = 0; zz < zSize; zz++) {
                double z = (_z + zz) * zs + self->zo;
                int zf = (int) z;
                if (z < zf) zf--;
                int Z = zf & 255;
                z -= zf;
                if (doClamp) {
                    if (z < 0.0f) z = 0.0f;
                    if (z > 1.0f) z = 1.0f;
                }
                double w = z * z * z * (z * (z * 6 - 15) + 10);

                A = self->p[X] + 0;
                AA = self->p[A] + Z;
                B = self->p[X + 1] + 0;
                BA = self->p[B] + Z;
                vv0 = self->lerp(u, self->grad2(self->p[AA], x, z), self->grad(self->p[BA], x - 1, 0, z));
                vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, 0, z - 1), self->grad(self->p[BA + 1], x - 1, 0, z - 1));

                double val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    double scale = 1 / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    double vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        double x = (_x + xx) * xs + self->xo;
        int xf = (int) x;
        if (x < xf) xf--;
        int X = xf & 255;
        x -= xf;
        if (doClamp) {
            if (x < 0.0f) x = 0.0f;
            if (x > 1.0f) x = 1.0f;
        }
        double u = x * x * x * (x * (x * 6 - 15) + 10);

        for (int zz = 0; zz < zSize; zz++) {
            double z = (_z + zz) * zs + self->zo;
            int zf = (int) z;
            if (z < zf) zf--;
            int Z = zf & 255;
            z -= zf;
            if (doClamp) {
                if (z < 0.0f) z = 0.0f;
                if (z > 1.0f) z = 1.0f;
            }
            double w = z * z * z * (z * (z * 6 - 15) + 10);

            for (int yy = 0; yy < ySize; yy++) {
                double y = (_y + yy) * ys + self->yo;
                int yf = (int) y;
                if (y < yf) yf--;
                int Y = yf & 255;
                y -= yf;
                if (doClamp) {
                    if (y < 0.0f) y = 0.0f;
                    if (y > 1.0f) y = 1.0f;
                }
                double v = y * y * y * (y * (y * 6 - 15) + 10);

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

                double v0 = self->lerp(v, vv0, vv1);
                double v1 = self->lerp(v, vv2, vv3);
                double val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// 使用 64 位 int64_t 坐标的 add 函数（无 32 位溢出，模拟无限世界）
static void add_int64(ImprovedNoise* self, double* buffer, double _x, double _y, double _z, int xSize, int ySize, int zSize, double xs, double ys, double zs, double pow)
{
    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        double vv0 = 0, vv2 = 0;
        int pp = 0;
        double scale = 1.0f / pow;
        for (int xx = 0; xx < xSize; xx++) {
            double x = (_x + xx) * xs + self->xo;
            int64_t xf = (int64_t)x;
            if (x < (double)xf) xf--;
            int X = (int)(xf & 255);
            x -= (double)xf;
            if (doClamp) {
                if (x < 0.0f) x = 0.0f;
                if (x > 1.0f) x = 1.0f;
            }
            double u = x * x * x * (x * (x * 6 - 15) + 10);

            for (int zz = 0; zz < zSize; zz++) {
                double z = (_z + zz) * zs + self->zo;
                int64_t zf = (int64_t)z;
                if (z < (double)zf) zf--;
                int Z = (int)(zf & 255);
                z -= (double)zf;
                if (doClamp) {
                    if (z < 0.0f) z = 0.0f;
                    if (z > 1.0f) z = 1.0f;
                }
                double w = z * z * z * (z * (z * 6 - 15) + 10);

                A = self->p[X] + 0;
                AA = self->p[A] + Z;
                B = self->p[X + 1] + 0;
                BA = self->p[B] + Z;
                vv0 = self->lerp(u, self->grad2(self->p[AA], x, z), self->grad(self->p[BA], x - 1, 0, z));
                vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, 0, z - 1), self->grad(self->p[BA + 1], x - 1, 0, z - 1));

                double val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    double scale = 1 / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    double vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        double x = (_x + xx) * xs + self->xo;
        int64_t xf = (int64_t)x;
        if (x < (double)xf) xf--;
        int X = (int)(xf & 255);
        x -= (double)xf;
        if (doClamp) {
            if (x < 0.0f) x = 0.0f;
            if (x > 1.0f) x = 1.0f;
        }
        double u = x * x * x * (x * (x * 6 - 15) + 10);

        for (int zz = 0; zz < zSize; zz++) {
            double z = (_z + zz) * zs + self->zo;
            int64_t zf = (int64_t)z;
            if (z < (double)zf) zf--;
            int Z = (int)(zf & 255);
            z -= (double)zf;
            if (doClamp) {
                if (z < 0.0f) z = 0.0f;
                if (z > 1.0f) z = 1.0f;
            }
            double w = z * z * z * (z * (z * 6 - 15) + 10);

            for (int yy = 0; yy < ySize; yy++) {
                double y = (_y + yy) * ys + self->yo;
                int64_t yf = (int64_t)y;
                if (y < (double)yf) yf--;
                int Y = (int)(yf & 255);
                y -= (double)yf;
                if (doClamp) {
                    if (y < 0.0f) y = 0.0f;
                    if (y > 1.0f) y = 1.0f;
                }
                double v = y * y * y * (y * (y * 6 - 15) + 10);

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

                double v0 = self->lerp(v, vv0, vv1);
                double v1 = self->lerp(v, vv2, vv3);
                double val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// 使用 double 坐标的 add 函数（避免整数溢出，利用 double 的大范围）
static void add_double(ImprovedNoise* self, double* buffer, double _x, double _y, double _z, int xSize, int ySize, int zSize, double xs, double ys, double zs, double pow)
{
    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        double vv0 = 0, vv2 = 0;
        int pp = 0;
        double scale = 1.0f / pow;
        for (int xx = 0; xx < xSize; xx++) {
            double x = (_x + xx) * xs + self->xo;
            double xf = floor(x);
            if (x < xf) xf -= 1.0;
            int X = ((int64_t)xf) & 255;
            double x_frac = x - xf;
            if (doClamp) {
                if (x_frac < 0.0) x_frac = 0.0;
                if (x_frac > 1.0) x_frac = 1.0;
            }
            double u = (double)(x_frac * x_frac * x_frac * (x_frac * (x_frac * 6 - 15) + 10));

            for (int zz = 0; zz < zSize; zz++) {
                double z = (_z + zz) * zs + self->zo;
                double zf = floor(z);
                if (z < zf) zf -= 1.0;
                int Z = ((int64_t)zf) & 255;
                double z_frac = z - zf;
                if (doClamp) {
                    if (z_frac < 0.0) z_frac = 0.0;
                    if (z_frac > 1.0) z_frac = 1.0;
                }
                double w = (double)(z_frac * z_frac * z_frac * (z_frac * (z_frac * 6 - 15) + 10));

                A = self->p[X] + 0;
                AA = self->p[A] + Z;
                B = self->p[X + 1] + 0;
                BA = self->p[B] + Z;
                vv0 = self->lerp(u, self->grad2(self->p[AA], (double)x_frac, (double)z_frac), self->grad(self->p[BA], (double)(x_frac - 1), 0, (double)z_frac));
                vv2 = self->lerp(u, self->grad(self->p[AA + 1], (double)x_frac, 0, (double)(z_frac - 1)), self->grad(self->p[BA + 1], (double)(x_frac - 1), 0, (double)(z_frac - 1)));

                double val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    double scale = 1 / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    double vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        double x = (_x + xx) * xs + self->xo;
        double xf = floor(x);
        if (x < xf) xf -= 1.0;
        int X = ((int64_t)xf) & 255;
        double x_frac = x - xf;
        if (doClamp) {
            if (x_frac < 0.0) x_frac = 0.0;
            if (x_frac > 1.0) x_frac = 1.0;
        }
        double u = (double)(x_frac * x_frac * x_frac * (x_frac * (x_frac * 6 - 15) + 10));

        for (int zz = 0; zz < zSize; zz++) {
            double z = (_z + zz) * zs + self->zo;
            double zf = floor(z);
            if (z < zf) zf -= 1.0;
            int Z = ((int64_t)zf) & 255;
            double z_frac = z - zf;
            if (doClamp) {
                if (z_frac < 0.0) z_frac = 0.0;
                if (z_frac > 1.0) z_frac = 1.0;
            }
            double w = (double)(z_frac * z_frac * z_frac * (z_frac * (z_frac * 6 - 15) + 10));

            for (int yy = 0; yy < ySize; yy++) {
                double y = (_y + yy) * ys + self->yo;
                double yf = floor(y);
                if (y < yf) yf -= 1.0;
                int Y = ((int64_t)yf) & 255;
                double y_frac = y - yf;
                if (doClamp) {
                    if (y_frac < 0.0) y_frac = 0.0;
                    if (y_frac > 1.0) y_frac = 1.0;
                }
                double v = (double)(y_frac * y_frac * y_frac * (y_frac * (y_frac * 6 - 15) + 10));

                if (yy == 0 || Y != yOld) {
                    yOld = Y;
                    A = self->p[X] + Y;
                    AA = self->p[A] + Z;
                    AB = self->p[A + 1] + Z;
                    B = self->p[X + 1] + Y;
                    BA = self->p[B] + Z;
                    BB = self->p[B + 1] + Z;
                    vv0 = self->lerp(u, self->grad(self->p[AA], (double)x_frac, (double)y_frac, (double)z_frac), self->grad(self->p[BA], (double)(x_frac - 1), (double)y_frac, (double)z_frac));
                    vv1 = self->lerp(u, self->grad(self->p[AB], (double)x_frac, (double)(y_frac - 1), (double)z_frac), self->grad(self->p[BB], (double)(x_frac - 1), (double)(y_frac - 1), (double)z_frac));
                    vv2 = self->lerp(u, self->grad(self->p[AA + 1], (double)x_frac, (double)y_frac, (double)(z_frac - 1)), self->grad(self->p[BA + 1], (double)(x_frac - 1), (double)y_frac, (double)(z_frac - 1)));
                    vv3 = self->lerp(u, self->grad(self->p[AB + 1], (double)x_frac, (double)(y_frac - 1), (double)(z_frac - 1)), self->grad(self->p[BB + 1], (double)(x_frac - 1), (double)(y_frac - 1), (double)(z_frac - 1)));
                }

                double v0 = self->lerp(v, vv0, vv1);
                double v1 = self->lerp(v, vv2, vv3);
                double val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// 原 add 函数：根据选项调用 32 位、64 位或 double 版本
void ImprovedNoise::add( double* buffer, double _x, double _y, double _z, int xSize, int ySize, int zSize, double xs, double ys, double zs, double pow )
{
    bool use64Bit = false;
    bool useDouble = false;
    if (Minecraft::instance) {
        use64Bit = Minecraft::instance->options.getBooleanValue(OPTIONS_SIXTYFOUR_FARLANDS);
        useDouble = Minecraft::instance->options.getBooleanValue(OPTIONS_DOUBLE_FARLANDS); // 需要添加此选项
    }
    if (useDouble) {
        add_double(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    } else if (use64Bit) {
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
