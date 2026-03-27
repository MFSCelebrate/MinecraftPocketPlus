#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__

#include "ChunkSource.h"
#include "storage/ChunkStorage.h"
#include "EmptyLevelChunk.h"
#include "../Level.h"
#include "../LevelConstants.h"
#include <unordered_map>

struct pair_hash {
    std::size_t operator()(const std::pair<int64_t, int64_t>& p) const {
        return std::hash<int64_t>()(p.first) ^ (std::hash<int64_t>()(p.second) << 1);
    }
};

class ChunkCache: public ChunkSource {
    static const int MAX_SAVES = 2;
public:
    ChunkCache(Level* level_, ChunkStorage* storage_, ChunkSource* source_)
    :   xLast(-999999999),
        zLast(-999999999),
        last(NULL),
        level(level_),
        storage(storage_),
        source(source_)
    {
        isChunkCache = true;
        emptyChunk = new EmptyLevelChunk(level_, NULL, 0, 0);
    }

    ~ChunkCache() {
        delete source;
        delete emptyChunk;
        for (auto& pair : chunks) {
            if (pair.second) {
                pair.second->deleteBlockData();
                delete pair.second;
            }
        }
    }

    bool fits(int x, int z) {
        return true;   // 不再限制范围
    }

    bool hasChunk(int x, int z) {
        auto key = std::make_pair((int64_t)x, (int64_t)z);
        return chunks.find(key) != chunks.end();
    }

    LevelChunk* create(int x, int z) {
        return getChunk(x, z);
    }

    LevelChunk* getChunk(int x, int z) {
        if (x == xLast && z == zLast && last != NULL) {
            return last;
        }
        auto key = std::make_pair((int64_t)x, (int64_t)z);
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            xLast = x;
            zLast = z;
            last = it->second;
            return last;
        }

        // 加载或生成新区块
        LevelChunk* newChunk = load(x, z);
        bool updateLights = false;
        if (newChunk == NULL) {
            if (source == NULL) {
                newChunk = emptyChunk;
            } else {
                newChunk = source->getChunk(x, z);
            }
        } else {
            updateLights = true;
        }
        chunks[key] = newChunk;
        newChunk->lightLava();

        if (updateLights) {
            for (int cx = 0; cx < 16; cx++) {
                for (int cz = 0; cz < 16; cz++) {
                    int height = level->getHeightmap(cx + x * 16, cz + z * 16);
                    for (int cy = height; cy >= 0; cy--) {
                        level->updateLight(LightLayer::Sky, cx + x * 16, cy, cz + z * 16, cx + x * 16, cy, cz + z * 16);
                        level->updateLight(LightLayer::Block, cx + x * 16 - 1, cy, cz + z * 16 - 1, cx + x * 16 + 1, cy, cz + z * 16 + 1);
                    }
                }
            }
        }

        if (newChunk != NULL) {
            newChunk->load();
        }

        // 后处理相邻区块
       // if (!newChunk->terrainPopulated && hasChunk(x + 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x + 1, z))
        //    postProcess(this, x, z);
     //   if (hasChunk(x - 1, z) && !getChunk(x - 1, z)->terrainPopulated && hasChunk(x - 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x - 1, z))
        //    postProcess(this, x - 1, z);
    //    if (hasChunk(x, z - 1) && !getChunk(x, z - 1)->terrainPopulated && hasChunk(x + 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x + 1, z))
       //     postProcess(this, x, z - 1);
    //    if (hasChunk(x - 1, z - 1) && !getChunk(x - 1, z - 1)->terrainPopulated && hasChunk(x - 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x - 1, z))
          //  postProcess(this, x - 1, z - 1);

        xLast = x;
        zLast = z;
        last = newChunk;
        return newChunk;
    }

    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
        return source->getMobsAt(mobCategory, x, y, z);
    }

    void postProcess(ChunkSource* parent, int x, int z) {
        static int depth = 0;
        if (depth > 20) return;
        depth++;
        if (!fits(x, z)) return;
        LevelChunk* chunk = getChunk(x, z);
        if (!chunk->terrainPopulated) {
            chunk->terrainPopulated = true;
            if (source != NULL) {
                source->postProcess(parent, x, z);
                chunk->clearUpdateMap();
            }
        }
        depth--;
    }

    bool tick() {
        if (storage != NULL) storage->tick();
        return source->tick();
    }

    bool shouldSave() {
        return true;
    }

    std::string gatherStats() {
        return "ChunkCache: dynamic";
    }

    void saveAll(bool onlyUnsaved) {
        if (storage != NULL) {
            std::vector<LevelChunk*> chunksToSave;
            for (auto& pair : chunks) {
                LevelChunk* chunk = pair.second;
                if (!onlyUnsaved || chunk->shouldSave(false))
                    chunksToSave.push_back(chunk);
            }
            storage->saveAll(level, chunksToSave);
        }
    }

private:
    LevelChunk* load(int x, int z) {
        if (storage == NULL) return emptyChunk;
        LevelChunk* levelChunk = storage->load(level, x, z);
        if (levelChunk != NULL) {
            levelChunk->lastSaveTime = level->getTime();
        }
        return levelChunk;
    }

    void saveEntities(LevelChunk* levelChunk) {
        if (storage == NULL) return;
        storage->saveEntities(level, levelChunk);
    }

    void save(LevelChunk* levelChunk) {
        if (storage == NULL) return;
        levelChunk->lastSaveTime = level->getTime();
        storage->save(level, levelChunk);
    }

public:
    int xLast;
    int zLast;
private:
    LevelChunk* emptyChunk;
    ChunkSource* source;
    ChunkStorage* storage;
    std::unordered_map<std::pair<int64_t, int64_t>, LevelChunk*, pair_hash> chunks;
    Level* level;

    LevelChunk* last;
};

#endif
