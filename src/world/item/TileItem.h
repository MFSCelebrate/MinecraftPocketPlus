#ifndef NET_MINECRAFT_WORLD_ITEM__TileItem_H__
#define NET_MINECRAFT_WORLD_ITEM__TileItem_H__

//package net.minecraft.world.item;

#include <string>

#include "Item.h"
#include "ItemInstance.h"
#include "../entity/player/Player.h"
#include "../level/Level.h"
#include "../level/tile/Tile.h"

#include "../../network/RakNetInstance.h"
#include "../../network/packet/PlaceBlockPacket.h"

class TileItem: public Item
{
	typedef Item super;

	int tileId;
public:
    TileItem(int id_) : super(id_) {
    setStackedByData(true);
    this->tileId = id_ + 256;
    int blockId = this->tileId - 256;  // 正确的 tiles 索引
    if (blockId >= 0 && blockId < Tile::NUM_BLOCK_TYPES && Tile::tiles[blockId] != nullptr) {
        this->setIcon(Tile::tiles[blockId]->getTexture(2));
    }
	}

    int getTileId() {
        return tileId;
    }

    bool useOn(ItemInstance* instance, Player* player, Level* level, int x, int y, int z, int face, float clickX, float clickY, float clickZ) {
		if (level->adventureSettings.immutableWorld) {
			const Tile* tile = Tile::tiles[tileId - 256];  // ← 🛡️
			if (tileId != ((Tile*)Tile::leaves)->id
				&& tile->material != Material::plant) {
					return false;
			}
		}

		if (level->getTile(x, y, z) == Tile::topSnow->id) {
            face = 0;
        } else {
			switch (face) {
				case Facing::DOWN : y--; break;
				case Facing::UP   : y++; break;
				case Facing::NORTH: z--; break;
				case Facing::SOUTH: z++; break;
				case Facing::WEST : x--; break;
				case Facing::EAST : x++; break;
			}
        }

        if (instance->count == 0) return false;

        if (level->mayPlace(tileId, x, y, z, false, face)) {
            Tile* tile = Tile::tiles[tileId];
			int data = tile->getPlacedOnFaceDataValue(level, x, y, z, face, clickX, clickY, clickZ, getLevelDataForAuxValue(instance->getAuxValue()));
            if (level->setTileAndData(x, y, z, tileId, data)) {
                Tile::tiles[tileId]->setPlacedBy(level, x, y, z, player);
                level->playSound(x + 0.5f, y + 0.5f, z + 0.5f, tile->soundType->getStepSound(), (tile->soundType->getVolume() + 1) / 2, tile->soundType->getPitch() * 0.8f);

/*
				PlaceBlockPacket packet(player->entityId, x, y, z, face, tileId, instance->getAuxValue());
				//LOGI("Place block at @ %d, %d, %d\n", x, y, z);
				level->raknetInstance->send(packet);
*/
                instance->count--;
            }
            return true;
        }
        return false;
    }

    // 🛡️ 修复 getDescriptionId() const
    std::string getDescriptionId(const ItemInstance* instance) const {
        // 修改前：return Tile::tiles[tileId]->getDescriptionId();
        return Tile::tiles[tileId - 256]->getDescriptionId();
    }

    // 🛡️ 修复 getDescriptionId()
    std::string getDescriptionId() const {
        // 修改前：return Tile::tiles[tileId]->getDescriptionId();
        return Tile::tiles[tileId - 256]->getDescriptionId();
    }
};

#endif /*NET_MINECRAFT_WORLD_ITEM__TileItem_H__*/
