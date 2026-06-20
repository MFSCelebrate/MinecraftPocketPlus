// 文件：src/world/level/levelgen/TheEndLevelSource.h

#ifndef NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__TheEndLevelSource_H__
#define NET_MINECRAFT_WORLD_LEVEL_LEVELGEN__TheEndLevelSource_H__

#include "../chunk/ChunkSource.h"
#include "synth/PerlinNoise.h"
#include <map>

class Level;
class LevelChunk;

class TheEndLevelSource : public ChunkSource
{
public:
    TheEndLevelSource(Level* level, long seed);
    ~TheEndLevelSource();

    virtual bool       hasChunk(int64_t x, int64_t z) override;
    virtual LevelChunk* create(int64_t x, int64_t z) override;
    virtual LevelChunk* getChunk(int64_t x, int64_t z) override;
    virtual void       postProcess(ChunkSource* parent, int64_t xt, int64_t zt) override;
    virtual bool       tick() override;
    virtual bool       shouldSave() override;
    virtual std::string gatherStats() override;
    virtual Biome::MobList getMobsAt(const MobCategory& mc, int x, int y, int z) override;

    // 🛡️ 调试面板访问器
    double getWorldOffsetX() const { return m_worldOffsetX; }
    double getWorldOffsetY() const { return m_worldOffsetY; }
    double getWorldOffsetZ() const { return m_worldOffsetZ; }
    double getWorldScaleX()  const { return m_worldScaleX; }
    double getWorldScaleY()  const { return m_worldScaleY; }
    double getWorldScaleZ()  const { return m_worldScaleZ; }

    double getPNoise1Value(double x, double y, double z) const;
    double getPNoise2Value(double x, double y, double z) const;
    double getPNoise3Value(double x, double y, double z) const;
    double getSNoise1Value(double x, double z) const;

    double sampleDensityAt(double worldX, double worldY, double worldZ);
    double debugIslandHeightValue(int64_t chunkX, int64_t chunkZ) {
        return getIslandHeightValue(chunkX, chunkZ, 1, 1);
    }

private:
    void prepareHeights(int64_t chunkX, int64_t chunkZ, unsigned char* blocks);
    void generateDensityCells(int64_t chunkX, int64_t chunkZ, double* density);
    double getIslandHeightValue(int64_t chunkX, int64_t chunkZ, int xC, int zC);
    void generateEndSpikes();

    std::map<int64_t, LevelChunk*> chunkMap;
    Level*    level;
    Random    random;

    // 🛡️ 噪声成员改为指针 — 运行时选择 double 或 float
    PerlinNoise*        pNoise1 = nullptr;   // PerlinNoiseT<double>
    PerlinNoise*        pNoise2 = nullptr;
    PerlinNoise*        pNoise3 = nullptr;
    PerlinNoise*        sNoise1 = nullptr;

    PerlinNoiseT<float>* pNoise1_f = nullptr; // float 备用
    PerlinNoiseT<float>* pNoise2_f = nullptr;
    PerlinNoiseT<float>* pNoise3_f = nullptr;
    PerlinNoiseT<float>* sNoise1_f = nullptr;

    bool m_useDoubleNoise = true;   // 🛡️ 标记当前使用 double 还是 float

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

    bool m_endCircles     = false;
    bool m_spikesGenerated = false;
};

#endif
