#include "TheEndLevelSource.h"
#include "../Level.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"

TheEndLevelSource::TheEndLevelSource(Level* level, long seed)
    : random(seed),
      level(level),
      pNoise1(&random, 16),
      pNoise2(&random, 16),
      pNoise3(&random, 8),
      sNoise1(&random, 4),
      densityBuffer(nullptr)
{
    densityBuffer = new double[DENSITY_X * DENSITY_Y * DENSITY_Z];
}

TheEndLevelSource::~TheEndLevelSource() {
    delete[] densityBuffer;
}

// ========== Island Height ==========

// 文件：src/world/level/levelgen/TheEndLevelSource.cpp

double TheEndLevelSource::getIslandHeightValue(int64_t chunkX, int64_t chunkZ, int xC, int zC) {
    // === 中央岛基础高度 ===
    double cx = (double)(zC + 2 * chunkZ);
    double cz = (double)(xC + 2 * chunkX);
    double v9 = 100.0 - sqrt(cx * cx + cz * cz) * 8.0;
    v9 = Mth::clamp(v9, -100.0, 80.0);

    // === 外岛检测 ===
    // JS: for (v25=25, v11=x-12, v28=xC+24; v25; v25--, v11++, v28-=2)
    int v11_start = (int)chunkX - 12;
    int v28_start = xC + 24;

    for (int i = 0; i < 25; i++) {
        int v11 = v11_start + i;
        int v28 = v28_start - 2 * i;

        int v16_start = (int)chunkZ - 12;
        int v17_start = zC + 24;

        for (int j = 0; j < 25; j++) {
            int v16 = v16_start + j;
            int v17 = v17_start - 2 * j;

            // JS: if (v11*v11 + v16*v16 > 4096)
            // v11/v16 是 chunk 坐标，4096 = 64²，即距离原点 > 64 chunks
            if ((int64_t)v11 * v11 + (int64_t)v16 * v16 > 4096) {

                // JS: if (sNoise1.getValue(v11, v16) < -0.89999998)
                if (sNoise1.getValue((double)v11, (double)v16) < -0.89999998) {

                    // JS: (147*abs(v16) + 3439*abs(v11)) % 13 + 9
                    int multiplier = (147 * abs(v16) + 3439 * abs(v11)) % 13 + 9;

                    // JS: 100 - hypot(v17, v28) * multiplier
                    double v20 = 100.0 - hypot((double)v17, (double)v28) * (double)multiplier;
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
    const double SX = 1368.824;
    const double SY = 684.412;
    const double SZ = 1368.824;
    const double S_SMALL = 17.1103;
    const double S_SMALL_Y = 4.277575;

    double originX = (double)(chunkX * 2);
    double originZ = (double)(chunkZ * 2);

    double* n1 = pNoise1.getRegion(nullptr, originX, 0.0, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z, SX, SY, SZ);
    double* n2 = pNoise2.getRegion(nullptr, originX, 0.0, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z, SX, SY, SZ);
    double* n3 = pNoise3.getRegion(nullptr, originX, 0.0, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z, S_SMALL, S_SMALL_Y, S_SMALL);

    for (int xC = 0; xC < DENSITY_X; xC++) {
        for (int zC = 0; zC < DENSITY_Z; zC++) {
            double hV = getIslandHeightValue(chunkX, chunkZ, xC, zC);
            double v23 = 7.0;

            // JS: for (yC = 0, v23 = 7; yC < 33; yC += 3, v23 -= 3)
            for (int yC = 0; yC < DENSITY_Y; yC += 3, v23 -= 3.0) {
                int idx = yC + DENSITY_Y * (zC + DENSITY_Z * xC);

                // 3 个连续 Y 采样点
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
                            // 🛡️ JS: Vec3(v23+1, v23, v23-1).scale(1/7) → per-point t
                            double t = (v23 + 1.0 - (double)d) / 7.0;
                            if (t < 0.0) t = 0.0;
                            if (t > 1.0) t = 1.0;
                            v6 = (1.0 - t) * v4 - t * 30.0;
                        }
                    } else {
                        // 🛡️ JS: Vec3(yC-14, yC-13, yC-12).scale(1/64).clamp → per-point t
                        double t = ((double)(yC - 14) + (double)d) / 64.0;
                        if (t < 0.0) t = 0.0;
                        if (t > 1.0) t = 1.0;
                        v6 = (1.0 v4 - t * 3000.0;
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
