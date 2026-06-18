#ifndef NET_MINECRAFT_WORLD_ITEM__TileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__TileItem_H__

#include <string>

#include "Item.h"
#include "ItemInstance.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"

#include "../../network/RakNetInstance.h"
#include "../../network/packet/PlaceBlockPacket.h"

class TileItem : public Item
{
    typedef Item super;

    int tileId;  // item ID = blockId + 256 (例如 endStone: 121+256=377)

public:
    TileItem(int id_)
        : super(id_)
    {
        setStackedByData(true);
        this->tileId = id_ + 256;

        // 🛡️ 修复 ①：构造器 — tiles 索引用 blockId (id_)，不是 itemId (id_+256)
        if (id_ >= 0 && id_ < Tile::NUM_BLOCK_TYPES && Tile::tiles[id_] != nullptr) {
            this->setIcon(Tile::tiles[id_]->getTexture(2));
        }
    }

    int getTileId() {
        return tileId;
    }

    bool useOn(ItemInstance* instance, Player* player, Level* level,
               int x, int y, int z, int face,
               float clickX, float clickY, float clickZ)
    {
        // 🛡️ 修复 ④：immutableWorld 检查 — Tile::tiles 用 blockId
        if (level->adventureSettings.immutableWorld) {
            const Tile* tile = Tile::tiles[tileId - 256];
            if (tileId != ((Tile*)Tile::leaves)->id
                && tile->material != Material::plant) {
                return false;
            }
        }

        if (level->getTile(x, y, z) == Tile::topSnow->id) {
            face = 0;
        } else {
            switch (face) {
                case Facing::DOWN:  y--; break;
                case Facing::UP:    y++; break;
                case Facing::NORTH: z--; break;
                case Facing::SOUTH: z++; break;
                case Facing::WEST:  x--; break;
                case Facing::EAST:  x++; break;
            }
        }

        if (instance->count == 0) return false;

        // 注意：mayPlace 和 setTileAndData 接受的是 tileId (blockId+256)，不需要改
        if (level->mayPlace(tileId, x, y, z, false, face)) {

            // 🛡️ 修复 ⑤：获取 tile 对象 — 用 blockId
            Tile* tile = Tile::tiles[tileId - 256];

            int data = tile->getPlacedOnFaceDataValue(level, x, y, z, face,
                          clickX, clickY, clickZ,
                          getLevelDataForAuxValue(instance->getAuxValue()));

            if (level->setTileAndData(x, y, z, tileId, data)) {

                // 🛡️ 修复 ⑥：setPlacedBy — 用 blockId
                Tile::tiles[tileId - 256]->setPlacedBy(level, x, y, z, player);

                level->playSound(x + 0.5f, y + 0.5f, z + 0.5f,
                    tile->soundType->getStepSound(),
                    (tile->soundType->getVolume() + 1) / 2,
                    tile->soundType->getPitch() * 0.8f);

                instance->count--;
            }
            return true;
        }
        return false;
    }

    // 🛡️ 修复 ②：getDescriptionId(const ItemInstance*) — 用 blockId
    std::string getDescriptionId(const ItemInstance* instance) const {
        return Tile::tiles[tileId - 256]->getDescriptionId();
    }

    // 🛡️ 修复 ③：getDescriptionId() — 用 blockId
    std::string getDescriptionId() const {
        return Tile::tiles[tileId - 256]->getDescriptionId();
    }
};

#endif
