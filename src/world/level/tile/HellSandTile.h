#ifndef HELLSANDTILE_H__
#define HELLSANDTILE_H__

#include "Tile.h"

class HellSandTile : public Tile {
    typedef Tile super;
public:
    HellSandTile(int id, int tex) : Tile(id, tex, Material::sand) {
        friction = 0.8f; // 比普通方块略滑
    }

    // 后续可在此添加 entityInside 下沉逻辑
};

#endif
