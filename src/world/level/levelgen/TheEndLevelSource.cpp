#include "TheEndLevelSource.h"
#include "../Level.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../../../client/Minecraft.h"

#include <cstdint>

// ============ 构造器 ============

TheEndLevelSource::TheEndLevelSource(Level* level, long seed)
    : random(seed), level(level), densityBuffer(nullptr),
      m_worldOffsetX(0.0), m_worldOffsetY(0.0), m_worldOffsetZ(0.0),
      m_worldScaleX(1.0), m_worldScaleY(1.0), m_worldScaleZ(1.0)
{
    densityBuffer = new double[DENSITY_X * DENSITY_Y * DENSITY_Z];

    // 🛡️ 读取 Options（偏移/缩放 + 环开关 + 噪声类型）
    if (Minecraft::instance) {
        std::string sx = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_X);
        if (!sx.empty()) m_worldScaleX = atof(sx.c_str());
        std::string sy = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_Y);
        if (!sy.empty()) m_worldScaleY = atof(sy.c_str());
        std::string sz = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_Z);
        if (!sz.empty()) m_worldScaleZ = atof(sz.c_str());

        std::string ox = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_X);
        if (!ox.empty()) m_worldOffsetX = atof(ox.c_str());
        std::string oy = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_Y);
        if (!oy.empty()) m_worldOffsetY = atof(oy.c_str());
        std::string oz = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_Z);
        if (!oz.empty()) m_worldOffsetZ = atof(oz.c_str());

        m_endCircles = Minecraft::instance->options.getBooleanValue(OPTIONS_END_CIRCLES);
        m_useDoubleNoise = Minecraft::instance->options.getBooleanValue(OPTIONS_EXTENDED_PERLIN_NOISE);
    }

    // 🛡️ 创建噪声 — 根据选项选择 double 或 float
    if (m_useDoubleNoise) {
        pNoise1 = new PerlinNoiseT<double>(&random, 16);
        pNoise2 = new PerlinNoiseT<double>(&random, 16);
        pNoise3 = new PerlinNoiseT<double>(&random, 8);
        sNoise1 = new PerlinNoiseT<double>(&random, 4);
    } else {
        pNoise1_f = new PerlinNoiseT<float>(&random, 16);
        pNoise2_f = new PerlinNoiseT<float>(&random, 16);
        pNoise3_f = new PerlinNoiseT<float>(&random, 8);
        sNoise1_f = new PerlinNoiseT<float>(&random, 4);
    }
}

// ============ 析构器 ============

TheEndLevelSource::~TheEndLevelSource() {
    delete pNoise1;   delete pNoise1_f;
    delete pNoise2;   delete pNoise2_f;
    delete pNoise3;   delete pNoise3_f;
    delete sNoise1;   delete sNoise1_f;
    delete[] densityBuffer;
}

// ============ 噪声访问器 ============

double TheEndLevelSource::getPNoise1Value(double x, double y, double z) const {
    if (m_useDoubleNoise)
        return pNoise1->getValue(x, y, z);
    else
        return (double)pNoise1_f->getValue((float)x, (float)y, (float)z);
}

double TheEndLevelSource::getPNoise2Value(double x, double y, double z) const {
    if (m_useDoubleNoise)
        return pNoise2->getValue(x, y, z);
    else
        return (double)pNoise2_f->getValue((float)x, (float)y, (float)z);
}

double TheEndLevelSource::getPNoise3Value(double x, double y, double z) const {
    if (m_useDoubleNoise)
        return pNoise3->getValue(x, y, z);
    else
        return (double)pNoise3_f->getValue((float)x, (float)y, (float)z);
}

double TheEndLevelSource::getSNoise1Value(double x, double z) const {
    if (m_useDoubleNoise)
        return sNoise1->getValue(x, z);
    else
        return (double)sNoise1_f->getValue((float)x, (float)z);
}

// 文件：src/world/level/levelgen/TheEndLevelSource.cpp

