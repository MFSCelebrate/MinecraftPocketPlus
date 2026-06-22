#include "ImprovedNoise.h"
#include "../../../../util/Random.h"
#include "../../../../client/Minecraft.h"
#include <cmath>
#include <cstdint>

ImprovedNoise::ImprovedNoise() {
    Random random(1);
    init(&random);
}

ImprovedNoise::ImprovedNoise(Random* random) {
    init(random);
}

void ImprovedNoise::init(Random* random) {
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

// ===== Float 版本 =====
float ImprovedNoise::lerp(float t, float a, float b) const {
        // 如果开启了 Progressive Farlands 选项，则禁用插值，直接返回 a
    if (Minecraft::instance && Minecraft::instance->options.getBooleanValue(OPTIONS_PROGRESSIVE_FARLANDS)) {
        return a;
    }
    return a + t * (b - a);
}

float ImprovedNoise::grad2(int hash, float x, float z) const {
    int h = hash & 15;
    float u = (1 - ((h & 8) >> 3)) * x;
    float v = h < 4 ? 0 : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

float ImprovedNoise::grad(int hash, float x, float y, float z) const {
    int h = hash & 15;
    float u = h < 8 ? x : y;
    float v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// ===== Double 版本（用于禁用边缘之地） =====
double ImprovedNoise::lerp(double t, double a, double b) const {
        // 如果开启了 Progressive Farlands 选项，则禁用插值，直接返回 a
    if (Minecraft::instance && Minecraft::instance->options.getBooleanValue(OPTIONS_PROGRESSIVE_FARLANDS)) {
        return a;
    }
    return a + t * (b - a);
}

double ImprovedNoise::grad2(int hash, double x, double z) const {
    int h = hash & 15;
    double u = (1 - ((h & 8) >> 3)) * x;
    double v = h < 4 ? 0 : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

double ImprovedNoise::grad(int hash, double x, double y, double z) const {
    int h = hash & 15;
    double u = h < 8 ? x : y;
    double v = h < 4 ? y : (h == 12 || h == 14 ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// ===== noise(float) =====
float ImprovedNoise::noise(float _x, float _y, float _z) const {
    bool disableFringe = false;
    bool doClamp = false;
    if (Minecraft::instance) {
        disableFringe = Minecraft::instance->options.getBooleanValue(OPTIONS_DISABLED_FRINGE_LANDS);
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    // 提升为 double 以便后续计算
    double x = (double)_x + xo;
    double y = (double)_y + yo;
    double z = (double)_z + zo;

    // 整数溢出（保留）
    int xfi = (int)x;
    if (x < (double)xfi) xfi--;
    int X = xfi & 255;
    double dx = x - (double)xfi;

    int yfi = (int)y;
    if (y < (double)yfi) yfi--;
    int Y = yfi & 255;
    double dy = y - (double)yfi;

    int zfi = (int)z;
    if (z < (double)zfi) zfi--;
    int Z = zfi & 255;
    double dz = z - (double)zfi;

    // 原有的 doClamp 钳位（由 OPTIONS_POSTPONED_FRINGE 控制）
    if (doClamp) {
        if (dx < 0.0) dx = 0.0;
        if (dx > 1.0) dx = 1.0;
        if (dy < 0.0) dy = 0.0;
        if (dy > 1.0) dy = 1.0;
        if (dz < 0.0) dz = 0.0;
        if (dz > 1.0) dz = 1.0;
    }

    // 根据 disableFringe 选择计算路径
    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    double val;
    if (disableFringe) {
        // 完全 double 路径
        double u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);
        double v = dy * dy * dy * (dy * (dy * 6 - 15) + 10);
        double w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);

        val = lerp(w,
            lerp(v,
                lerp(u, grad(p[AA], dx, dy, dz), grad(p[BA], dx - 1.0, dy, dz)),
                lerp(u, grad(p[AB], dx, dy - 1.0, dz), grad(p[BB], dx - 1.0, dy - 1.0, dz))
            ),
            lerp(v,
                lerp(u, grad(p[AA + 1], dx, dy, dz - 1.0), grad(p[BA + 1], dx - 1.0, dy, dz - 1.0)),
                lerp(u, grad(p[AB + 1], dx, dy - 1.0, dz - 1.0), grad(p[BB + 1], dx - 1.0, dy - 1.0, dz - 1.0))
            )
        );
    } else {
        // 原版 float 路径（保留精度丢失）
        float dxf = (float)dx;
        float dyf = (float)dy;
        float dzf = (float)dz;
        float u = dxf * dxf * dxf * (dxf * (dxf * 6 - 15) + 10);
        float v = dyf * dyf * dyf * (dyf * (dyf * 6 - 15) + 10);
        float w = dzf * dzf * dzf * (dzf * (dzf * 6 - 15) + 10);

        float valf = lerp(w,
            lerp(v,
                lerp(u, grad(p[AA], dxf, dyf, dzf), grad(p[BA], dxf - 1, dyf, dzf)),
                lerp(u, grad(p[AB], dxf, dyf - 1, dzf), grad(p[BB], dxf - 1, dyf - 1, dzf))
            ),
            lerp(v,
                lerp(u, grad(p[AA + 1], dxf, dyf, dzf - 1), grad(p[BA + 1], dxf - 1, dyf, dzf - 1)),
                lerp(u, grad(p[AB + 1], dxf, dyf - 1, dzf - 1), grad(p[BB + 1], dxf - 1, dyf - 1, dzf - 1))
            )
        );
        val = (double)valf;
    }
    return (float)val;
}

// ===== getValue(float, float) =====
float ImprovedNoise::getValue(float x, float y) {
    return getValue(x, y, 0.0f);
}

float ImprovedNoise::getValue(float x, float y) const {
    return getValue(x, y, 0.0f);
}

float ImprovedNoise::getValue(float x, float y, float z) const {
    return noise(x, y, z);
}

// ===== getValue(double, double) =====
double ImprovedNoise::getValue(double x, double y) const {
    return getValue(x, y, 0.0);
}

// ===== getValue(double, double, double) =====
double ImprovedNoise::getValue(double x, double y, double z) const {
    bool disableFringe = false;
    bool doClamp = false;
    if (Minecraft::instance) {
        disableFringe = Minecraft::instance->options.getBooleanValue(OPTIONS_DISABLED_FRINGE_LANDS);
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    // 直接使用 double 坐标
    double xd = x + xo;
    double yd = y + yo;
    double zd = z + zo;

    int xfi = (int)xd;
    if (xd < (double)xfi) xfi--;
    int X = xfi & 255;
    double dx = xd - (double)xfi;

    int yfi = (int)yd;
    if (yd < (double)yfi) yfi--;
    int Y = yfi & 255;
    double dy = yd - (double)yfi;

    int zfi = (int)zd;
    if (zd < (double)zfi) zfi--;
    int Z = zfi & 255;
    double dz = zd - (double)zfi;

    if (doClamp) {
        if (dx < 0.0) dx = 0.0;
        if (dx > 1.0) dx = 1.0;
        if (dy < 0.0) dy = 0.0;
        if (dy > 1.0) dy = 1.0;
        if (dz < 0.0) dz = 0.0;
        if (dz > 1.0) dz = 1.0;
    }

    int A = p[X] + Y;
    int AA = p[A] + Z;
    int AB = p[A + 1] + Z;
    int B = p[X + 1] + Y;
    int BA = p[B] + Z;
    int BB = p[B + 1] + Z;

    double val;
    if (disableFringe) {
        double u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);
        double v = dy * dy * dy * (dy * (dy * 6 - 15) + 10);
        double w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);

        val = lerp(w,
            lerp(v,
                lerp(u, grad(p[AA], dx, dy, dz), grad(p[BA], dx - 1.0, dy, dz)),
                lerp(u, grad(p[AB], dx, dy - 1.0, dz), grad(p[BB], dx - 1.0, dy - 1.0, dz))
            ),
            lerp(v,
                lerp(u, grad(p[AA + 1], dx, dy, dz - 1.0), grad(p[BA + 1], dx - 1.0, dy, dz - 1.0)),
                lerp(u, grad(p[AB + 1], dx, dy - 1.0, dz - 1.0), grad(p[BB + 1], dx - 1.0, dy - 1.0, dz - 1.0))
            )
        );
    } else {
        float dxf = (float)dx;
        float dyf = (float)dy;
        float dzf = (float)dz;
        float u = dxf * dxf * dxf * (dxf * (dxf * 6 - 15) + 10);
        float v = dyf * dyf * dyf * (dyf * (dyf * 6 - 15) + 10);
        float w = dzf * dzf * dzf * (dzf * (dzf * 6 - 15) + 10);

        float valf = lerp(w,
            lerp(v,
                lerp(u, grad(p[AA], dxf, dyf, dzf), grad(p[BA], dxf - 1, dyf, dzf)),
                lerp(u, grad(p[AB], dxf, dyf - 1, dzf), grad(p[BB], dxf - 1, dyf - 1, dzf))
            ),
            lerp(v,
                lerp(u, grad(p[AA + 1], dxf, dyf, dzf - 1), grad(p[BA + 1], dxf - 1, dyf, dzf - 1)),
                lerp(u, grad(p[AB + 1], dxf, dyf - 1, dzf - 1), grad(p[BB + 1], dxf - 1, dyf - 1, dzf - 1))
            )
        );
        val = (double)valf;
    }
    return val;
}

// ===== add 入口 =====
void ImprovedNoise::add(float* buffer, double _x, double _y, double _z,
                         int xSize, int ySize, int zSize,
                         float xs, float ys, float zs, float pow) {
    bool use64Bit = false;
    bool useDouble = false;
    if (Minecraft::instance) {
        use64Bit = Minecraft::instance->options.getBooleanValue(OPTIONS_SIXTYFOUR_FARLANDS);
        useDouble = Minecraft::instance->options.getBooleanValue(OPTIONS_DOUBLE_FARLANDS);
    }
    if (useDouble) {
        // add_double 本身就是 double 计算，无需改动
        add_double(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    } else if (use64Bit) {
        add_int64(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    } else {
        add_int(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    }
}

// ===== add_int（完整实现） =====
static void add_int(ImprovedNoise* self, float* buffer,
                    double _x, double _y, double _z,
                    int xSize, int ySize, int zSize,
                    float xs, float ys, float zs, float pow) {
    bool doClamp = false;
    bool disableFringe = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
        disableFringe = Minecraft::instance->options.getBooleanValue(OPTIONS_DISABLED_FRINGE_LANDS);
    }

    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        float vv0 = 0, vv2 = 0;
        int pp = 0;
        float scale = 1.0f / pow;
        for (int xx = 0; xx < xSize; xx++) {
            double x = (_x + xx) * (double)xs + self->xo;
            int xf = (int)x;
            if (x < (double)xf) xf--;
            int X = xf & 255;
            double dx = x - (double)xf;
            if (doClamp) {
                if (dx < 0.0) dx = 0.0;
                if (dx > 1.0) dx = 1.0;
            }

            double u;
            if (disableFringe) {
                u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);
            } else {
                float dxf = (float)dx;
                u = (double)(dxf * dxf * dxf * (dxf * (dxf * 6 - 15) + 10));
            }

            for (int zz = 0; zz < zSize; zz++) {
                double z = (_z + zz) * (double)zs + self->zo;
                int zf = (int)z;
                if (z < (double)zf) zf--;
                int Z = zf & 255;
                double dz = z - (double)zf;
                if (doClamp) {
                    if (dz < 0.0) dz = 0.0;
                    if (dz > 1.0) dz = 1.0;
                }

                double w;
                double dx_adj, dz_adj;
                if (disableFringe) {
                    w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);
                    A = self->p[X] + 0;
                    AA = self->p[A] + Z;
                    B = self->p[X + 1] + 0;
                    BA = self->p[B] + Z;
                    double vv0_d = self->lerp(u,
                        self->grad2(self->p[AA], dx, dz),
                        self->grad(self->p[BA], dx - 1.0, 0.0, dz));
                    double vv2_d = self->lerp(u,
                        self->grad(self->p[AA + 1], dx, 0.0, dz - 1.0),
                        self->grad(self->p[BA + 1], dx - 1.0, 0.0, dz - 1.0));
                    double val = self->lerp(w, vv0_d, vv2_d);
                    buffer[pp++] += (float)(val * scale);
                } else {
                    float dxf = (float)dx;
                    float dzf = (float)dz;
                    w = (double)(dzf * dzf * dzf * (dzf * (dzf * 6 - 15) + 10));
                    A = self->p[X] + 0;
                    AA = self->p[A] + Z;
                    B = self->p[X + 1] + 0;
                    BA = self->p[B] + Z;
                    float vv0_f = self->lerp((float)u,
                        self->grad2(self->p[AA], dxf, dzf),
                        self->grad(self->p[BA], dxf - 1, 0, dzf));
                    float vv2_f = self->lerp((float)u,
                        self->grad(self->p[AA + 1], dxf, 0, dzf - 1),
                        self->grad(self->p[BA + 1], dxf - 1, 0, dzf - 1));
                    float valf = self->lerp((float)w, vv0_f, vv2_f);
                    buffer[pp++] += valf * scale;
                }
            }
        }
        return;
    }

    // 3D 分支
    int pp = 0;
    float scale = 1.0f / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    double vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        double x = (_x + xx) * (double)xs + self->xo;
        int xf = (int)x;
        if (x < (double)xf) xf--;
        int X = xf & 255;
        double dx = x - (double)xf;
        if (doClamp) {
            if (dx < 0.0) dx = 0.0;
            if (dx > 1.0) dx = 1.0;
        }

        double u;
        if (disableFringe) {
            u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);
        } else {
            float dxf = (float)dx;
            u = (double)(dxf * dxf * dxf * (dxf * (dxf * 6 - 15) + 10));
        }

        for (int zz = 0; zz < zSize; zz++) {
            double z = (_z + zz) * (double)zs + self->zo;
            int zf = (int)z;
            if (z < (double)zf) zf--;
            int Z = zf & 255;
            double dz = z - (double)zf;
            if (doClamp) {
                if (dz < 0.0) dz = 0.0;
                if (dz > 1.0) dz = 1.0;
            }

            double w;
            if (disableFringe) {
                w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);
            } else {
                float dzf = (float)dz;
                w = (double)(dzf * dzf * dzf * (dzf * (dzf * 6 - 15) + 10));
            }

            for (int yy = 0; yy < ySize; yy++) {
                double y = (_y + yy) * (double)ys + self->yo;
                int yf = (int)y;
                if (y < (double)yf) yf--;
                int Y = yf & 255;
                double dy = y - (double)yf;
                if (doClamp) {
                    if (dy < 0.0) dy = 0.0;
                    if (dy > 1.0) dy = 1.0;
                }

                double v;
                double dx_adj, dy_adj, dz_adj;
                if (disableFringe) {
                    v = dy * dy * dy * (dy * (dy * 6 - 15) + 10);
                    if (yy == 0 || Y != yOld) {
                        yOld = Y;
                        A = self->p[X] + Y;
                        AA = self->p[A] + Z;
                        AB = self->p[A + 1] + Z;
                        B = self->p[X + 1] + Y;
                        BA = self->p[B] + Z;
                        BB = self->p[B + 1] + Z;
                        vv0 = self->lerp(u,
                            self->grad(self->p[AA], dx, dy, dz),
                            self->grad(self->p[BA], dx - 1.0, dy, dz));
                        vv1 = self->lerp(u,
                            self->grad(self->p[AB], dx, dy - 1.0, dz),
                            self->grad(self->p[BB], dx - 1.0, dy - 1.0, dz));
                        vv2 = self->lerp(u,
                            self->grad(self->p[AA + 1], dx, dy, dz - 1.0),
                            self->grad(self->p[BA + 1], dx - 1.0, dy, dz - 1.0));
                        vv3 = self->lerp(u,
                            self->grad(self->p[AB + 1], dx, dy - 1.0, dz - 1.0),
                            self->grad(self->p[BB + 1], dx - 1.0, dy - 1.0, dz - 1.0));
                    }
                    double v0 = self->lerp(v, vv0, vv1);
                    double v1 = self->lerp(v, vv2, vv3);
                    double val = self->lerp(w, v0, v1);
                    buffer[pp++] += (float)(val * scale);
                } else {
                    float dxf = (float)dx;
                    float dyf = (float)dy;
                    float dzf = (float)dz;
                    v = (double)(dyf * dyf * dyf * (dyf * (dyf * 6 - 15) + 10));
                    if (yy == 0 || Y != yOld) {
                        yOld = Y;
                        A = self->p[X] + Y;
                        AA = self->p[A] + Z;
                        AB = self->p[A + 1] + Z;
                        B = self->p[X + 1] + Y;
                        BA = self->p[B] + Z;
                        BB = self->p[B + 1] + Z;
                        float uf = (float)u;
                        vv0 = self->lerp(uf,
                            self->grad(self->p[AA], dxf, dyf, dzf),
                            self->grad(self->p[BA], dxf - 1, dyf, dzf));
                        vv1 = self->lerp(uf,
                            self->grad(self->p[AB], dxf, dyf - 1, dzf),
                            self->grad(self->p[BB], dxf - 1, dyf - 1, dzf));
                        vv2 = self->lerp(uf,
                            self->grad(self->p[AA + 1], dxf, dyf, dzf - 1),
                            self->grad(self->p[BA + 1], dxf - 1, dyf, dzf - 1));
                        vv3 = self->lerp(uf,
                            self->grad(self->p[AB + 1], dxf, dyf - 1, dzf - 1),
                            self->grad(self->p[BB + 1], dxf - 1, dyf - 1, dzf - 1));
                    }
                    float v0f = self->lerp((float)v, (float)vv0, (float)vv1);
                    float v1f = self->lerp((float)v, (float)vv2, (float)vv3);
                    float valf = self->lerp((float)w, v0f, v1f);
                    buffer[pp++] += valf * scale;
                }
            }
        }
    }
}

