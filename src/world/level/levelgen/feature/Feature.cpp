#include "Feature.h"
#include <cstdint>

Feature::Feature( bool doUpdate /*= false*/ )
:	doUpdate(doUpdate)
{
}

// ✅ 新代码 —— x/z 升级为 int64_t
void Feature::placeBlock( Level* level, int64_t x, int y, int64_t z, int tile ) {
    if (doUpdate)
        level->setTile(x, y, z, tile);
    else
        level->setTileNoUpdate(x, y, z, tile);
}

void Feature::placeBlock( Level* level, int64_t x, int y, int64_t z, int tile, int data ) {
    if (doUpdate)
        level->setTileAndData(x, y, z, tile, data);
    else
        level->setTileAndDataNoUpdate(x, y, z, tile, data);
}