void TheEndLevelSource::generateDensityCells(int64_t chunkX, int64_t chunkZ, double* density) {
    const double B_XZ  = 1368.824;
    const double B_Y   = 684.412;
    const double BS_XZ = 17.1103;
    const double BS_Y  = 4.277575;

    double originX = (double)(chunkX * 2) * m_worldScaleX + m_worldOffsetX * m_worldScaleX / 8.0;
    double originY = m_worldOffsetY * m_worldScaleY / 8.0;
    double originZ = (double)(chunkZ * 2) * m_worldScaleZ + m_worldOffsetZ * m_worldScaleZ / 8.0;

    double sx  = B_XZ  * m_worldScaleX;
    double sy  = B_Y   * m_worldScaleY;
    double sz  = B_XZ  * m_worldScaleZ;
    double ssx = BS_XZ * m_worldScaleX;
    double ssy = BS_Y  * m_worldScaleY;
    double ssz = BS_XZ * m_worldScaleZ;

    // 🛡️ 大坐标折回 + 防溢出
    const double P = 256.0;
    originX = fmod(originX, P); if (originX < 0.0) originX += P;
    originY = fmod(originY, P); if (originY < 0.0) originY += P;
    originZ = fmod(originZ, P); if (originZ < 0.0) originZ += P;

    const double MAX_SAFE = 5.0e8;
    if (sx  > MAX_SAFE || sx  < -MAX_SAFE) sx  = (sx  > 0) ? MAX_SAFE : -MAX_SAFE;
    if (sy  > MAX_SAFE || sy  < -MAX_SAFE) sy  = (sy  > 0) ? MAX_SAFE : -MAX_SAFE;
    if (sz  > MAX_SAFE || sz  < -MAX_SAFE) sz  = (sz  > 0) ? MAX_SAFE : -MAX_SAFE;
    if (ssx > MAX_SAFE || ssx < -MAX_SAFE) ssx = (ssx > 0) ? MAX_SAFE : -MAX_SAFE;
    if (ssy > MAX_SAFE || ssy < -MAX_SAFE) ssy = (ssy > 0) ? MAX_SAFE : -MAX_SAFE;
    if (ssz > MAX_SAFE || ssz < -MAX_SAFE) ssz = (ssz > 0) ? MAX_SAFE : -MAX_SAFE;

    if (m_useDoubleNoise) {
        // ── Double 路径（当前默认） ──
        double* n1 = pNoise1->getRegion(nullptr, originX, originY, originZ,
            DENSITY_X, DENSITY_Y, DENSITY_Z, sx, sy, sz);
        double* n2 = pNoise2->getRegion(nullptr, originX, originY, originZ,
            DENSITY_X, DENSITY_Y, DENSITY_Z, sx, sy, sz);
        double* n3 = pNoise3->getRegion(nullptr, originX, originY, originZ,
            DENSITY_X, DENSITY_Y, DENSITY_Z, ssx, ssy, ssz);

        for (int xC = 0; xC < DENSITY_X; xC++) {
            for (int zC = 0; zC < DENSITY_Z; zC++) {
                double hV = getIslandHeightValue(chunkX, chunkZ, xC, zC);
                double v23 = 7.0;
                for (int yC = 0; yC < DENSITY_Y; yC += 3, v23 -= 3.0) {
                    int idx = yC + DENSITY_Y * (zC + DENSITY_Z * xC);
                    for (int d = 0; d < 3; d++) {
                        int ind = idx + d;
                        double s_d = Mth::clamp(n3[ind] / 20.0 + 0.5, 0.0, 1.0);
                        double v4 = n1[ind] / 512.0 + (n2[ind] / 512.0 - n1[ind] / 512.0) * s_d + hV - 8.0;
                        double v6;
                        if (yC < 14) {
                            v6 = v4;
                            if (yC < 9) {
                                double t = (v23 + 1.0 - (double)d) / 7.0;
                                if (t < 0.0) t = 0.0; if (t > 1.0) t = 1.0;
                                v6 = (1.0 - t) * v4 - t * 30.0;
                            }
                        } else {
                            double t = ((double)(yC - 14) + (double)d) / 64.0;
                            if (t < 0.0) t = 0.0; if (t > 1.0) t = 1.0;
                            v6 = (1.0 - t) * v4 - t * 3000.0;
                        }
                        density[ind] = v6;
                    }
                }
            }
        }
        delete[] n1; delete[] n2; delete[] n3;
    } else {
        // ── Float 路径 ──
        float* n1_f = pNoise1_f->getRegion(nullptr,
            (float)originX, (float)originY, (float)originZ,
            DENSITY_X, DENSITY_Y, DENSITY_Z,
            (float)sx, (float)sy, (float)sz);
        float* n2_f = pNoise2_f->getRegion(nullptr,
            (float)originX, (float)originY, (float)originZ,
            DENSITY_X, DENSITY_Y, DENSITY_Z,
            (float)sx, (float)sy, (float)sz);
        float* n3_f = pNoise3_f->getRegion(nullptr,
            (float)originX, (float)originY, (float)originZ,
            DENSITY_X, DENSITY_Y, DENSITY_Z,
            (float)ssx, (float)ssy, (float)ssz);

        for (int xC = 0; xC < DENSITY_X; xC++) {
            for (int zC = 0; zC < DENSITY_Z; zC++) {
                double hV = getIslandHeightValue(chunkX, chunkZ, xC, zC);
                float v23 = 7.0f;
                for (int yC = 0; yC < DENSITY_Y; yC += 3, v23 -= 3.0f) {
                    int idx = yC + DENSITY_Y * (zC + DENSITY_Z * xC);
                    for (int d = 0; d < 3; d++) {
                        int ind = idx + d;
                        double s_d = Mth::clamp((double)n3_f[ind] / 20.0 + 0.5, 0.0, 1.0);
                        double v4 = (double)n1_f[ind] / 512.0
                            + ((double)n2_f[ind] / 512.0 - (double)n1_f[ind] / 512.0) * s_d
                            + hV - 8.0;
                        double v6;
                        if (yC < 14) {
                            v6 = v4;
                            if (yC < 9) {
                                double t = (double)(v23 + 1.0f - (float)d) / 7.0;
                                if (t < 0.0) t = 0.0; if (t > 1.0) t = 1.0;
                                v6 = (1.0 - t) * v4 - t * 30.0;
                            }
                        } else {
                            double t = (double)(yC - 14 + d) / 64.0;
                            if (t < 0.0) t = 0.0; if (t > 1.0) t = 1.0;
                            v6 = (1.0 - t) * v4 - t * 3000.0;
                        }
                        density[ind] = v6;
                    }
                }
            }
        }
        delete[] n1_f; delete[] n2_f; delete[] n3_f;
    }
}

