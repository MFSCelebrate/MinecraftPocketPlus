#include "TheEndLevelSource.h"
#include "../Level.h"
#include "../chunk/LevelChunk.h"
#include "../tile/Tile.h"
#include "../../../util/Mth.h"
#include "../../../util/Mth.h"
#include "../../../client/Minecraft.h"

TheEndLevelSource::TheEndLevelSource(Level* level, long seed)
    : random(seed), level(level),
      pNoise1(&random, 16), pNo(&random, 16), pNoise3(&random, 8),
      sNoise1(&random, 4), densityBuffer(nullptr)
{
    densityBuffer = new double[DENSITY_X * DENSITY_Y * DENSITY_Z];

    if (Minecraft::instance) {
        std::string v;
        v = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_X);
        if (!v.empty()) m_worldScaleX = atof(v.c_str());
        v = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_Y);
        if (!v.empty()) m_worldScaleY = atof(v.c_str());
        v = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_Z);
        if (!v.empty()) m_worldScaleZ = atof(v.c_str());

        v = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_X);
        if (!v.empty()) m_worldOffsetX = atof(v.c_str());
        v = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_Y);
        if (!v.empty()) m_worldOffsetY = atof(v.c_str());
        v = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_Z);
        if (!v.empty()) m_worldOffsetZ = atof(v.c_str());

        enableCircles = Minecraft::instance->options.getBooleanValue(OPTIONS_END_CIRCLES);
    }
}

TheEndLevelSource::~TheEndLevelSource() {
    delete[] densityBuffer;
}

double TheEndLevelSource::getIslandHeightValue(int64_t chunkX, int64_t chunkZ, int xC, int zC) {
    double chX = (double)chunkX + m_worldOffsetX * m_worldScaleX / 16.0;
    double chZ = (double)chunkZ + m_worldOffsetZ * m_worldScaleZ / 16.0;

    double v9;
    if (enableCircles) {
        int ix = zC + 2 * (int)chZ;
        int iz = xC + 2 * (int)chX;
        float distSq = (float)(ix * ix + iz * iz);
        v9 = (double)(100.0f - Mth::sqrt(distSq) * 8.0f);
    } else {
        double cx = (double)(zC + 2 * chZ);
        double cz = (double)(xC + 2 * chX);
        v9 = 100.0 - sqrt(cx * cx + cz * cz) * 8.0;
    }
    v9 = Mth::clamp(v9, -100.0, 80.0);

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

            bool outsideCentral;
            if (enableCircles) {
                outsideCentral = (wX * wX + wZ * wZ > 4096);
            } else {
                outsideCentral = ((int64_t)wX * wX + (int64_t)wZ * wZ > 4096);
            }

            if (outsideCentral) {
                if (sNoise1.getValue((double)wX, (double)wZ) < -0.89999998) {
                    int mul = (147 * abs(wZ) + 3439 * abs(wX)) % 13 + 9;
                    double v20;
                    if (enableCircles) {
                        float v20f = 100.0f - Mth::sqrt((float)(v17 * v17 + v28 * v28)) * (float)mul;
                        v20 = (double)v20f;
                    } else {
                        v20 = 100.0 - hypot((double)v17, (double)v28) * (double)mul;
                    }
                    v20 = Mth::clamp(v20, -100.0, 80.0);
                    if (v20 > v9) v9 = v20;
                }
            }
        }
    }
    return v9;
}

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

    // 🛡️ 防止大缩放导致 int 溢出：折回 origin 取模 256
    const double P = 256.0;
    originX = fmod(originX, P);
    if (originX < 0.0) originX += P;
    originY = fmod(originY, P);
    if (originY < 0.0) originY += P;
    originZ = fmod(originZ, P);
    if (originZ < 0.0) originZ += P;

    // 🛡️ 钳制噪声频率防止 int 溢出
    const double MAX_SAFE = 5.0e8;
    if (sx > MAX_SAFE || sx < -MAX_SAFE) sx = (sx > 0) ? MAX_SAFE : -MAX_SAFE;
    if (sy > MAX_SAFE || sy < -MAX_SAFE) sy = (sy > 0) ? MAX_SAFE : -MAX_SAFE;
    if (sz > MAX_SAFE || sz < -MAX_SAFE) sz = (sz > 0) ? MAX_SAFE : -MAX_SAFE;
    if (ssx > MAX_SAFE || ssx < -MAX_SAFE) ssx = (ssx > 0) ? MAX_SAFE : -MAX_SAFE;
    if (ssy > MAX_SAFE || ssy < -MAX_SAFE) ssy = (ssy > 0) ? MAX_SAFE : -MAX_SAFE;
    if (ssz > MAX_SAFE || ssz < -MAX_SAFE) ssz = (ssz > 0) ? MAX_SAFE : -MAX_SAFE;

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
                        double t = ((double)(yC - 14) + (double 0;
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

void TheEndLevelSource::prepareHeights(int64_t chunkX, int64_t chunkZ, unsigned char* blocks) {
    generateDensityCells(chunkX, chunkZ, densityBuffer);
    memset(blocks, 0, LevelChunk::ChunkBlockCount);

    const int DZ = DENSITY_Y;
    const int DX = DENSITY_Y * DENSITY_Z;

    for (int xCH = 0; xCH < 2; xCH++) {
        for (int zCH = 0; zCH < 2; zCH++) {
            for (int yCH = 0; yCH < 32; yCH++) {
                int base = yCH + DZ * (zCH + DENSITY_Z * xCH);

                double c000 = densityBuffer[base];
                double c001 = densityBuffer[base + DZ];
                double c010 = densityBuffer[base + DX];
                double c011 = densityBuffer[base + DX + DZ];
                double c100 = densityBuffer[base + 1];
                double c101 = densityBuffer[base + 1 + DZ];
                double c110 = densityBuffer[base + 1 + DX];
                double c111 = densityBuffer[base + 1 + DX + DZ];

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

LevelChunk* TheEndLevelSource::create(int64_t x, int64_t z) {
    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    LevelChunk* levelChunk = new LevelChunk(level, blocks, x, z);

    int64_t hashedPos = (x << 32) | (z & 0xffffffff);
    chunkMap.insert(std::make_pair(hashedPos, levelChunk));

    prepareHeights(x, z, blocks);
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

void TheEndLevelSource::postProcess(ChunkSource* parent, int64_t xt, int64_t zt) {}

bool TheEndLevelSource::tick() { return false; }

bool TheEndLevelSource::shouldSave() { return true; }

std::string TheEndLevelSource::gatherStats() { return "TheEndLevelSource"; }

Biome::MobList TheEndLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
    return Biome::MobList();
}
