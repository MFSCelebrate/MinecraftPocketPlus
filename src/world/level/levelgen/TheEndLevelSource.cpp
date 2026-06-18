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

double TheEndLevelSource::getIslandHeightValue(int64_t chunkX, int64_t chunkZ, int xC, int zC) {
    // 中央岛: 距离中心越远越低
    double cx = (double)(zC + 2 * chunkZ);
    double cz = (double)(xC + 2 * chunkX);
    double v9 = 100.0 - sqrt(cx * cx + cz * cz) * 8.0;
    v9 = Mth::clamp(v9, -100.0, 80.0);

    // 外岛: SimplexNoise 检测 + 距离
    for (int x = (int)(chunkX * 2) - 12, xEnd = x + 25; x < xEnd; x++) {
        for (int z = (int)(chunkZ * 2) - 12, zEnd = z + 25; z < zEnd; z++) {
            if ((int64_t)x * x + (int64_t)z * z > 4096) {
                if (sNoise1.getValue((double)x, (double)z) < -0.89999998) {
                    double dx = (double)((z + 12) * 2 - (chunkZ * 2));
                    double dz = (double)((x + 12) * 2 - (chunkX * 2));
                    double v20 = 100.0 - hypot(dx, dz) * 
                        (double)((147 * abs(z) + 3439 * abs(x)) % 13 + 9);
                    v20 = Mth::clamp(v20, -100.0, 80.0);
                    v9 = Mth::Max(v20, v9);
                }
            }
        }
    }

    return v9;
}

// ========== Density Cells ==========

void TheEndLevelSource::generateDensityCells(int64_t chunkX, int64_t chunkZ, double* density) {
    const double SX = 1368.824;   // 684.412 * 2
    const double SY = 684.412;
    const double SZ = 1368.824;
    const double S_SMALL = 17.1103; // SX / 80

    double originX = (double)(chunkX * 2);
    double originZ = (double)(chunkZ * 2);

    // 获取三组噪声
    double* n1 = pNoise1.getRegion(nullptr,
        originX, 0.0, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z,
        SX, SY, SZ);
    double* n2 = pNoise2.getRegion(nullptr,
        originX, 0.0, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z,
        SX, SY, SZ);
    double* n3 = pNoise3.getRegion(nullptr,
        originX, 0.0, originZ,
        DENSITY_X, DENSITY_Y, DENSITY_Z,
        S_SMALL, S_SMALL / 4.0, S_SMALL);

    int idx = 0;
    for (int xC = 0; xC < DENSITY_X; xC++) {
        for (int zC = 0; zC < DENSITY_Z; zC++) {
            double hV = getIslandHeightValue(chunkX, chunkZ, xC, zC);
            double v23 = 7.0; // y 从顶部向下

            for (int yC = 0; yC < DENSITY_Y; yC++, v23 -= 1.0) {
                // 中=64(索引8), 底=0(索引0), 顶=128(索引16)
                // 索引0→y=0~7, 索引8→y=64~71, 索引16→y=128~135
                double yInChunk = yC * 8; // 实际 y 坐标

                // 选择器: clampedLerp
                double s = Mth::clamp(n3[idx] / 20.0 + 0.5, 0.0, 1.0);
                double terrainNoise = n1[idx] / 512.0 + (n2[idx] / 512.0 - n1[idx] / 512.0) * s;
                
                double v4 = terrainNoise + hV - 8.0;
                double v6;

                if (yInChunk < 64) {
                    // 岛的下半部
                    v6 = v4;
                    if (yInChunk < 32) {
                        // 岛底: 向下收缩
                        double t = (v23 + 1.0) / 7.0; // v23 从 7 递减
                        if (t < 0.0) t = 0.0;
                        if (t > 1.0) t = 1.0;
                        v6 = (1.0 - t) * v4 - t * 30.0;
                    }
                } else {
                    // 岛顶: 向上收缩
                    double t = (yInChunk - 64.0) / 64.0;
                    if (t < 0.0) t = 0.0;
                    if (t > 1.0) t = 1.0;
                    v6 = (1.0 - t) * v4 - t * 3000.0;
                }

                density[idx] = v6;
                idx++;
            }
        }
    }

    delete[] n1; delete[] n2; delete[] n3;
}

// ========== Prepare Heights ==========

void TheEndLevelSource::prepareHeights(int64_t chunkX, int64_t chunkZ, unsigned char* blocks) {
    generateDensityCells(chunkX, chunkZ, densityBuffer);

    // 清空区块
    memset(blocks, 0, LevelChunk::ChunkBlockCount);

    // 对每个 8×4×8 的 cell 做三线性插值
    for (int xCH = 0; xCH < 2; xCH++) {       // 2 cells in X per chunk
        for (int zCH = 0; zCH < 2; zCH++) {   // 2 cells in Z per chunk
            int baseIdx = DENSITY_Y * (zCH + DENSITY_Z * xCH);
            
            for (int yCH = 0; yCH < DENSITY_Y - 1; yCH++) { // each Y cell
                int idx000 = baseIdx + yCH;
                int idx001 = baseIdx + yCH + DENSITY_Y;
                int idx010 = baseIdx + yCH + DENSITY_Y * DENSITY_Z;
                int idx011 = baseIdx + yCH + DENSITY_Y + DENSITY_Y * DENSITY_Z;

                double d000 = densityBuffer[idx000];
                double d001 = densityBuffer[idx000 + 1];
                double d010 = densityBuffer[idx001];
                double d011 = densityBuffer[idx001 + 1];
                double d100 = densityBuffer[idx010];
                double d101 = densityBuffer[idx010 + 1];
                double d110 = densityBuffer[idx011];
                double d111 = densityBuffer[idx011 + 1];

                for (int zL = 0; zL < 8; zL++) {
                    double fz = zL / 8.0;
                    double dz00 = d000 + (d010 - d000) * fz;
                    double dz01 = d001 + (d011 - d001) * fz;
                    double dz10 = d100 + (d110 - d100) * fz;
                    double dz11 = d101 + (d111 - d101) * fz;

                    int zP = zCH * 8 + zL;

                    for (int xL = 0; xL < 8; xL++) {
                        double fx = xL / 8.0;
                        double vx0 = dz00 + (dz10 - dz00) * fx;
                        double vx1 = dz01 + (dz11 - dz01) * fx;

                        int xP = xCH * 8 + xL;

                        for (int yL = 0; yL < 8; yL++) {
                            double fy = yL / 8.0;
                            double val = vx0 + (vx1 - vx0) * fy;

                            if (val > 0.0) {
                                int yP = yCH * 8 + yL;
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

LevelChunk* TheEndLevelSource::create(int64_t x, int64_t z) {
    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];

    LevelChunk* levelChunk = new LevelChunk(level, blocks, x, z);

    int64_t hashedPos = (x << 32) | (z & 0xffffffff);  // 🔧 自己算 hash
    chunkMap.insert(std::make_pair(hashedPos, levelChunk));

    prepareHeights(x, z, blocks);

    // 手动填 heightmap
    levelChunk->recalcHeightmapOnly();  // ← 🛡️ 替代手动填 heightmap

    return levelChunk;
}

LevelChunk* TheEndLevelSource::getChunk(int64_t x, int64_t z) {
    int64_t hashedPos = (x << 32) | (z & 0xffffffff);
    auto it = chunkMap.find(hashedPos);
    if (it != chunkMap.end()) return it->second;
    return create(x, z);  // create 已插入 chunkMap，直接返回
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
