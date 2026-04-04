#include "Region.h"
#include "chunk/LevelChunk.h"
#include "material/Material.h"
#include "tile/Tile.h"
#include "Level.h"

Region::Region(Level* level, int64_t x1, int y1, int64_t z1, int64_t x2, int y2, int64_t z2) {
    this->level = level;

    xc1 = x1 >> 4;
    zc1 = z1 >> 4;
    int64_t xc2 = x2 >> 4;
    int64_t zc2 = z2 >> 4;

    size_x = (int)(xc2 - xc1 + 1);
    size_z = (int)(zc2 - zc1 + 1);
    chunks = new LevelChunk**[size_x];
    for (int i = 0; i < size_x; ++i)
        chunks[i] = new LevelChunk*[size_z];

    for (int64_t xc = xc1; xc <= xc2; xc++) {
        for (int64_t zc = zc1; zc <= zc2; zc++) {
            chunks[(int)(xc - xc1)][(int)(zc - zc1)] = level->getChunk(xc, zc);
        }
    }
}

Region::~Region() {
    for (int i = 0; i < size_x; ++i)
        delete[] chunks[i];
    delete[] chunks;
}

int Region::getTile(int64_t x, int y, int64_t z) {
    if (y < 0) return 0;
    if (y >= Level::DEPTH) return 0;

    int64_t xc = (x >> 4) - xc1;
    int64_t zc = (z >> 4) - zc1;

    if (xc < 0 || xc >= size_x || zc < 0 || zc >= size_z) {
        return 0;
    }

    LevelChunk* lc = chunks[(int)xc][(int)zc];
    if (lc == NULL) return 0;

    return lc->getTile((int)(x & 15), y, (int)(z & 15));
}

bool Region::isEmptyTile(int64_t x, int y, int64_t z) {
    return Tile::tiles[getTile(x, y, z)] == NULL;
}

float Region::getBrightness(int64_t x, int y, int64_t z) {
    return level->dimension->brightnessRamp[getRawBrightness(x, y, z)];
}

int Region::getRawBrightness(int64_t x, int y, int64_t z) {
    return getRawBrightness(x, y, z, true);
}

int Region::getRawBrightness(int64_t x, int y, int64_t z, bool propagate) {
    // 注意：MAX_LEVEL_SIZE 是 int，这里只用于粗略边界检查，大坐标会超出，但此函数通常只在近距离调用
    if (x < -Level::MAX_LEVEL_SIZE || z < -Level::MAX_LEVEL_SIZE || x >= Level::MAX_LEVEL_SIZE || z > Level::MAX_LEVEL_SIZE) {
        return Level::MAX_BRIGHTNESS;
    }

    if (propagate) {
        int id = getTile(x, y, z);
        if (id == Tile::stoneSlabHalf->id || id == Tile::farmland->id) {
            int br = getRawBrightness(x, y + 1, z, false);
            int br1 = getRawBrightness(x + 1, y, z, false);
            int br2 = getRawBrightness(x - 1, y, z, false);
            int br3 = getRawBrightness(x, y, z + 1, false);
            int br4 = getRawBrightness(x, y, z - 1, false);
            if (br1 > br) br = br1;
            if (br2 > br) br = br2;
            if (br3 > br) br = br3;
            if (br4 > br) br = br4;
            return br;
        }
    }

    if (y < 0) return 0;
    if (y >= Level::DEPTH) {
        int br = Level::MAX_BRIGHTNESS - level->skyDarken;
        if (br < 0) br = 0;
        return br;
    }

    int64_t xc = (x >> 4) - xc1;
    int64_t zc = (z >> 4) - zc1;

    return chunks[(int)xc][(int)zc]->getRawBrightness((int)(x & 15), y, (int)(z & 15), level->skyDarken);
}

int Region::getData(int64_t x, int y, int64_t z) {
    if (y < 0) return 0;
    if (y >= Level::DEPTH) return 0;
    int64_t xc = (x >> 4) - xc1;
    int64_t zc = (z >> 4) - zc1;

    return chunks[(int)xc][(int)zc]->getData((int)(x & 15), y, (int)(z & 15));
}

const Material* Region::getMaterial(int64_t x, int y, int64_t z) {
    int t = getTile(x, y, z);
    if (t == 0) return Material::air;
    return Tile::tiles[t]->material;
}

bool Region::isSolidBlockingTile(int64_t x, int y, int64_t z) {
    Tile* tile = Tile::tiles[getTile(x, y, z)];
    if (tile == NULL) return false;
    return tile->material->isSolidBlocking() && tile->isCubeShaped();
}

bool Region::isSolidRenderTile(int64_t x, int y, int64_t z) {
    Tile* tile = Tile::tiles[getTile(x, y, z)];
    if (tile == NULL) return false;
    return tile->isSolidRender();
}

Biome* Region::getBiome(int64_t x, int64_t z) {
    return level->getBiome(x, z);
}
