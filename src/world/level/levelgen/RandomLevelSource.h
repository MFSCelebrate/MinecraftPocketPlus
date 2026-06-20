#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__

#include <map>
#include "world/level/chunk/ChunkSource.h"
#include "LargeCaveFeature.h"
#include "synth/PerlinNoise.h"
#include "../../../SharedConstants.h"

#include "../../../util/WorldCoordinate.h"

class Biome;
class Level;
class LevelChunk;

typedef std::map<int64_t, LevelChunk*> ChunkMap;

class RandomLevelSource : public ChunkSource
{
public:
    static const float SNOW_CUTOFF;
    static const float SNOW_SCALE;
    static const int CHUNK_HEIGHT = 8;
    static const int CHUNK_WIDTH = 4;

    // 噪声查询接口（全部 double 精度）
    inline double getLPerlinNoise1(double x, double y, double z) {
    if (m_useDoubleNoise) return lperlinNoise1->getValue(x, y, z);
    else return (double)lperlinNoise1_f->getValue((float)x, (float)y, (float)z);
}
inline double getLPerlinNoise2(double x, double y, double z) {
    if (m_useDoubleNoise) return lperlinNoise2->getValue(x, y, z);
    else return (double)lperlinNoise2_f->getValue((float)x, (float)y, (float)z);
}
inline double getPerlinNoise1(double x, double y, double z) {
    if (m_useDoubleNoise)perlinNoise1->getValue(x, y, z);
    else return (double)perlinNoise1_f->getValue((float)x, (float)y, (float)z);
}
inline double getPerlinNoise2(double x, double z) {
    if (m_useDoubleNoise) return perlinNoise2->getValue(x, z);
    else return (double)perlinNoise2_f->getValue((float)x, (float)z);
}
inline double getPerlinNoise3(double x, double z) {
    if (m_useDoubleNoise) return perlinNoise3->getValue(x, z);
    else return (double)perlinNoise3_f->getValue((float)x, (float)z);
}
inline double getScaleNoise(double x, double z) {
    if (m_useDoubleNoise) return scaleNoise->getValue(x, z);
    else return (double)scaleNoise_f->getValue((float)x, (float)z);
}
inline double getDepthNoise(double x, double z) {
    if (m_useDoubleNoise) return depthNoise->getValue(x, z);
    else return (double)depthNoise_f->getValue((float)x, (float)z);
}
inline double getForestNoise(double x, double z) {
    if (m_useDoubleNoise) return forestNoise->getValue(x, z);
    else return (double)forestNoise_f->getValue((float)x, (float)z);
}
    RandomLevelSource(Level* level, long seed, int version, bool spawnMobs);
    ~RandomLevelSource();

    virtual bool hasChunk(int64_t x, int64_t z) override;
    virtual LevelChunk* create(int64_t x, int64_t z) override;
    virtual LevelChunk* getChunk(int64_t xOffs, int64_t zOffs) override;
    virtual void postProcess(ChunkSource* parent, int64_t xt, int64_t zt) override;
    virtual bool tick() override;
    virtual Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) override;
    virtual bool shouldSave() override;
    virtual std::string gatherStats() override;

    double getWorldOffsetX() const { return m_worldOffsetX; }
double getWorldOffsetY() const { return m_worldOffsetY; }
double getWorldOffsetZ() const { return m_worldOffsetZ; }
double getWorldScaleX()  const { return m_worldScaleX; }
double getWorldScaleY()  const { return m_worldScaleY; }
double getWorldScaleZ()  const { return m_worldScaleZ; }

    void setWorldOffset(double ox, double oy, double oz) {
    m_worldOffsetX = ox;
    m_worldOffsetY = oy;
    m_worldOffsetZ = oz;
}

    int getSeaLevel() const { return customSeaLevel; }

    LargeCaveFeature caveFeature;
    int waterDepths[16+16][16+16];

private:
    double* getHeights(double* buffer, double x, int y, double z, int xSize, int ySize, int zSize);
    void prepareHeights(double xOffs, double zOffs, unsigned char* blocks, void* biomes, double* temperatures);
    void buildSurfaces(double xOffs, double zOffs, unsigned char* blocks, Biome** biomes);
    void calcWaterDepths(ChunkSource* parent, int64_t xt, int64_t zt);

    ChunkMap chunkMap;
    Random random;
    // ============ 成员声明 — 替换原有 8 行 ============
PerlinNoise*        lperlinNoise1 = nullptr;
PerlinNoise*        lperlinNoise2 = nullptr;
PerlinNoise*        perlinNoise1  = nullptr;
PerlinNoise*        perlinNoise2  = nullptr;
PerlinNoise*        perlinNoise3  = nullptr;
PerlinNoise*        scaleNoise    = nullptr;
PerlinNoise*        depthNoise    = nullptr;
PerlinNoise*        forestNoise   = nullptr;

PerlinNoiseT<float>* lperlinNoise1_f = nullptr;
PerlinNoiseT<float>* lperlinNoise2_f = nullptr;
PerlinNoiseT<float>* perlinNoise1_f  = nullptr;
PerlinNoiseT<float>* perlinNoise2_f  = nullptr;
PerlinNoiseT<float>* perlinNoise3_f  = nullptr;
PerlinNoiseT<float>* scaleNoise_f    = nullptr;
PerlinNoiseT<float>* depthNoise_f    = nullptr;
PerlinNoiseT<float>* forestNoise_f   = nullptr;

bool m_useDoubleNoise = true;

    Level* level;
    bool spawnMobs;

    double* buffer;                // 通用缓冲区，改为 double
    double sandBuffer[16*16];
    double gravelBuffer[16*16];
    double depthBuffer[16*16];
    double* pnr;
    double* ar;
    double* br;
    double* sr;
    double* dr;
    double* fi;
    double* fis;

    double m_worldOffsetX = 0.0;
double m_worldOffsetY = 0.0;
double m_worldOffsetZ = 0.0;
double m_worldScaleX = 1.0;
double m_worldScaleY = 1.0;
double m_worldScaleZ = 1.0;

    int customSeaLevel;

    bool m_disableSkygrid;
};

class PerformanceTestChunkSource : public ChunkSource
{
    Level* level;
public:
    PerformanceTestChunkSource(Level* level) : ChunkSource(), level(level) {}
    virtual bool hasChunk(int64_t x, int64_t z) override { return true; }
    virtual LevelChunk* getChunk(int64_t x, int64_t z) override { return create(x, z); }
    virtual LevelChunk* create(int64_t x, int64_t z) override;
    virtual void postProcess(ChunkSource* parent, int64_t x, int64_t z) override {}
    virtual bool tick() override { return false; }
    virtual bool shouldSave() override { return false; }
    virtual std::string gatherStats() override { return "PerformanceTestChunkSource"; }
};

#endif
