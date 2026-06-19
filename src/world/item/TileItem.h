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

// 文件：src/world/item/TileItem.h

class TileItem : public Item {
    typedef Item super;
    int tileId;

public:
    TileItem(int id_) : super(id_) {
        setStackedByData(true);
        this->tileId = id_ + 256;
        // 🛡️ 安全获取 icon
        if (id_ >= 0 && id_ < Tile::NUM_BLOCK_TYPES && Tile::tiles[id_] != nullptr) {
            this->setIcon(Tile::tiles[id_]->getTexture(2));
        }
    }

    int getTileId() { return tileId; }

    // 🛡️ 安全的 tile 访问辅助函数
    Tile* getTile() const {
        int blockId = tileId - 256;
        if (blockId < 0 || blockId >= Tile::NUM_BLOCK_TYPES) return nullptr;
        return Tile::tiles[blockId];
    }

    // 🛡️ 修复 getDescriptionId — 加 null 防御
    std::string getDescriptionId(const ItemInstance* instance) const {
        Tile* tile = getTile();
        if (tile != nullptr) {
            return tile->getDescriptionId();
        }
        return "tile.unknown.name";
    }

    // 🛡️ 修复 getDescriptionId — 加 null 防御
    std::string getDescriptionId() const {
        Tile* tile = getTile();
        if (tile != nullptr) {
            return tile->getDescriptionId();
        }
        return "tile.unknown.name";
    }

    // 🛡️ 修复 useOn — 加 null 防御
    bool useOn(ItemInstance* instance, Player* player, Level* level,
               int x, int y, int z, int face,
               float clickX, float clickY, float clickZ) {
        
        Tile* tile = getTile();  // 🛡️ 安全检查
        if (tile == nullptr) return false;

        if (level->adventureSettings.immutableWorld) {
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

        if (level->mayPlace(tileId, x, y, z, false, face)) {
            int data = tile->getPlacedOnFaceDataValue(level, x, y, z, face,
                          clickX, clickY, clickZ,
                          getLevelDataForAuxValue(instance->getAuxValue()));

            if (level->setTileAndData(x, y, z, tileId, data)) {
                tile->setPlacedBy(level, x, y, z, player);
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
};

#endif
