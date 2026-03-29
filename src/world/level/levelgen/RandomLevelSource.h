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
// 在 public 区域添加（不标记 const）
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

    bool hasChunk(int x, int y);
    LevelChunk* create(int x, int z);
    LevelChunk* getChunk(int xOffs, int zOffs);

    void prepareHeights(int xOffs, int zOffs, unsigned char* blocks, /*Biome*/void* biomes, float* temperatures);
    void buildSurfaces(int xOffs, int zOffs, unsigned char* blocks, Biome** biomes);
    void postProcess(ChunkSource* parent, int xt, int zt);

    bool tick();

    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z);

    bool shouldSave();
    std::string gatherStats();

    // 新增：获取地形偏移量
    int getOffsetX() const { return offsetX; }
    int getOffsetZ() const { return offsetZ; }

private:
    float* getHeights(float* buffer, int x, int y, int z, int xSize, int ySize, int zSize);
    void calcWaterDepths(ChunkSource* parent, int xt, int zt);

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

    int customSeaLevel;     // 用户自定义海平面高度（0-127），默认63

    // 地形偏移量（区块为单位）
    int offsetX, offsetZ;   // 注意：此处为唯一声明，原有 offsetX, OffsetZ 已被替换
};

class PerformanceTestChunkSource : public ChunkSource
{
    Level* level;
public:
    PerformanceTestChunkSource(Level* level)
    :   ChunkSource(),
        level(level)
    {}

    virtual bool hasChunk(int x, int y) { return true; };
    virtual LevelChunk* getChunk(int x, int z) { return create(x, z); };
    virtual LevelChunk* create(int x, int z);
    virtual void postProcess(ChunkSource* parent, int x, int z) {};
    virtual bool tick() { return false; };
    virtual bool shouldSave() { return false; };
    virtual std::string gatherStats() { return "PerformanceTestChunkSource"; };
};

#endif
