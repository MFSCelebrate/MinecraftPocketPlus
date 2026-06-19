#include "TheEndLevelSource.h"
#include "../Level.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../../../client/Minecraft.h"

#include <cstdint>

TheEndLevelSource::TheEndLevelSource(Level* level, long seed)
    : random(seed), level(level),
      pNoise1(&random, 16), pNoise2(&random, 16), pNoise3(&random, 8),
      sNoise1(&random, 4), densityBuffer(nullptr),
      m_worldOffsetX(0.0),
      m_worldOffsetY(0.0),
      m_worldOffsetZ(0.0),
      m_worldScaleX(1.0),
      m_worldScaleY(1.0),
      m_worldScaleZ(1.0)
{
    densityBuffer = new double[DENSITY_X * DENSITY_Y * DENSITY_Z];

    // 🛡️ 读取世界缩放/偏移（和 RandomLevelSource 一致）
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
    }
}

TheEndLevelSource::~TheEndLevelSource() {
    delete[] densityBuffer;
}

// ========== Island Height ==========

// 文件：src/world/level/levelgen/TheEndLevelSource.cpp

double TheEndLevelSource::getIslandHeightValue(int64_t chunkX, int64_t chunkZ, int xC, int zC) {
    // 🛡️ 偏移后的 chunk 坐标（影响岛屿在 XZ 平面的位置）
    double chX = (double)chunkX + m_worldOffsetX * m_worldScaleX / 16.0;
    double chZ = (double)chunkZ + m_worldOffsetZ * m_worldScaleZ / 16.0;

    // 中央岛
    double cx = (double)(zC + 2 * chZ);
    double cz = (double)(xC + 2 * chX);
    double v9 = 100.0 - sqrt(cx * cx + cz * cz) * 8.0;
    v9 = Mth::clamp(v9, -100.0, 80.0);

    // 外岛
    int wX_start = (int)(chX - 12.0);
    int wZ_start = (int)(chZ - 12.0);
    int v28_start = xC + 24;
    int v17_start = zC + 24;

    for (int i = 0; i < 25; i++) {
        int wX = wX_start + i;
        int v28 = v28_start - 2 * i;
        for (int j = 0; j < 25; j++) {
            int wZ = wZ_start + j;
            int v17 = v17_start - 2 * j;
            if ((int64_t)wX * wX + (int64_t)wZ * wZ > 4096) {
                if (sNoise1.getValue((double)wX, (double)wZ) < -0.89999998) {
                    int mul = (147 * abs(wZ) + 3439 * abs(wX)) % 13 + 9;
                    double v20 = 100.0 - hypot((double)v17, (double)v28) * (double)mul;
                    v20 = Mth::clamp(v20, -100.0, 80.0);
                    v9 = Mth::Max(v20, v9);
                }
            }
        }
    }
    return v9;
}
// ========== Density Cells ==========

// 文件：src/world/level/levelgen/TheEndLevelSource.cpp
void TheEndLevelSource::generateDensityCells(int64_t chunkX, int64_t chunkZ, double* density) {
    // 🛡️ 噪声原点：世界块坐标 × 缩放 + 偏移 × 缩放 → 除以 8（半chunk）
    //   origin = (chunk*16 * scale + offset * scale) / 8
    //          = chunk*2 * scale + offset * scale / 8
    double originX = (double)(chunkX * 2) * m_worldScaleX + m_worldOffsetX * m_worldScaleX / 8.0;
    double originY = m_worldOffsetY * m_worldScaleY / 8.0;
    double originZ = (double)(chunkZ * 2) * m_worldScaleZ + m_worldOffsetZ * m_worldScaleZ / 8.0;

    // 🛡️ 噪声频率：基础值 × 各轴独立缩放
    const double B_XZ  = 1368.824;
    const double B_Y   = 684.412;
    const double BS_XZ = 17.1103;
    const double BS_Y  = 4.277575;

    double sx  = B_XZ  * m_worldScaleX;
    double sy  = B_Y   * m_worldScaleY;
    double sz  = B_XZ  * m_worldScaleZ;
    double ssx = BS_XZ * m_worldScaleX;
    double ssy = BS_Y  * m_worldScaleY;
    double ssz = BS_XZ * m_worldScaleZ;

    double* n1 = pNoise1.getRegion(nullptr, originX, originY, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z, sx, sy, sz);
    double* n2 = pNoise2.getRegion(nullptr, originX, originY, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z, sx, sy, sz);
    double* n3 = pNoise3.getRegion(nullptr, originX, originY, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z, ssx, ssy, ssz);

    for (int xC = 0; xC < DENSITY_X; xC++) {
        for (int zC = 0; zC < DENSITY_Z; zC++) {
            double hV = getIslandHeightValue(chunkX, chunkZ, xC, zC);
            double v23 = 7.0;

            for (int yC = 0; yC < DENSITY_Y; yC += 3, v23 -= 3.0) {
                int idx = yC + DENSITY_Y * (zC + DENSITY_Z * xC);

                for (int d = 0; d < 3; d++) {
                    int ind = idx + d;

                    double s  = Mth::clamp(n3[ind] / 20.0 + 0.5, 0.0, 1.0);
                    double v4 = n1[ind] / 512.0
                              + (n2[ind] / 512.0 - n1[ind] / 512.0) * s
                              + hV - 8.0;
                    double v6;

                    if (yC < 14) {
                        v6 = v4;
                        if (yC < 9) {
                            double t = (v23 + 1.0 - (double)d) / 7.0;
                            if (t < 0.0) t = 0.0;
                            if (t > 1.0) t = 1.0;
                            v6 = (1.0 - t) * v4 - t * 30.0;
                        }
                    } else {
                        double t = ((double)(yC - 14) + (double)d) / 64.0;
                        if (t < 0.0) t = 0.0;
                        if (t > 1.0) t = 1.0;
                        v6 = (1.0 - t) * v4 - t * 3000.0;
                    }

                    density[ind] = v6;
                }
            }
        }
    }

    delete[] n1;
    delete[] n2;
    delete[] n3;
}