// ========== sampleDensityAt ==========

double TheEndLevelSource::sampleDensityAt(double worldX, double worldY, double worldZ) {
    double noiseX = worldX * m_worldScaleX + m_worldOffsetX * m_worldScaleX;
    double noiseY = worldY * m_worldScaleY + m_worldOffsetY * m_worldScaleY;
    double noiseZ = worldZ * m_worldScaleZ + m_worldOffsetZ * m_worldScaleZ;

    double s, n1v, n2v;

    if (m_useDoubleNoise) {
        s = pNoise3->getValue(noiseX * 17.1103, noiseY * 4.277575, noiseZ * 17.1103);
        n1v = pNoise1->getValue(noiseX * 1368.824, noiseY * 684.412, noiseZ * 1368.824);
        n2v = pNoise2->getValue(noiseX * 1368.824, noiseY * 684.412, noiseZ * 1368.824);
    } else {
        s = (double)pNoise3_f->getValue(
            (float)(noiseX * 17.1103), (float)(noiseY * 4.277575), (float)(noiseZ * 17.1103));
        n1v = (double)pNoise1_f->getValue(
            (float)(noiseX * 1368.824), (float)(noiseY * 684.412), (float)(noiseZ * 1368.824));
        n2v = (double)pNoise2_f->getValue(
            (float)(noiseX * 1368.824), (float)(noiseY * 684.412), (float)(noiseZ * 1368.824));
    }

    s = Mth::clamp(s / 20.0 + 0.5, 0.0, 1.0);
    double density = n1v / 512.0 + (n2v / 512.0 - n1v / 512.0) * s;

    int64_t cx = Mth::floor64(worldX / 16.0);
    int64_t cz = Mth::floor64(worldZ / 16.0);
    double hV = getIslandHeightValue(cx, cz, 1, 1);
    density += hV - 8.0;

    return density;
}

