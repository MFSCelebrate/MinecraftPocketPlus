#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__

#include "ChunkSource.h"
#include "storage/ChunkStorage.h"
#include "EmptyLevelChunk.h"
#include "../Level.h"
#include "../LevelConstants.h"
#include "AsyncChunkGenerator.h"
#include "../levelgen/RandomLevelSource.h"
#include <unordered_map>

struct pair_hash {
    std::size_t operator()(const std::pair<int64_t, int64_t>& p) const {
        return std::hash<int64_t>()(p.first) ^ (std::hash<int64_t>()(p.second) << 1);
    }
};

class ChunkCache: public ChunkSource {
    static const int MAX_SAVES = 2;
public:
    ChunkSource* getSource() const { return source; }

    ChunkCache(Level* level_, ChunkStorage* storage_, ChunkSource* source_)
    :   xLast(-999999999), zLast(-999999999), last(NULL),
        level(level_), storage(storage_), source(source_)
    {
        isChunkCache = true;
        emptyChunk = new EmptyLevelChunk(level_, NULL, 0, 0);

        // 若底层源是随机地形生成器，则创建异步生成器，但先不启动
        RandomLevelSource* rls = dynamic_cast<RandomLevelSource*>(source);
        if (rls) {
            asyncGen = new AsyncChunkGenerator(rls, this, level);
        } else {
            asyncGen = nullptr;
        }
    }

    ~ChunkCache() {
        delete source;
        delete emptyChunk;
        if (asyncGen) delete asyncGen;
        for (auto& pair : chunks) {
            if (pair.second) {
                pair.second->deleteBlockData();
                delete pair.second;
            }
        }
    }

    bool fits(int64_t x, int64_t z) { return true; }

    bool hasChunk(int64_t x, int64_t z) {
        auto key = std::make_pair(x, z);
        return chunks.find(key) != chunks.end();
    }

    LevelChunk* create(int64_t x, int64_t z) {
        return getChunk(x, z);
    }

    LevelChunk* getChunk(int64_t x, int64_t z) {
        if (x == xLast && z == zLast && last != NULL) {
            return last;
        }
        auto key = std::make_pair(x, z);
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            xLast = x;
            zLast = z;
            last = it->second;
            return last;
        }

        LevelChunk* newChunk = load(x, z);
        if (newChunk == NULL) {
            if (source == NULL) {
                newChunk = emptyChunk;
            } else {
                // 异步生成器必须已启动才走异步路径
                if (asyncGen && asyncGenStarted) {
                    asyncGen->requestChunk(x, z);
                    newChunk = emptyChunk;
                } else {
                    // 世界创建阶段或未启用异步 → 同步生成
                    newChunk = source->getChunk(x, z);
                }
            }
        }

        if (newChunk != NULL) {
            newChunk->load();
        }

        xLast = x;
        zLast = z;
        last = newChunk;
        return newChunk;
    }

    void putChunk(int64_t x, int64_t z, LevelChunk* chunk) {
        auto key = std::make_pair(x, z);
        if (chunks.find(key) == chunks.end()) {
            chunks[key] = chunk;
            chunk->lightLava();
            pendingInstall.push_back({x, z});
        } else {
            chunk->deleteBlockData();
            delete chunk;
        }
    }

    void processAsyncChunks() {
        if (!asyncGen) return;
        
        asyncGen->processCompletedChunks();

        for (auto& pos : pendingInstall) {
            LevelChunk* chunk = nullptr;
            auto key = std::make_pair(pos.first, pos.second);
            auto it = chunks.find(key);
            if (it != chunks.end()) chunk = it->second;
            if (!chunk) continue;

            int64_t blockX = pos.first * 16;
            int64_t blockZ = pos.second * 16;
            for (int cx = 0; cx < 16; cx++) {
                for (int cz = 0; cz < 16; cz++) {
                    int height = level->getHeightmap(blockX + cx, blockZ + cz);
                    for (int cy = height; cy >= 0; cy--) {
                        level->updateLight(LightLayer::Sky, blockX + cx, cy, blockZ + cz,
                                           blockX + cx, cy, blockZ + cz);
                        level->updateLight(LightLayer::Block, blockX + cx - 1, cy, blockZ + cz - 1,
                                           blockX + cx + 1, cy, blockZ + cz + 1);
                    }
                }
            }

            int64_t x = pos.first, z = pos.second;
            if (!chunk->terrainPopulated &&
                hasChunk(x+1, z+1) && hasChunk(x, z+1) && hasChunk(x+1, z))
                postProcess(this, x, z);
            if (hasChunk(x-1, z) && !getChunk(x-1, z)->terrainPopulated &&
                hasChunk(x-1, z+1) && hasChunk(x, z+1) && hasChunk(x-1, z))
                postProcess(this, x-1, z);
            if (hasChunk(x, z-1) && !getChunk(x, z-1)->terrainPopulated &&
                hasChunk(x+1, z-1) && hasChunk(x, z-1) && hasChunk(x+1, z))
                postProcess(this, x, z-1);
            if (hasChunk(x-1, z-1) && !getChunk(x-1, z-1)->terrainPopulated &&
                hasChunk(x-1, z-1) && hasChunk(x, z-1) && hasChunk(x-1, z))
                postProcess(this, x-1, z-1);
        }
        pendingInstall.clear();
    }

    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
        return source->getMobsAt(mobCategory, x, y, z);
    }

    void postProcess(ChunkSource* parent, int64_t x, int64_t z) {
        static int depth = 0;
        if (depth > 20) return;
        depth++;
        if (!fits(x, z)) { depth--; return; }
        LevelChunk* chunk = getChunk(x, z);
        if (!chunk->terrainPopulated) {
            chunk->terrainPopulated = true;
            if (source != NULL) {
                source->postProcess(parent, x, z);
            }
            chunk->clearUpdateMap();
        }
        depth--;
    }

    bool tick() {
        if (asyncGen && !asyncGenStarted) {
            asyncGen->start();
            asyncGenStarted = true;
        }
        if (asyncGen) processAsyncChunks();
        if (storage != NULL) storage->tick();
        return source->tick();
    }

    void getLoadedChunks(std::vector<LevelChunk*>& out) const {
        out.clear();
        for (const auto& pair : chunks) {
            if (pair.second && pair.second != emptyChunk) {
                out.push_back(pair.second);
            }
        }
    }

    bool shouldSave() { return true; }
    std::string gatherStats() { return "ChunkCache: async dynamic"; }

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
    LevelChunk* load(int64_t x, int64_t z) {
        if (storage == NULL) return emptyChunk;
        LevelChunk* levelChunk = storage->load(level, x, z);
        if (levelChunk != NULL) {
            levelChunk->lastSaveTime = level->getTime();
        }
        return levelChunk;
    }

public:
    int64_t xLast;
    int64_t zLast;
private:
    LevelChunk* emptyChunk;
    ChunkSource* source;
    ChunkStorage* storage;
    std::unordered_map<std::pair<int64_t, int64_t>, LevelChunk*, pair_hash> chunks;
    Level* level;
    LevelChunk* last;

    AsyncChunkGenerator* asyncGen = nullptr;
    bool asyncGenStarted = false;
    std::vector<std::pair<int64_t, int64_t>> pendingInstall;
};

#endif
