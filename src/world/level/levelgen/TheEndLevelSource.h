#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__TheEndLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__TheEndLevelSource_H__

#include "../chunk/ChunkSource.h"
#include "synth/PerlinNoise.h"
#include <map>

class Level;
class LevelChunk;

class TheEndLevelSource : public ChunkSource {
public:
    TheEndLevelSource(Level* level, long seed);
    ~TheEndLevelSource();

    virtual bool hasChunk(int64_t x, int64_t z) override;
    virtual LevelChunk* create(int64_t x, int64_t z) override;
    virtual LevelChunk* getChunk(int64_t x, int64_t z) override;
    virtual void postProcess(ChunkSource* parent, int64_t xt, int64_t zt) override;
    virtual bool tick() override;
    virtual Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) override;
    virtual bool shouldSave() override;
    virtual std::string gatherStats() override;

private:
    void prepareHeights(int64_t chunkX, int64_t chunkZ, unsigned char* blocks);
    void generateDensityCells(int64_t chunkX, int64_t chunkZ, double* density);
    double getIslandHeightValue(int64_t chunkX, int64_t chunkZ, int xC, int zC);

    std::map<int64_t, LevelChunk*> chunkMap;
    Level* level;
    Random random;
    PerlinNoise pNoise1;
    PerlinNoise pNoise2;
    PerlinNoise pNoise3;
    PerlinNoise sNoise1;

    double* densityBuffer;

    static const int DENSITY_X = 3;
    static const int DENSITY_Y = 33;
    static const int DENSITY_Z = 3;

    double m_worldOffsetX = 0.0;
    double m_worldOffsetY = 0.0;
    double m_worldOffsetZ = 0.0;
    double m_worldScaleX  = 1.0;
    double m_worldScaleY  = 1.0;
    double m_worldScaleZ  = 1.0;

    bool enableCircles = false;
};

#endif
