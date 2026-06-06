#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__

#include <map>
#include "world/level/chunk/ChunkSource.h"
#include "LargeCaveFeature.h"
#include "synth/PerlinNoise.h"
#include "../../../SharedConstants.h"
#include <boost/multiprecision/cpp_dec_float.hpp>

// ===== 在类定义之前（或 namespace 内）定义类型别名 =====
// RandomLevelSource.h
typedef boost::multiprecision::cpp_dec_float_50 big_float;

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

const big_float& getBigWorldOffsetX() const { return m_worldOffsetX; }
const big_float& getBigWorldOffsetY() const { return m_worldOffsetY; }
const big_float& getBigWorldOffsetZ() const { return m_worldOffsetZ; }
const big_float& getBigWorldScaleX()  const { return m_worldScaleX; }
const big_float& getBigWorldScaleY()  const { return m_worldScaleY; }
const big_float& getBigWorldScaleZ()  const { return m_worldScaleZ; }

    void setWorldOffset(double ox, double oy, double oz) {
    m_worldOffsetX = big_float(ox);
    m_worldOffsetY = big_float(oy);
    m_worldOffsetZ = big_float(oz);
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

    big_float m_worldOffsetX = 0;
big_float m_worldOffsetY = 0;
big_float m_worldOffsetZ = 0;
big_float m_worldScaleX = 1;
big_float m_worldScaleY = 1;
big_float m_worldScaleZ = 1;

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
