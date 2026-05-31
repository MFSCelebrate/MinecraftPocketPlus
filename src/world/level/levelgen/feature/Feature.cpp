#include "Feature.h"

Feature::Feature( bool doUpdate /*= false*/ )
:	doUpdate(doUpdate)
{
}

// ✅ 修改后
void Feature::placeBlock(Level* level, int64_t x, int y, int64_t z, int tile) {
    if (doUpdate)
        level->setTile(x, y, z, tile);
    else
        level->setTileNoUpdate(x, y, z, tile);
}

void Feature::placeBlock(Level* level, int64_t x, int y, int64_t z, int tile, int data) {
    if (doUpdate)
        level->setTileAndData(x, y, z, tile, data);
    else
        level->setTileAndDataNoUpdate(x, y, z, tile, data);
}
