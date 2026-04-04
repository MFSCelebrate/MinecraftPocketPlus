#ifndef NET_MINECRAFT_WORLD_LEVEL__Region_H__
#define NET_MINECRAFT_WORLD_LEVEL__Region_H__

#include "LevelSource.h"
#include <cstdint>

class Level;
class Material;
class LevelChunk;

class Region: public LevelSource
{
public:
    Region(Level* level, int64_t x1, int y1, int64_t z1, int64_t x2, int y2, int64_t z2);
    ~Region();

    // 实现 LevelSource 的纯虚函数（参数改为 int64_t）
    virtual int getTile(int64_t x, int y, int64_t z) override;
    virtual bool isEmptyTile(int64_t x, int y, int64_t z) override;
    virtual float getBrightness(int64_t x, int y, int64_t z) override;
    virtual int getData(int64_t x, int y, int64_t z) override;
    virtual const Material* getMaterial(int64_t x, int y, int64_t z) override;
    virtual bool isSolidRenderTile(int64_t x, int y, int64_t z) override;
    virtual bool isSolidBlockingTile(int64_t x, int y, int64_t z) override;
    virtual Biome* getBiome(int64_t x, int64_t z) override;

    // 辅助函数（内部使用）
    int getRawBrightness(int64_t x, int y, int64_t z);
    int getRawBrightness(int64_t x, int y, int64_t z, bool propagate);

private:
    int64_t xc1, zc1;           // 区块坐标（单位：区块）
    LevelChunk*** chunks;
    Level* level;
    int size_x;                 // 区块范围大小（int 足够，因为区域不会太大）
    int size_z;
};

#endif
