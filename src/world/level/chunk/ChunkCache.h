#ifndef NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__
#define NET_MINECRAFT_WORLD_LEVEL_CHUNK__ChunkCache_H__

#include "ChunkSource.h"
#include "storage/ChunkStorage.h"
#include "EmptyLevelChunk.h"
#include "../Level.h"
#include "../LevelConstants.h"
#include <unordered_map>
#include <set>

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
        genBlocks = new unsigned char[LevelChunk::ChunkBlockCount];
        memset(genBlocks, 0, LevelChunk::ChunkBlockCount);
    }

    ~ChunkCache() {
        delete source;
        delete emptyChunk;
        delete[] genBlocks;
        for (auto& pair : chunks) {
            if (pair.second) {
                pair.second->deleteBlockData();
                delete pair.second;
            }
        }
    }

    bool fits(int64_t x, int64_t z) { return true; }

    bool hasChunk(int64_t x, int64_t z) {
        return chunks.find(std::make_pair(x, z)) != chunks.end();
    }

    LevelChunk* create(int64_t x, int64_t z) {
        return getChunk(x, z);
    }

    LevelChunk* getChunk(int64_t x, int64_t z) {
        if (x == xLast && z == zLast && last != NULL)
            return last;

        auto key = std::make_pair(x, z);
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            if (deferredLighting.find(key) != deferredLighting.end()) {
                doFullLighting(it->second, x, z);
                deferredLighting.erase(key);
            }
            if (deferredPostProcess.find(key) != deferredPostProcess.end()) {
                doPostProcess(x, z);
                deferredPostProcess.erase(key);
            }
            xLast = x; zLast = z; last = it->second;
            return last;
        }

        LevelChunk* newChunk = load(x, z);
        if (newChunk == NULL) {
            if (source == NULL) {
                newChunk = emptyChunk;
            } else {
                newChunk = source->getChunk(x, z);
            }
        }

        if (newChunk == NULL) {
            newChunk = emptyChunk;
        } else {
            chunks[key] = newChunk;
            newChunk->lightLava();
            doFullLighting(newChunk, x, z);
            doPostProcess(x, z);
            newChunk->load();
        }

        xLast = x; zLast = z; last = newChunk;
        return newChunk;
    }

    void preloadChunk(int64_t x, int64_t z) {
        auto key = std::make_pair(x, z);
        if (chunks.find(key) != chunks.end()) return;

        LevelChunk* newChunk = load(x, z);
        if (newChunk == NULL && source != NULL) {
            newChunk = source->getChunk(x, z);
            if (newChunk) {
                chunks[key] = newChunk;
                newChunk->lightLava();
                deferredLighting.insert(key);
                deferredPostProcess.insert(key);
                newChunk->load();
            }
        } else if (newChunk) {
            chunks[key] = newChunk;
        }
    }

    bool tick() {
        // 1. 延迟光照：每帧最多 2 个
        int lightCount = 0;
        auto lit = deferredLighting.begin();
        while (lit != deferredLighting.end() && lightCount < 2) {
            auto key = *lit;
            auto it = chunks.find(key);
            if (it != chunks.end() && it->second != emptyChunk) {
                doFullLighting(it->second, key.first, key.second);
                lit = deferredLighting.erase(lit);
                lightCount++;
            } else {
                ++lit;
            }
        }

        // 2. 延迟后处理：每帧最多 1 个
        auto ppit = deferredPostProcess.begin();
        while (ppit != deferredPostProcess.end()) {
            auto key = *ppit;
            doPostProcess(key.first, key.second);
            ppit = deferredPostProcess.erase(ppit);
            break; // 每帧只做 1 个
        }

        // 3. 预生成玩家周围未加载区块（每帧最多 2 个）
        int pregen = 0;
        for (size_t pi = 0; pi < level->players.size() && pregen < 2; ++pi) {
            Player* player = level->players[pi];
            int cx = Mth::floor(player->x / 16.0);
            int cz = Mth::floor(player->z / 16.0);
            for (int r = 1; r <= 4 && pregen < 2; ++r) {
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = cx + dx, nz = cz + r;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= 2) break; }
                    nz = cz - r;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= 2) break; }
                }
                for (int dz = -r+1; dz <= r-1; ++dz) {
                    int nx = cx + r, nz = cz + dz;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= 2) break; }
                    nx = cx - r;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= 2) break; }
                }
            }
        }

        if (storage != NULL) storage->tick();
        return source->tick();
    }

    void getLoadedChunks(std::vector<LevelChunk*>& out) const {
        out.clear();
        for (const auto& pair : chunks) {
            if (pair.second && pair.second != emptyChunk) out.push_back(pair.second);
        }
    }

    bool shouldSave() { return true; }
    std::string gatherStats() { return "ChunkCache: optimized sync pregen"; }

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

    Biome::MobList getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
        return source->getMobsAt(mobCategory, x, y, z);
    }

void postProcess(ChunkSource* parent, int64_t x, int64_t z) override {
        if (!fits(x, z)) return;
        LevelChunk* chunk = getChunk(x, z);
        if (!chunk->terrainPopulated) {
            chunk->terrainPopulated = true;
            if (source != NULL) {
                if (hasChunk(x+1, z+1) && hasChunk(x, z+1) && hasChunk(x+1, z))
                    source->postProcess(parent, x, z);
                else
                    chunk->terrainPopulated = false;
            }
            chunk->clearUpdateMap();
        }
}

private:
    void doFullLighting(LevelChunk* chunk, int64_t x, int64_t z) {
        for (int cx = 0; cx < 16; cx++) {
            for (int cz = 0; cz < 16; cz++) {
                int height = level->getHeightmap((int)(cx + x * 16), (int)(cz + z * 16));
                for (int cy = height; cy >= 0; cy--) {
                    level->updateLight(LightLayer::Sky, (int)(cx + x * 16), cy, (int)(cz + z * 16),
                                       (int)(cx + x * 16), cy, (int)(cz + z * 16));
                    level->updateLight(LightLayer::Block, (int)(cx + x * 16 - 1), cy, (int)(cz + z * 16 - 1),
                                       (int)(cx + x * 16 + 1), cy, (int)(cz + z * 16 + 1));
                }
            }
        }
    }

    LevelChunk* load(int64_t x, int64_t z) {
        if (storage == NULL) return emptyChunk;
        LevelChunk* levelChunk = storage->load(level, x, z);
        if (levelChunk != NULL) levelChunk->lastSaveTime = level->getTime();
        return levelChunk;
    }

    int64_t xLast, zLast;
    LevelChunk* emptyChunk;
    ChunkSource* source;
    ChunkStorage* storage;
    std::unordered_map<std::pair<int64_t, int64_t>, LevelChunk*, pair_hash> chunks;
    Level* level;
    LevelChunk* last;

    unsigned char* genBlocks;
    std::set<std::pair<int64_t, int64_t>> deferredLighting;
    std::set<std::pair<int64_t, int64_t>> deferredPostProcess;
};

#endif