// 文件：src/world/level/levelgen/TheEndLevelSource.cpp

// ========== getIslandHeightValue ==========

double TheEndLevelSource::getIslandHeightValue(int64_t chunkX, int64_t chunkZ, int xC, int zC) {
    double chX = (double)chunkX + m_worldOffsetX * m_worldScaleX / 16.0;
    double chZ = (double)chunkZ + m_worldOffsetZ * m_worldScaleZ / 16.0;

    // === 中央岛 ===
    double v9;
    if (m_endCircles) {
        int sx = (int)(zC + 2.0 * chZ);
        int sz = (int)(xC + 2.0 * chX);
        int distSq = sx * sx + sz * sz;
        v9 = 100.0 - Mth::sqrt((double)distSq) * 8.0;
    } else {
        double sx = (double)(zC + 2.0 * chZ);
        double sz = (double)(xC + 2.0 * chX);
        v9 = 100.0 - Mth::sqrt(sx * sx + sz * sz) * 8.0;
    }
    v9 = Mth::clamp(v9, -100.0, 80.0);

    // === 外岛 ===
    int wX_start = (int)floor(chX - 12.0);
    int wZ_start = (int)floor(chZ - 12.0);
    int v28_start = xC + 24;
    int v17_start = zC + 24;

    for (int i = 0; i < 25; i++) {
        int wX = wX_start + i;
        int v28 = v28_start - 2 * i;

        for (int j = 0; j < 25; j++) {
            int wZ = wZ_start + j;
            int v17 = v17_start - 2 * j;

            bool outsideCentral;
            if (m_endCircles) {
                int sx = wX * 2;
                int sz = wZ * 2;
                int dSq = sx * sx + sz * sz;
                outsideCentral = dSq > 16384;
            } else {
                outsideCentral = (int64_t)wX * wX + (int64_t)wZ * wZ > 4096;
            }

            if (outsideCentral) {
                // 🛡️ sNoise1 双路径
                double sVal;
                if (m_useDoubleNoise)
                    sVal = sNoise1->getValue((double)wX, (double)wZ);
                else
                    sVal = (double)sNoise1_f->getValue((float)wX, (float)wZ);

                if (sVal < -0.89999998) {
                    int mul = (237 * abs(wZ) + 3439 * abs(wX)) % 13 + 9;
                    double v20 = 100.0 - hypot((double)v17, (double)v28) * (double)mul;
                    v20 = Mth::clamp(v20, -100.0, 80.0);
                    v9 = Mth::Max(v20, v9);
                }
            }
        }
    }

    return v9;
}

// ========== prepareHeights ==========

