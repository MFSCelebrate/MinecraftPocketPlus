#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__

#include <map>
#include "world/level/chunk/ChunkSource.h"
#include "LargeCaveFeature.h"
#include "synth/PerlinNoise.h"
#include "../../../SharedConstants.h"

#include "../../util/WorldCoordinate.h"

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
    inline double getLPerlinNoise1(double x, double y, double z) { return lperlinNoise1.getValue(x, y, z); }
    inline double getLPerlinNoise2(double x, double y, double z) { return lperlinNoise2.getValue(x, y, z); }
    inline double getPerlinNoise1(double x, double y, double z) { return perlinNoise1.getValue(x, y, z); }
    inline double getPerlinNoise2(double x, double z) { return perlinNoise2.getValue(x, z); }
    inline double getPerlinNoise3(double x, double z) { return perlinNoise3.getValue(x, z); }
    inline double getScaleNoise(double x, double z) { return scaleNoise.getValue(x, z); }
    inline double getDepthNoise(double x, double z) { return depthNoise.getValue(x, z); }
    inline double getForestNoise(double x, double z) { return forestNoise.getValue(x, z); }

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

    double getWorldOffsetX() const {
    double v = m_worldOffsetX.convert_to<double>();
}
double getWorldOffsetY() const {
    double v = m_worldOffsetY.convert_to<double>();
}
double getWorldOffsetZ() const {
    double v = m_worldOffsetZ.convert_to<double>();
}
double getWorldScaleX() const {
    double v = m_worldScaleX.convert_to<double>();
}
double getWorldScaleY() const {
    double v = m_worldScaleY.convert_to<double>();
}
double getWorldScaleZ() const {
    double v = m_worldScaleZ.convert_to<double>();
}

double getWorldOffsetX() const { return worldCoordToDouble(m_worldOffsetX); }
double getWorldOffsetY() const { return worldCoordToDouble(m_worldOffsetY); }
double getWorldOffsetZ() const { return worldCoordToDouble(m_worldOffsetZ); }
double getWorldScaleX()  const { return worldCoordToDouble(m_worldScaleX); }
double getWorldScaleY()  const { return worldCoordToDouble(m_worldScaleY); }
double getWorldScaleZ()  const { return worldCoordToDouble(m_worldScaleZ); }

// ===== 新 getter（字符串，给调试屏幕） =====
std::string getStrOffsetX() const { return worldCoordToString(m_worldOffsetX); }
std::string getStrOffsetY() const { return worldCoordToString(m_worldOffsetY); }
std::string getStrOffsetZ() const { return worldCoordToString(m_worldOffsetZ); }
std::string getStrScaleX()  const { return worldCoordToString(m_worldScaleX); }
std::string getStrScaleY()  const { return worldCoordToString(m_worldScaleY); }
std::string getStrScaleZ()  const { return worldCoordToString(m_worldScaleZ); }

// ===== 新 getter（裸值，给 cpp_int 计算路径） =====
WorldCoordinate getRawOffsetX() const { return m_worldOffsetX; }
WorldCoordinate getRawOffsetY() const { return m_worldOffsetY; }
WorldCoordinate getRawOffsetZ() const { return m_worldOffsetZ; }
WorldCoordinate getRawScaleX()  const { return m_worldScaleX; }
WorldCoordinate getRawScaleY()  const { return m_worldScaleY; }
WorldCoordinate getRawScaleZ()  const { return m_worldScaleZ; }

    void setWorldOffset(double ox, double oy, double oz) {
    // 把 double 转成定点存储
    m_worldOffsetX = (WorldCoordinate)(ox * FIXED_SCALE_D);
    m_worldOffsetY = (WorldCoordinate)(oy * FIXED_SCALE_D);
    m_worldOffsetZ = (WorldCoordinate)(oz * FIXED_SCALE_D);
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
    PerlinNoise lperlinNoise1;
    PerlinNoise lperlinNoise2;
    PerlinNoise perlinNoise1;
    PerlinNoise perlinNoise2;
    PerlinNoise perlinNoise3;
    PerlinNoise scaleNoise;
    PerlinNoise depthNoise;
    PerlinNoise forestNoise;

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

    WorldCoordinate m_worldOffsetX = 0;
WorldCoordinate m_worldOffsetY = 0;
WorldCoordinate m_worldOffsetZ = 0;
WorldCoordinate m_worldScaleX = FIXED_SCALE;
WorldCoordinate m_worldScaleY = FIXED_SCALE;
WorldCoordinate m_worldScaleZ = FIXED_SCALE;

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
