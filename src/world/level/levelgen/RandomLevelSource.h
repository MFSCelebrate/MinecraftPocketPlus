#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__RandomLevelSource_H__

class Biome;
class Level;
class LevelChunk;

#include <map>
typedef std::map<int64_t, LevelChunk*> ChunkMap;

#include "../chunk/ChunkSource.h"
#include "LargeCaveFeature.h"
#include "synth/PerlinNoise.h"
#include "../../../SharedConstants.h"

class RandomLevelSource: public ChunkSource
{
    static const float SNOW_CUTOFF;
    static const float SNOW_SCALE;

public:
    // 3D 噪声访问器（传入 XYZ）
inline double getLPerlinNoise1(double x, double y, double z) const { return lperlinNoise1.getValue(x, y, z); }
inline double getLPerlinNoise2(double x, double y, double z) const { return lperlinNoise2.getValue(x, y, z); }
inline double getPerlinNoise1 (double x, double y, double z) const { return perlinNoise1.getValue(x, y, z); }

// 2D 版也提供 double 版
inline double getPerlinNoise2(double x, double z) const { return perlinNoise2.getValue(x, z); }
// … 其他 2D 噪声类似，如果需要的话
    inline double getPerlinNoise3(double x, double z) { return perlinNoise3.getValue(x, z); }
    inline double getScaleNoise(double x, double z) { return scaleNoise.getValue(x, z); }
    inline double getDepthNoise(double x, double z) { return depthNoise.getValue(x, z); }
    inline double getForestNoise(double x, double z) { return forestNoise.getValue(x, z); }

    static const int CHUNK_HEIGHT = 8;
    static const int CHUNK_WIDTH = 4;

    RandomLevelSource(Level* level, long seed, int version, bool spawnMobs);
    ~RandomLevelSource();

    // ✅ 修复 —— y 也是 int64_t（匹配 ChunkSource 基类）
bool hasChunk(int64_t x, int64_t y) override;
    LevelChunk* create(int64_t x, int64_t z) override;
    LevelChunk* getChunk(int64_t xOffs, int64_t zOffs) override;
    void prepareHeights(double xOffs, double zOffs, unsigned char* blocks, void* biomes, float* temperatures);
void buildSurfaces(double xOffs, double zOffs, unsigned char* blocks, Biome** biomes);
    void postProcess(ChunkSource* parent, int64_t xt, int64_t zt) override;

    bool tick() override;
    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) override;
    bool shouldSave() override;
    std::string gatherStats() override;

    // 偏移访问器
    double getWorldOffsetX() const { return m_worldOffsetX; }
    double getWorldOffsetY() const { return m_worldOffsetY; }
    double getWorldOffsetZ() const { return m_worldOffsetZ; }
    void setWorldOffset(double ox, double oy, double oz) {
        m_worldOffsetX = ox; m_worldOffsetY = oy; m_worldOffsetZ = oz;
    }

    // 缩放访问器
    float getWorldScaleX() const { return m_worldScaleX; }
    float getWorldScaleY() const { return m_worldScaleY; }
    float getWorldScaleZ() const { return m_worldScaleZ; }

private:
    float* getHeights(float* buffer, double x, int y, double z, int xSize, int ySize, int zSize);
    void calcWaterDepths(ChunkSource* parent, int64_t xt, int64_t zt);

public:
    LargeCaveFeature caveFeature;
    int waterDepths[16+16][16+16];
    int getSeaLevel() const { return customSeaLevel; }

private:
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

    float* buffer;
    float sandBuffer[16 * 16];
    float gravelBuffer[16 * 16];
    float depthBuffer[16 * 16];
    float* pnr;
    float* ar;
    float* br;
    float* sr;
    float* dr;
    float* fi;
    float* fis;

    int customSeaLevel;

    // 偏移 (double 精度)
    double m_worldOffsetX;
    double m_worldOffsetY;
    double m_worldOffsetZ;

    // 缩放 (float)
    float m_worldScaleX;
    float m_worldScaleY;
    float m_worldScaleZ;

    bool m_disableSkygrid;   // 禁用天空网格开关缓存
};

class PerformanceTestChunkSource : public ChunkSource
{
    Level* level;
public:
    PerformanceTestChunkSource(Level* level) : ChunkSource(), level(level) {}
    virtual bool hasChunk(int64_t x, int y) override { return true; }
    virtual LevelChunk* getChunk(int64_t x, int64_t z) override { return create(x, z); }
    virtual LevelChunk* create(int64_t x, int64_t z) override;
    virtual void postProcess(ChunkSource* parent, int64_t x, int64_t z) override {}
    virtual bool tick() override { return false; }
    virtual bool shouldSave() override { return false; }
    virtual std::string gatherStats() override { return "PerformanceTestChunkSource"; }
};

#endif