void TheEndLevelSource::prepareHeights(int64_t chunkX, int64_t chunkZ, unsigned char* blocks) {
    generateDensityCells(chunkX, chunkZ, densityBuffer);
    memset(blocks, 0, LevelChunk::ChunkBlockCount);

    for (int xCH = 0; xCH < 2; xCH++) {
        for (int zCH = 0; zCH < 2; zCH++) {
            for (int yCH = 0; yCH < 32; yCH++) {
                int baseIdx = yCH + DENSITY_Y * (zCH + DENSITY_Z * xCH);

                double c000 = densityBuffer[baseIdx];
                double c001 = densityBuffer[baseIdx + DENSITY_Y];
                double c010 = densityBuffer[baseIdx + DENSITY_Y * DENSITY_Z];
                double c011 = densityBuffer[baseIdx + DENSITY_Y + DENSITY_Y * DENSITY_Z];
                double c100 = densityBuffer[baseIdx + 1];
                double c101 = densityBuffer[baseIdx + 1 + DENSITY_Y];
                double c110 = densityBuffer[baseIdx + 1 + DENSITY_Y * DENSITY_Z];
                double c111 = densityBuffer[baseIdx + 1 + DENSITY_Y + DENSITY_Y * DENSITY_Z];

                for (int zCL = 0; zCL < 8; zCL++) {
                    double fz = zCL / 8.0;
                    int zP = zCH * 8 + zCL;

                    double z0_y0 = c000 + (c001 - c000) * fz;
                    double z1_y0 = c010 + (c011 - c010) * fz;
                    double z0_y1 = c100 + (c101 - c100) * fz;
                    double z1_y1 = c110 + (c111 - c110) * fz;

                    for (int xCL = 0; xCL < 8; xCL++) {
                        double fx = xCL / 8.0;
                        int xP = xCH * 8 + xCL;

                        double val_y0 = z0_y0 + (z1_y0 - z0_y0) * fx;
                        double val_y1 = z0_y1 + (z1_y1 - z0_y1) * fx;

                        for (int yCL = 0; yCL < 4; yCL++) {
                            double fy = yCL / 4.0;
                            double val = val_y0 + (val_y1 - val_y0) * fy;

                            if (val > 0.0) {
                                int yP = yCH * 4 + yCL;
                                if (yP >= 0 && yP < 128) {
                                    int offs = (xP << 11) | (zP << 7) | yP;
                                    blocks[offs] = (unsigned char)Tile::endStone->id;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

// ========== ChunkSource 接口 ==========

LevelChunk* TheEndLevelSource::create(int64_t x, int64_t z) {
    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    LevelChunk* levelChunk = new LevelChunk(level, blocks, x, z);

    int64_t hashedPos = (x << 32) | (z & 0xffffffff);
    chunkMap.insert(std::make_pair(hashedPos, levelChunk));

    prepareHeights(x, z, blocks);

    // 🛡️ 首次创建时生成黑曜石柱（一次性）
    generateEndSpikes();

    // 🛡️ 末地全亮度
    memset(levelChunk->skyLight.data, 0xFF, LevelChunk::ChunkBlockCount / 2);
    levelChunk->recalcHeightmapOnly();

    return levelChunk;
}

LevelChunk* TheEndLevelSource::getChunk(int64_t x, int64_t z) {
    int64_t hashedPos = (x << 32) | (z & 0xffffffff);
    auto it = chunkMap.find(hashedPos);
    if (it != chunkMap.end()) return it->second;
    return create(x, z);
}

bool TheEndLevelSource::hasChunk(int64_t x, int64_t z) {
    return true;
}

void TheEndLevelSource::postProcess(ChunkSource* parent, int64_t xt, int64_t zt) {
    // 末地暂不生成结构
}

bool TheEndLevelSource::tick() {
    return false;
}

bool TheEndLevelSource::shouldSave() {
    return true;
}

std::string TheEndLevelSource::gatherStats() {
    return "TheEndLevelSource";
}

Biome::MobList TheEndLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
    return Biome::MobList();
}

// ========== 黑曜石柱 ==========

void TheEndLevelSource::generateEndSpikes() {
    if (m_spikesGenerated) return;
    m_spikesGenerated = true;

    Random rand(level->getSeed());

    int nums[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    for (int i = 1; i < 10; i++) {
        int t = rand.nextInt(i + 1);
        int tmp = nums[i];
        nums[i] = nums[t];
        nums[t] = tmp;
    }

    for (int i = 0; i < 10; i++) {
        double radians = i * 0.62831855 - 6.2831855;
        int posX = (int)floor(cos(radians) * 42.0);
        int posZ = (int)floor(sin(radians) * 42.0);

        int size   = nums[i] / 3 + 2;
        int height = nums[i] + 2 * (nums[i] + 38);

        int offX = (int)(m_worldOffsetX * m_worldScaleX);
        int offZ = (int)(m_worldOffsetZ * m_worldScaleZ);

        for (int dx = -size; dx <= size; dx++) {
            for (int dz = -size; dz <= size; dz++) {
                if (dx * dx + dz * dz > size * size) continue;
                int worldX = posX + dx + offX;
                int worldZ = posZ + dz + offZ;
                for (int y = 0; y < height; y++) {
                    level->setTileAndDataNoUpdate(worldX, y, worldZ,
                        Tile::obsidian->id, 0);
                }
            }
        }
    }
}