// ===== add_int64（仅展示差异，逻辑与 add_int 类似） =====
static void add_int64(ImprovedNoise* self, float* buffer,
                      double _x, double _y, double _z,
                      int xSize, int ySize, int zSize,
                      float xs, float ys, float zs, float pow) {
    bool doClamp = false;
    bool disableFringe = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
        disableFringe = Minecraft::instance->options.getBooleanValue(OPTIONS_DISABLED_FRINGE_LANDS);
    }

    // ====== 2D 分支 ======
    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        float vv0 = 0, vv2 = 0;
        int pp = 0;
        float scale = 1.0f / pow;
        for (int xx = 0; xx < xSize; xx++) {
            double x = (_x + xx) * (double)xs + self->xo;
            int64_t xf = (int64_t)x;                     // 64位溢出保留！
            if (x < (double)xf) xf--;
            int X = (int)(xf & 255);
            double dx = x - (double)xf;
            if (doClamp) {
                if (dx < 0.0) dx = 0.0;
                if (dx > 1.0) dx = 1.0;
            }

            double u;
            if (disableFringe) {
                u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);
            } else {
                float dxf = (float)dx;
                u = (double)(dxf * dxf * dxf * (dxf * (dxf * 6 - 15) + 10));
            }

            for (int zz = 0; zz < zSize; zz++) {
                double z = (_z + zz) * (double)zs + self->zo;
                int64_t zf = (int64_t)z;
                if (z < (double)zf) zf--;
                int Z = (int)(zf & 255);
                double dz = z - (double)zf;
                if (doClamp) {
                    if (dz < 0.0) dz = 0.0;
                    if (dz > 1.0) dz = 1.0;
                }

                double w;
                if (disableFringe) {
                    w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);
                    A = self->p[X] + 0;
                    AA = self->p[A] + Z;
                    B = self->p[X + 1] + 0;
                    BA = self->p[B] + Z;
                    double vv0_d = self->lerp(u,
                        self->grad2(self->p[AA], dx, dz),
                        self->grad(self->p[BA], dx - 1.0, 0.0, dz));
                    double vv2_d = self->lerp(u,
                        self->grad(self->p[AA + 1], dx, 0.0, dz - 1.0),
                        self->grad(self->p[BA + 1], dx - 1.0, 0.0, dz - 1.0));
                    double val = self->lerp(w, vv0_d, vv2_d);
                    buffer[pp++] += (float)(val * scale);
                } else {
                    float dxf = (float)dx;
                    float dzf = (float)dz;
                    w = (double)(dzf * dzf * dzf * (dzf * (dzf * 6 - 15) + 10));
                    A = self->p[X] + 0;
                    AA = self->p[A] + Z;
                    B = self->p[X + 1] + 0;
                    BA = self->p[B] + Z;
                    float vv0_f = self->lerp((float)u,
                        self->grad2(self->p[AA], dxf, dzf),
                        self->grad(self->p[BA], dxf - 1, 0, dzf));
                    float vv2_f = self->lerp((float)u,
                        self->grad(self->p[AA + 1], dxf, 0, dzf - 1),
                        self->grad(self->p[BA + 1], dxf - 1, 0, dzf - 1));
                    float valf = self->lerp((float)w, vv0_f, vv2_f);
                    buffer[pp++] += valf * scale;
                }
            }
        }
        return;
    }

    // ====== 3D 分支 ======
    int pp = 0;
    float scale = 1.0f / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    double vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        double x = (_x + xx) * (double)xs + self->xo;
        int64_t xf = (int64_t)x;
        if (x < (double)xf) xf--;
        int X = (int)(xf & 255);
        double dx = x - (double)xf;
        if (doClamp) {
            if (dx < 0.0) dx = 0.0;
            if (dx > 1.0) dx = 1.0;
        }

        double u;
        if (disableFringe) {
            u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);
        } else {
            float dxf = (float)dx;
            u = (double)(dxf * dxf * dxf * (dxf * (dxf * 6 - 15) + 10));
        }

        for (int zz = 0; zz < zSize; zz++) {
            double z = (_z + zz) * (double)zs + self->zo;
            int64_t zf = (int64_t)z;
            if (z < (double)zf) zf--;
            int Z = (int)(zf & 255);
            double dz = z - (double)zf;
            if (doClamp) {
                if (dz < 0.0) dz = 0.0;
                if (dz > 1.0) dz = 1.0;
            }

            double w;
            if (disableFringe) {
                w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);
            } else {
                float dzf = (float)dz;
                w = (double)(dzf * dzf * dzf * (dzf * (dzf * 6 - 15) + 10));
            }

            for (int yy = 0; yy < ySize; yy++) {
                double y = (_y + yy) * (double)ys + self->yo;
                int64_t yf = (int64_t)y;
                if (y < (double)yf) yf--;
                int Y = (int)(yf & 255);
                double dy = y - (double)yf;
                if (doClamp) {
                    if (dy < 0.0) dy = 0.0;
                    if (dy > 1.0) dy = 1.0;
                }

                double v;
                if (disableFringe) {
                    v = dy * dy * dy * (dy * (dy * 6 - 15) + 10);
                    if (yy == 0 || Y != yOld) {
                        yOld = Y;
                        A = self->p[X] + Y;
                        AA = self->p[A] + Z;
                        AB = self->p[A + 1] + Z;
                        B = self->p[X + 1] + Y;
                        BA = self->p[B] + Z;
                        BB = self->p[B + 1] + Z;
                        vv0 = self->lerp(u,
                            self->grad(self->p[AA], dx, dy, dz),
                            self->grad(self->p[BA], dx - 1.0, dy, dz));
                        vv1 = self->lerp(u,
                            self->grad(self->p[AB], dx, dy - 1.0, dz),
                            self->grad(self->p[BB], dx - 1.0, dy - 1.0, dz));
                        vv2 = self->lerp(u,
                            self->grad(self->p[AA + 1], dx, dy, dz - 1.0),
                            self->grad(self->p[BA + 1], dx - 1.0, dy, dz - 1.0));
                        vv3 = self->lerp(u,
                            self->grad(self->p[AB + 1], dx, dy - 1.0, dz - 1.0),
                            self->grad(self->p[BB + 1], dx - 1.0, dy - 1.0, dz - 1.0));
                    }
                    double v0 = self->lerp(v, vv0, vv1);
                    double v1 = self->lerp(v, vv2, vv3);
                    double val = self->lerp(w, v0, v1);
                    buffer[pp++] += (float)(val * scale);
                } else {
                    float dxf = (float)dx;
                    float dyf = (float)dy;
                    float dzf = (float)dz;
                    v = (double)(dyf * dyf * dyf * (dyf * (dyf * 6 - 15) + 10));
                    if (yy == 0 || Y != yOld) {
                        yOld = Y;
                        A = self->p[X] + Y;
                        AA = self->p[A] + Z;
                        AB = self->p[A + 1] + Z;
                        B = self->p[X + 1] + Y;
                        BA = self->p[B] + Z;
                        BB = self->p[B + 1] + Z;
                        float uf = (float)u;
                        vv0 = self->lerp(uf,
                            self->grad(self->p[AA], dxf, dyf, dzf),
                            self->grad(self->p[BA], dxf - 1, dyf, dzf));
                        vv1 = self->lerp(uf,
                            self->grad(self->p[AB], dxf, dyf - 1, dzf),
                            self->grad(self->p[BB], dxf - 1, dyf - 1, dzf));
                        vv2 = self->lerp(uf,
                            self->grad(self->p[AA + 1], dxf, dyf, dzf - 1),
                            self->grad(self->p[BA + 1], dxf - 1, dyf, dzf - 1));
                        vv3 = self->lerp(uf,
                            self->grad(self->p[AB + 1], dxf, dyf - 1, dzf - 1),
                            self->grad(self->p[BB + 1], dxf - 1, dyf - 1, dzf - 1));
                    }
                    float v0f = self->lerp((float)v, (float)vv0, (float)vv1);
                    float v1f = self->lerp((float)v, (float)vv2, (float)vv3);
                    float valf = self->lerp((float)w, v0f, v1f);
                    buffer[pp++] += valf * scale;
                }
            }
        }
    }
}
// ===== add_double（原本就是 double，无需修改） =====
static void add_double(ImprovedNoise* self, float* buffer,
                       double _x, double _y, double _z,
                       int xSize, int ySize, int zSize,
                       float xs, float ys, float zs, float pow) {
    // 原版 add_double 实现，内部已使用 double 计算，无需改动
    // 但需要确保变量声明匹配 double
    bool doClamp = false;
    if (Minecraft::instance) {
        doClamp = Minecraft::instance->options.getBooleanValue(OPTIONS_POSTPONED_FRINGE);
    }

    if (ySize == 1) {
        int A = 0, AA = 0, B = 0, BA = 0;
        double vv0 = 0, vv2 = 0;
        int pp = 0;
        double scale = 1.0 / pow;
        for (int xx = 0; xx < xSize; xx++) {
            double x = (_x + xx) * xs + self->xo;
            double xf = floor(x);
            if (x < xf) xf -= 1.0;
            int X = ((int64_t)xf) & 255;
            double dx = x - xf;
            if (doClamp) {
                if (dx < 0.0) dx = 0.0;
                if (dx > 1.0) dx = 1.0;
            }
            double u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);

            for (int zz = 0; zz < zSize; zz++) {
                double z = (_z + zz) * zs + self->zo;
                double zf = floor(z);
                if (z < zf) zf -= 1.0;
                int Z = ((int64_t)zf) & 255;
                double dz = z - zf;
                if (doClamp) {
                    if (dz < 0.0) dz = 0.0;
                    if (dz > 1.0) dz = 1.0;
                }
                double w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);

                A = self->p[X] + 0;
                AA = self->p[A] + Z;
                B = self->p[X + 1] + 0;
                BA = self->p[B] + Z;
                vv0 = self->lerp(u,
                    self->grad2(self->p[AA], dx, dz),
                    self->grad(self->p[BA], dx - 1.0, 0.0, dz));
                vv2 = self->lerp(u,
                    self->grad(self->p[AA + 1], dx, 0.0, dz - 1.0),
                    self->grad(self->p[BA + 1], dx - 1.0, 0.0, dz - 1.0));

                double val = self->lerp(w, vv0, vv2);
                buffer[pp++] += (float)(val * scale);
            }
        }
        return;
    }

    // 3D 分支
    int pp = 0;
    double scale = 1.0 / pow;
    int yOld = -1;
    int A = 0, AA = 0, AB = 0, B = 0, BA = 0, BB = 0;
    double vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        double x = (_x + xx) * xs + self->xo;
        double xf = floor(x);
        if (x < xf) xf -= 1.0;
        int X = ((int64_t)xf) & 255;
        double dx = x - xf;
        if (doClamp) {
            if (dx < 0.0) dx = 0.0;
            if (dx > 1.0) dx = 1.0;
        }
        double u = dx * dx * dx * (dx * (dx * 6 - 15) + 10);

        for (int zz = 0; zz < zSize; zz++) {
            double z = (_z + zz) * zs + self->zo;
            double zf = floor(z);
            if (z < zf) zf -= 1.0;
            int Z = ((int64_t)zf) & 255;
            double dz = z - zf;
            if (doClamp) {
                if (dz < 0.0) dz = 0.0;
                if (dz > 1.0) dz = 1.0;
            }
            double w = dz * dz * dz * (dz * (dz * 6 - 15) + 10);

            for (int yy = 0; yy < ySize; yy++) {
                double y = (_y + yy) * ys + self->yo;
                double yf = floor(y);
                if (y < yf) yf -= 1.0;
                int Y = ((int64_t)yf) & 255;
                double dy = y - yf;
                if (doClamp) {
                    if (dy < 0.0) dy = 0.0;
                    if (dy > 1.0) dy = 1.0;
                }
                double v = dy * dy * dy * (dy * (dy * 6 - 15) + 10);

                if (yy == 0 || Y != yOld) {
                    yOld = Y;
                    A = self->p[X] + Y;
                    AA = self->p[A] + Z;
                    AB = self->p[A + 1] + Z;
                    B = self->p[X + 1] + Y;
                    BA = self->p[B] + Z;
                    BB = self->p[B + 1] + Z;
                    vv0 = self->lerp(u,
                        self->grad(self->p[AA], dx, dy, dz),
                        self->grad(self->p[BA], dx - 1.0, dy, dz));
                    vv1 = self->lerp(u,
                        self->grad(self->p[AB], dx, dy - 1.0, dz),
                        self->grad(self->p[BB], dx - 1.0, dy - 1.0, dz));
                    vv2 = self->lerp(u,
                        self->grad(self->p[AA + 1], dx, dy, dz - 1.0),
                        self->grad(self->p[BA + 1], dx - 1.0, dy, dz - 1.0));
                    vv3 = self->lerp(u,
                        self->grad(self->p[AB + 1], dx, dy - 1.0, dz - 1.0),
                        self->grad(self->p[BB + 1], dx - 1.0, dy - 1.0, dz - 1.0));
                }

                double v0 = self->lerp(v, vv0, vv1);
                double v1 = self->lerp(v, vv2, vv3);
                double val = self->lerp(w, v0, v1);
                buffer[pp++] += (float)(val * scale);
            }
        }
    }
}
int ImprovedNoise::hashCode() {
    int x = 4711;
    for (int i = 0; i < 512; ++i)
        x = x * 37 + p[i];
    return x;
}
