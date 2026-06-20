#include "ImprovedNoise.h"
#include "../../../../util/Random.h"
#include "../../../../client/Minecraft.h"
#include <cstdlib>
#include <cmath>
#include <cstring>

// ============ 构造/初始化 ============

template<typename T>
ImprovedNoiseT<T>::ImprovedNoiseT() {
    Random random(1);
    init(&random);
}

template<typename T>
ImprovedNoiseT<T>::ImprovedNoiseT(Random* random) {
    init(random);
}

template<typename T>
void ImprovedNoiseT<T>::init(Random* random) {
    xo = (T)(random->nextFloat() * 256.0);
    yo = (T)(random->nextFloat() * 256.0);
    zo = (T)(random->nextFloat() * 256.0);
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

// ============ 核心噪声 ============

template<typename T>
T ImprovedNoiseT<T>::noise(T _x, T _y, T _z) const {
    T x = _x + xo;
    T y = _y + yo;
    T z = _z + zo;

    int xf = (int)x;
    int yf = (int)y;
    int zf = (int)z;

    if (x < (T)xf) xf--;
    if (y < (T)yf) yf--;
    if (z < (T)zf) zf--;

    int X = xf & 255,
        Y = yf & 255,
        Z = zf & 255;

    x -= (T)xf;
    y -= (T)yf;
    z -= (T)zf;

    T u = x * x * x * (x * (x * (T)6 - (T)15) + (T)10);
    T v = y * y * y * (y * (y * (T)6 - (T)15) + (T)10);
    T w = z * z * z * (z * (z * (T)6 - (T)15) + (T)10);

    int A = p[X] + Y, AA = p[A] + Z, AB = p[A + 1] + Z,
        B = p[X + 1] + Y, BA = p[B] + Z, BB = p[B + 1] + Z;

    return lerp(w,
        lerp(v, lerp(u, grad(p[AA],     x,     y,     z),
                        grad(p[BA],     x - 1, y,     z)),
                lerp(u, grad(p[AB],     x,     y - 1, z),
                        grad(p[BB],     x - 1, y - 1, z))),
        lerp(v, lerp(u, grad(p[AA + 1], x,     y,     z - 1),
                        grad(p[BA + 1], x - 1, y,     z - 1)),
                lerp(u, grad(p[AB + 1], x,     y - 1, z - 1),
                        grad(p[BB + 1], x - 1, y - 1, z - 1))));
}

// ============ 辅助函数 ============

template<typename T>
T ImprovedNoiseT<T>::lerp(T t, T a, T b) const {
    return a + t * (b - a);
}

template<typename T>
T ImprovedNoiseT<T>::grad2(int hash, T x, T z) const {
    int h = hash & 15;
    T u = (T)(1 - ((h & 8) >> 3)) * x;
    T v = (h < 4) ? (T)0 : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

template<typename T>
T ImprovedNoiseT<T>::grad(int hash, T x, T y, T z) const {
    int h = hash & 15;
    T u = (h < 8) ? x : y;
    T v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);
    return ((h & 1) == 0 ? u : -u) + ((h & 2) == 0 ? v : -v);
}

// ============ getValue ============

template<typename T>
T ImprovedNoiseT<T>::getValue(T x, T y) {
    return static_cast<const ImprovedNoiseT<T>*>(this)->getValue(x, y);
}

template<typename T>
T ImprovedNoiseT<T>::getValue(T x, T y) const {
    return noise(x, y, (T)0);
}

template<typename T>
T ImprovedNoiseT<T>::getValue(T x, T y, T z) const {
    return noise(x, y, z);
}

template<typename T>
int ImprovedNoiseT<T>::hashCode() {
    int x = 4711;
    for (int i = 0; i < 512; ++i)
        x = x * 37 + p[i];
    return x;
}

// ============ add 辅助函数 — int 截断 ============

template<typename T>
static void add_int_t(ImprovedNoiseT<T>* self, T* buffer, T _x, T _y, T _z,
    int xSize, int ySize, int zSize, T xs, T ys, T zs, T pow)
{
    if (ySize == 1) {
        int pp = 0;
        T scale = (T)1 / pow;
        for (int xx = 0; xx < xSize; xx++) {
            T x = (_x + (T)xx) * xs + self->xo;
            int xf = (int)x;
            if (x < (T)xf) xf--;
            int X = xf & 255;
            x -= (T)xf;
            T u = x * x * x * (x * (x * (T)6 - (T)15) + (T)10);

            for (int zz = 0; zz < zSize; zz++) {
                T z = (_z + (T)zz) * zs + self->zo;
                int zf = (int)z;
                if (z < (T)zf) zf--;
                int Z = zf & 255;
                z -= (T)zf;
                T w = z * z * z * (z * (z * (T)6 - (T)15) + (T)10);

                int A  = self->p[X] + 0;
                int AA = self->p[A] + Z;
                int B  = self->p[X + 1] + 0;
                int BA = self->p[B] + Z;
                T vv0 = self->lerp(u, self->grad2(self->p[AA], x, z), self->grad(self->p[BA], x - 1, 0, z));
                T vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, 0, z - 1), self->grad(self->p[BA + 1], x - 1, 0, z - 1));

                T val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    T scale = (T)1 / pow;
    int yOld = -1;
    T vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        T x = (_x + (T)xx) * xs + self->xo;
        int xf = (int)x;
        if (x < (T)xf) xf--;
        int X = xf & 255;
        x -= (T)xf;
        T u = x * x * x * (x * (x * (T)6 - (T)15) + (T)10);

        for (int zz = 0; zz < zSize; zz++) {
            T z = (_z + (T)zz) * zs + self->zo;
            int zf = (int)z;
            if (z < (T)zf) zf--;
            int Z = zf & 255;
            z -= (T)zf;
            T w = z * z * z * (z * (z * (T)6 - (T)15) + (T)10);

            for (int yy = 0; yy < ySize; yy++) {
                T y = (_y + (T)yy) * ys + self->yo;
                int yf = (int)y;
                if (y < (T)yf) yf--;
                int Y = yf & 255;
                y -= (T)yf;
                T v = y * y * y * (y * (y * (T)6 - (T)15) + (T)10);

                if (yy == 0 || Y != yOld) {
                    yOld = Y;
                    int A  = self->p[X] + Y;
                    int AA = self->p[A] + Z;
                    int AB = self->p[A + 1] + Z;
                    int B  = self->p[X + 1] + Y;
                    int BA = self->p[B] + Z;
                    int BB = self->p[B + 1] + Z;
                    vv0 = self->lerp(u, self->grad(self->p[AA], x, y, z), self->grad(self->p[BA], x - 1, y, z));
                    vv1 = self->lerp(u, self->grad(self->p[AB], x, y - 1, z), self->grad(self->p[BB], x - 1, y - 1, z));
                    vv2 = self->lerp(u, self->grad(self->p[AA + 1], x, y, z - 1), self->grad(self->p[BA + 1], x - 1, y, z - 1));
                    vv3 = self->lerp(u, self->grad(self->p[AB + 1], x, y - 1, z - 1), self->grad(self->p[BB + 1], x - 1, y - 1, z - 1));
                }

                T v0 = self->lerp(v, vv0, vv1);
                T v1 = self->lerp(v, vv2, vv3);
                T val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// ============ add 辅助函数 — int64_t 截断 ============

template<typename T>
static void add_int64_t(ImprovedNoiseT<T>* self, T* buffer, T _x, T _y, T _z,
    int xSize, int ySize, int zSize, T xs, T ys, T zs, T pow)
{
    if (ySize == 1) {
        int pp = 0;
        T scale = (T)1 / pow;
        for (int xx = 0; xx < xSize; xx++) {
            double x = (double)((_x + (T)xx) * xs) + (double)self->xo;
            int64_t xf = (int64_t)x;
            if (x < (double)xf) xf--;
            int X = (int)xf & 255;
            double xd = x - (double)xf;
            T u = (T)(xd * xd * xd * (xd * (xd * 6.0 - 15.0) + 10.0));

            for (int zz = 0; zz < zSize; zz++) {
                double z = (double)((_z + (T)zz) * zs) + (double)self->zo;
                int64_t zf = (int64_t)z;
                if (z < (double)zf) zf--;
                int Z = (int)zf & 255;
                double zd = z - (double)zf;
                T w = (T)(zd * zd * zd * (zd * (zd * 6.0 - 15.0) + 10.0));

                int A  = self->p[X] + 0;
                int AA = self->p[A] + Z;
                int B  = self->p[X + 1] + 0;
                int BA = self->p[B] + Z;
                T vv0 = self->lerp(u, self->grad2(self->p[AA], (T)xd, (T)zd), self->grad(self->p[BA], (T)(xd - 1.0), 0, (T)zd));
                T vv2 = self->lerp(u, self->grad(self->p[AA + 1], (T)xd, 0, (T)(zd - 1.0)), self->grad(self->p[BA + 1], (T)(xd - 1.0), 0, (T)(zd - 1.0)));

                T val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    T scale = (T)1 / pow;
    int yOld = -1;
    T vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        double x = (double)((_x + (T)xx) * xs) + (double)self->xo;
        int64_t xf = (int64_t)x;
        if (x < (double)xf) xf--;
        int X = (int)xf & 255;
        double xd = x - (double)xf;
        T u = (T)(xd * xd * xd * (xd * (xd * 6.0 - 15.0) + 10.0));

        for (int zz = 0; zz < zSize; zz++) {
            double z = (double)((_z + (T)zz) * zs) + (double)self->zo;
            int64_t zf = (int64_t)z;
            if (z < (double)zf) zf--;
            int Z = (int)zf & 255;
            double zd = z - (double)zf;
            T w = (T)(zd * zd * zd * (zd * (zd * 6.0 - 15.0) + 10.0));

            for (int yy = 0; yy < ySize; yy++) {
                double y = (double)((_y + (T)yy) * ys) + (double)self->yo;
                int64_t yf = (int64_t)y;
                if (y < (double)yf) yf--;
                int Y = (int)yf & 255;
                double yd = y - (double)yf;
                T v = (T)(yd * yd * yd * (yd * (yd * 6.0 - 15.0) + 10.0));

                if (yy == 0 || Y != yOld) {
                    yOld = Y;
                    int A  = self->p[X] + Y;
                    int AA = self->p[A] + Z;
                    int AB = self->p[A + 1] + Z;
                    int B  = self->p[X + 1] + Y;
                    int BA = self->p[B] + Z;
                    int BB = self->p[B + 1] + Z;
                    vv0 = self->lerp(u, self->grad(self->p[AA], (T)xd, (T)yd, (T)zd), self->grad(self->p[BA], (T)(xd - 1.0), (T)yd, (T)zd));
                    vv1 = self->lerp(u, self->grad(self->p[AB], (T)xd, (T)(yd - 1.0), (T)zd), self->grad(self->p[BB], (T)(xd - 1.0), (T)(yd - 1.0), (T)zd));
                    vv2 = self->lerp(u, self->grad(self->p[AA + 1], (T)xd, (T)yd, (T)(zd - 1.0)), self->grad(self->p[BA + 1], (T)(xd - 1.0), (T)yd, (T)(zd - 1.0)));
                    vv3 = self->lerp(u, self->grad(self->p[AB + 1], (T)xd, (T)(yd - 1.0), (T)(zd - 1.0)), self->grad(self->p[BB + 1], (T)(xd - 1.0), (T)(yd - 1.0), (T)(zd - 1.0)));
                }

                T v0 = self->lerp(v, vv0, vv1);
                T v1 = self->lerp(v, vv2, vv3);
                T val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// ============ add 辅助函数 — floor 截断 ============

template<typename T>
static void add_floor_t(ImprovedNoiseT<T>* self, T* buffer, T _x, T _y, T _z,
    int xSize, int ySize, int zSize, T xs, T ys, T zs, T pow)
{
    if (ySize == 1) {
        int pp = 0;
        T scale = (T)1 / pow;
        for (int xx = 0; xx < xSize; xx++) {
            T x = (_x + (T)xx) * xs + self->xo;
            T xf = (T)std::floor((double)x);
            if (x < xf) xf -= (T)1;
            int X = ((int64_t)xf) & 255;
            T xd = x - xf;
            T u = xd * xd * xd * (xd * (xd * (T)6 - (T)15) + (T)10);

            for (int zz = 0; zz < zSize; zz++) {
                T z = (_z + (T)zz) * zs + self->zo;
                T zf = (T)std::floor((double)z);
                if (z < zf) zf -= (T)1;
                int Z = ((int64_t)zf) & 255;
                T zd = z - zf;
                T w = zd * zd * zd * (zd * (zd * (T)6 - (T)15) + (T)10);

                int A  = self->p[X] + 0;
                int AA = self->p[A] + Z;
                int B  = self->p[X + 1] + 0;
                int BA = self->p[B] + Z;
                T vv0 = self->lerp(u, self->grad2(self->p[AA], xd, zd), self->grad(self->p[BA], xd - 1, 0, zd));
                T vv2 = self->lerp(u, self->grad(self->p[AA + 1], xd, 0, zd - 1), self->grad(self->p[BA + 1], xd - 1, 0, zd - 1));

                T val = self->lerp(w, vv0, vv2);
                buffer[pp++] += val * scale;
            }
        }
        return;
    }

    int pp = 0;
    T scale = (T)1 / pow;
    int yOld = -1;
    T vv0 = 0, vv1 = 0, vv2 = 0, vv3 = 0;

    for (int xx = 0; xx < xSize; xx++) {
        T x = (_x + (T)xx) * xs + self->xo;
        T xf = (T)std::floor((double)x);
        if (x < xf) xf -= (T)1;
        int X = ((int64_t)xf) & 255;
        T xd = x - xf;
        T u = xd * xd * xd * (xd * (xd * (T)6 - (T)15) + (T)10);

        for (int zz = 0; zz < zSize; zz++) {
            T z = (_z + (T)zz) * zs + self->zo;
            T zf = (T)std::floor((double)z);
            if (z < zf) zf -= (T)1;
            int Z = ((int64_t)zf) & 255;
            T zd = z - zf;
            T w = zd * zd * zd * (zd * (zd * (T)6 - (T)15) + (T)10);

            for (int yy = 0; yy < ySize; yy++) {
                T y = (_y + (T)yy) * ys + self->yo;
                T yf = (T)std::floor((double)y);
                if (y < yf) yf -= (T)1;
                int Y = ((int64_t)yf) & 255;
                T yd = y - yf;
                T v = yd * yd * yd * (yd * (yd * (T)6 - (T)15) + (T)10);

                if (yy == 0 || Y != yOld) {
                    yOld = Y;
                    int A  = self->p[X] + Y;
                    int AA = self->p[A] + Z;
                    int AB = self->p[A + 1] + Z;
                    int B  = self->p[X + 1] + Y;
                    int BA = self->p[B] + Z;
                    int BB = self->p[B + 1] + Z;
                    vv0 = self->lerp(u, self->grad(self->p[AA], xd, yd, zd), self->grad(self->p[BA], xd - 1, yd, zd));
                    vv1 = self->lerp(u, self->grad(self->p[AB], xd, yd - 1, zd), self->grad(self->p[BB], xd - 1, yd - 1, zd));
                    vv2 = self->lerp(u, self->grad(self->p[AA + 1], xd, yd, zd - 1), self->grad(self->p[BA + 1], xd - 1, yd, zd - 1));
                    vv3 = self->lerp(u, self->grad(self->p[AB + 1], xd, yd - 1, zd - 1), self->grad(self->p[BB + 1], xd - 1, yd - 1, zd - 1));
                }

                T v0 = self->lerp(v, vv0, vv1);
                T v1 = self->lerp(v, vv2, vv3);
                T val = self->lerp(w, v0, v1);

                buffer[pp++] += val * scale;
            }
        }
    }
}

// ============ add 主函数 ============

template<typename T>
void ImprovedNoiseT<T>::add(T* buffer, T _x, T _y, T _z,
    int xSize, int ySize, int zSize, T xs, T ys, T zs, T pow)
{
    bool use64Bit = false;
    bool useDouble = false;
    if (Minecraft::instance) {
        use64Bit = Minecraft::instance->options.getBooleanValue(OPTIONS_SIXTYFOUR_FARLANDS);
        useDouble = Minecraft::instance->options.getBooleanValue(OPTIONS_DOUBLE_FARLANDS);
    }
    if (useDouble) {
        add_floor_t<T>(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    } else if (use64Bit) {
        add_int64_t<T>(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    } else {
        add_int_t<T>(this, buffer, _x, _y, _z, xSize, ySize, zSize, xs, ys, zs, pow);
    }
}

// ============ 显式实例化 ============

template class ImprovedNoiseT<double>;
template class ImprovedNoiseT<float>;