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
    double getLPerlinNoise1(float x, float z) { return lperlinNoise1.getValue(x, z); }
    double getLPerlinNoise2(float x, float z) { return lperlinNoise2.getValue(x, z); }
    double getPerlinNoise1(float x, float z) { return perlinNoise1.getValue(x, z); }
    double getPerlinNoise2(float x, float z) { return perlinNoise2.getValue(x, z); }
    double getPerlinNoise3(float x, float z) { return perlinNoise3.getValue(x, z); }
    double getScaleNoise(float x, float z) { return scaleNoise.getValue(x, z); }
    double getDepthNoise(float x, float z) { return depthNoise.getValue(x, z); }
    double getForestNoise(float x, float z) { return forestNoise.getValue(x, z); }

    static const int CHUNK_HEIGHT = 8;
    static const int CHUNK_WIDTH = 4;

    RandomLevelSource(Level* level, long seed, int version, bool spawnMobs);
    ~RandomLevelSource();

    bool hasChunk(int64_t x, int64_t y) override;
    LevelChunk* create(int64_t x, int64_t z) override;
    LevelChunk* getChunk(int64_t xOffs, int64_t zOffs) override;
    void prepareHeights(int64_t xOffs, int64_t zOffs, unsigned char* blocks, void* biomes, float* temperatures);
    void buildSurfaces(int64_t xOffs, int64_t zOffs, unsigned char* blocks, Biome** biomes);
    void postProcess(ChunkSource* parent, int64_t xt, int64_t zt) override;

    bool tick() override;
    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) override;
    bool shouldSave() override;
    std::string gatherStats() override;

    // 精确世界偏移（单位：世界方块）
    void setWorldOffset(int64_t ox, int64_t oz) { m_worldOffsetX = ox; m_worldOffsetZ = oz; }
    int64_t getWorldOffsetX() const { return m_worldOffsetX; }
    int64_t getWorldOffsetZ() const { return m_worldOffsetZ; }

private:
    float* getHeights(float* buffer, int64_t x, int y, int64_t z, int xSize, int ySize, int zSize);
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

    // 新的精确偏移（单位：世界方块），废弃了旧的 offsetX/offsetZ
    int64_t m_worldOffsetX, m_worldOffsetZ;
};

class PerformanceTestChunkSource : public ChunkSource
{
    Level* level;
public:
    PerformanceTestChunkSource(Level* level) : ChunkSource(), level(level) {}
    virtual bool hasChunk(int64_t x, int64_t y) override { return true; }
    virtual LevelChunk* getChunk(int64_t x, int64_t z) override { return create(x, z); }
    virtual LevelChunk* create(int64_t x, int64_t z) override;
    virtual void postProcess(ChunkSource* parent, int64_t x, int64_t z) override {}
    virtual bool tick() override { return false; }
    virtual bool shouldSave() override { return false; }
    virtual std::string gatherStats() override { return "PerformanceTestChunkSource"; }
};

#endif