// ========== Prepare Heights ==========

// 文件：src/world/level/levelgen/TheEndLevelSource.cpp
void TheEndLevelSource::prepareHeights(int64_t chunkX, int64_t chunkZ, unsigned char* blocks) {
    generateDensityCells(chunkX, chunkZ, densityBuffer);
    memset(blocks, 0, LevelChunk::ChunkBlockCount);

    // JS: for xCH in 0..1, zCH in 0..1, yCH in 0..31
    for (int xCH = 0; xCH < 2; xCH++) {
        for (int zCH = 0; zCH < 2; zCH++) {
            for (int yCH = 0; yCH < 32; yCH++) {
                // v23 = yCH + 33*(zCH + 3*xCH)
                int baseIdx = yCH + DENSITY_Y * (zCH + DENSITY_Z * xCH);

                // 8 个角: Y=yCH 面 4 个 + Y=yCH+1 面 4 个
                double c000 = densityBuffer[baseIdx];
                double c001 = densityBuffer[baseIdx + DENSITY_Y];
                double c010 = densityBuffer[baseIdx + DENSITY_Y * DENSITY_Z];
                double c011 = densityBuffer[baseIdx + DENSITY_Y + DENSITY_Y * DENSITY_Z];
                double c100 = densityBuffer[baseIdx + 1];
                double c101 = densityBuffer[baseIdx + 1 + DENSITY_Y];
                double c110 = densityBuffer[baseIdx + 1 + DENSITY_Y * DENSITY_Z];
                double c111 = densityBuffer[baseIdx + 1 + DENSITY_Y + DENSITY_Y * DENSITY_Z];

                // JS: for zCL in 0..7, xCL in 0..7, yCL in 0..3
                for (int zCL = 0; zCL < 8; zCL++) {
                    double fz = zCL / 8.0;
                    int zP = zCH * 8 + zCL;

                    // Z 方向双线性插值 (Y=yCH 和 Y=yCH+1)
                    double z0_y0 = c000 + (c001 - c000) * fz;
                    double z1_y0 = c010 + (c011 - c010) * fz;
                    double z0_y1 = c100 + (c101 - c100) * fz;
                    double z1_y1 = c110 + (c111 - c110) * fz;

                    for (int xCL = 0; xCL < 8; xCL++) {
                        double fx = xCL / 8.0;
                        int xP = xCH * 8 + xCL;

                        // X 方向插值
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
// ========== ChunkSource interface ==========

// 文件：src/world/level/levelgen/TheEndLevelSource.cpp → create()

LevelChunk* TheEndLevelSource::create(int64_t x, int64_t z) {
    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    LevelChunk* levelChunk = new LevelChunk(level, blocks, x, z);

    int64_t hashedPos = (x << 32) | (z & 0xffffffff);
    chunkMap.insert(std::make_pair(hashedPos, levelChunk));

    prepareHeights(x, z, blocks);

    // 🛡️ 末地不需要光照更新 — 全部设为最高亮度 15
    // DataLayer 每字节存两个 4-bit nibble，0xFF = 两个 15
    memset(levelChunk->skyLight.data, 0xFF, LevelChunk::ChunkBlockCount / 2);
    // blockLight 已经是 0（构造时 setAll(0)），末地无发光方块，不需改

    levelChunk->recalcHeightmapOnly();

    return levelChunk;
}

LevelChunk* TheEndLevelSource::getChunk(int64_t x, int64_t z) {
    int64_t hashedPos = (x << 32) | (z & 0xffffffff);
    auto it = chunkMap.find(hashedPos);
    if (it != chunkMap.end()) return it->second;
    return create(x, z);  // ← create 已 insert，直接返回
}

bool TheEndLevelSource::hasChunk(int64_t x, int64_t z) {
    return true;
}

void TheEndLevelSource::postProcess(ChunkSource* parent, int64_t xt, int64_t zt) {
    // 末地暂不生成结构（未来可加末地城）
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
    return Biome::MobList(); // 末地暂不刷怪
}
