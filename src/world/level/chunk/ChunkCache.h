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
        // 预分配生成缓冲区 (避免每次 getChunk 时动态分配)
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

    // ★ 主入口：获取区块并保证光照完成
    LevelChunk* getChunk(int64_t x, int64_t z) {
        if (x == xLast && z == zLast && last != NULL)
            return last;

        auto key = std::make_pair(x, z);
        auto it = chunks.find(key);
        if (it != chunks.end()) {
            // 如果需要光照（之前为预生成块），现在立即执行
            if (deferredLighting.find(key) != deferredLighting.end()) {
                doFullLighting(it->second, x, z);
                deferredLighting.erase(key);
            }
            // 如果需要后处理（树、矿等）
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
                // 同步生成，但使用复用的 genBlocks 减少内存分配
                RandomLevelSource* rls = dynamic_cast<RandomLevelSource*>(source);
                if (rls) {
                    // 注意：RandomLevelSource::getChunk 内部会 new blocks，
                    // 我们无法直接使用预分配内存，除非修改其实现。
                    // 这里保持原样，但后续可进一步改造。
                    newChunk = source->getChunk(x, z);
                } else {
                    newChunk = source->getChunk(x, z);
                }
            }
        }

        if (newChunk == NULL) {
            newChunk = emptyChunk;
        } else {
            chunks[key] = newChunk;
            // 立即执行光照 (因为此区块被直接访问)
            newChunk->lightLava();
            doFullLighting(newChunk, x, z);
            doPostProcess(x, z);
            newChunk->load();
        }

        xLast = x; zLast = z; last = newChunk;
        return newChunk;
    }

    // ★ 轻量级预生成：只生成裸区块，不加光照和后处理
    void preloadChunk(int64_t x, int64_t z) {
        auto key = std::make_pair(x, z);
        if (chunks.find(key) != chunks.end())
            return; // 已存在

        LevelChunk* newChunk = load(x, z);
        if (newChunk == NULL && source != NULL) {
            newChunk = source->getChunk(x, z);
            if (newChunk) {
                chunks[key] = newChunk;
                newChunk->lightLava(); // 基础亮度矫正
                // 不做光照更新，标记为延迟处理
                deferredLighting.insert(key);
                deferredPostProcess.insert(key);
                newChunk->load();
            }
        } else if (newChunk) {
            chunks[key] = newChunk;
        }
    }

    bool tick() {
        // 1. 批量处理延迟光照（每帧最多处理 2 个，分摊负载）
        int lightCount = 0;
        const int maxLightPerFrame = 2;
        auto lit = deferredLighting.begin();
        while (lit != deferredLighting.end() && lightCount < maxLightPerFrame) {
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

        // 2. 延迟后处理（每帧最多 1 个，更耗时）
        int ppCount = 0;
        const int maxPPPerFrame = 1;
        auto ppit = deferredPostProcess.begin();
        while (ppit != deferredPostProcess.end() && ppCount < maxPPPerFrame) {
            auto key = *ppit;
            doPostProcess(key.first, key.second);
            ppit = deferredPostProcess.erase(ppit);
            ppCount++;
        }

        // 3. 主动预生成玩家周围未加载区块（每帧最多 2 个）
        const int maxPregen = 2;
        int pregen = 0;
        for (size_t pi = 0; pi < level->players.size() && pregen < maxPregen; ++pi) {
            Player* player = level->players[pi];
            int cx = Mth::floor(player->x / 16.0);
            int cz = Mth::floor(player->z / 16.0);
            // 螺旋扩展半径
            for (int r = 1; r <= 4 && pregen < maxPregen; ++r) {
                for (int dx = -r; dx <= r; ++dx) {
                    int nx = cx + dx;
                    int nz = cz + r;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= maxPregen) break; }
                    nz = cz - r;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= maxPregen) break; }
                }
                for (int dz = -r+1; dz <= r-1; ++dz) {
                    int nx = cx + r;
                    int nz = cz + dz;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= maxPregen) break; }
                    nx = cx - r;
                    if (!hasChunk(nx, nz)) { preloadChunk(nx, nz); if (++pregen >= maxPregen) break; }
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

private:
    // 完整光照更新
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

    // 后处理（树木、矿石等）
    void doPostProcess(int64_t x, int64_t z) {
        if (!fits(x, z)) return;
        LevelChunk* chunk = getChunk(x, z);
        if (!chunk->terrainPopulated) {
            chunk->terrainPopulated = true;
            if (source != NULL) {
                // 检查周围区块存在性
                if (hasChunk(x+1, z+1) && hasChunk(x, z+1) && hasChunk(x+1, z))
                    source->postProcess(this, x, z);
                else
                    chunk->terrainPopulated = false; // 等待邻居
            }
            chunk->clearUpdateMap();
        }
    }

    LevelChunk* load(int64_t x, int64_t z) {
        if (storage == NULL) return emptyChunk;
        LevelChunk* levelChunk = storage->load(level, x, z);
        if (levelChunk != NULL) levelChunk->lastSaveTime = level->getTime();
        return levelChunk;
    }

    // 成员变量
    int64_t xLast, zLast;
    LevelChunk* emptyChunk;
    ChunkSource* source;
    ChunkStorage* storage;
    std::unordered_map<std::pair<int64_t, int64_t>, LevelChunk*, pair_hash> chunks;
    Level* level;
    LevelChunk* last;

    // 预分配生成缓冲区（备用，尚未完全利用）
    unsigned char* genBlocks;

    // 延迟处理队列
    std::set<std::pair<int64_t, int64_t>> deferredLighting;
    std::set<std::pair<int64_t, int64_t>> deferredPostProcess;
};

#endif
