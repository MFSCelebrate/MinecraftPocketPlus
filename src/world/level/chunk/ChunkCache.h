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

        // 若底层源是随机地形生成器，则创建异步预生成器（暂不启动）
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

    // ★ 永远同步获取区块（必须的区块立即生成）
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
        bool updateLights = false;
        if (newChunk == NULL) {
            if (source == NULL) {
                newChunk = emptyChunk;
            } else {
                // 直接走同步生成，保证区块立即可用
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
                    int height = level->getHeightmap((int)(cx + x * 16), (int)(cz + z * 16));
                    for (int cy = height; cy >= 0; cy--) {
                        level->updateLight(LightLayer::Sky, (int)(cx + x * 16), cy, (int)(cz + z * 16), (int)(cx + x * 16), cy, (int)(cz + z * 16));
                        level->updateLight(LightLayer::Block, (int)(cx + x * 16 - 1), cy, (int)(cz + z * 16 - 1), (int)(cx + x * 16 + 1), cy, (int)(cz + z * 16 + 1));
                    }
                }
            }
        }

        if (newChunk != NULL) {
            newChunk->load();
        }

        if (!newChunk->terrainPopulated && hasChunk(x + 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x + 1, z))
            postProcess(this, x, z);
        if (hasChunk(x - 1, z) && !getChunk(x - 1, z)->terrainPopulated && hasChunk(x - 1, z + 1) && hasChunk(x, z + 1) && hasChunk(x - 1, z))
            postProcess(this, x - 1, z);
        if (hasChunk(x, z - 1) && !getChunk(x, z - 1)->terrainPopulated && hasChunk(x + 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x + 1, z))
            postProcess(this, x, z - 1);
        if (hasChunk(x - 1, z - 1) && !getChunk(x - 1, z - 1)->terrainPopulated && hasChunk(x - 1, z - 1) && hasChunk(x, z - 1) && hasChunk(x - 1, z))
            postProcess(this, x - 1, z - 1);

        xLast = x;
        zLast = z;
        last = newChunk;
        return newChunk;
    }

    // ★ 后台生成的区块通过此方法安静加入缓存
    void putChunk(int64_t x, int64_t z, LevelChunk* chunk) {
        auto key = std::make_pair(x, z);
        if (chunks.find(key) == chunks.end()) {
            chunks[key] = chunk;
            chunk->lightLava();
            // 不需要光照更新，因为只是预先填充，真正需要时 getChunk 会再处理
        } else {
            chunk->deleteBlockData();
            delete chunk;
        }
    }

    // ★ 每帧调用：启动预生成器并提交任务
    bool tick() {
        // 首次 tick 或 asyncGen 存在即启动（仅一次）
        if (asyncGen && !asyncGenStarted) {
            asyncGen->start();
            asyncGenStarted = true;
        }

        // 分发后台任务：以每个玩家为中心，预生成周围半径内未加载的区块
        if (asyncGen && asyncGenStarted) {
            const int pregenRadius = 6;  // 预加载半径
            const PlayerList& players = level->players;
            for (size_t i = 0; i < players.size(); ++i) {
                Player* player = players[i];
                int cx = Mth::floor(player->x / 16.0);
                int cz = Mth::floor(player->z / 16.0);
                for (int dx = -pregenRadius; dx <= pregenRadius; ++dx) {
                    for (int dz = -pregenRadius; dz <= pregenRadius; ++dz) {
                        int nx = cx + dx;
                        int nz = cz + dz;
                        if (!hasChunk(nx, nz)) {
                            asyncGen->requestChunk(nx, nz);
                        }
                    }
                }
            }

            // 将后台完成的区块装入缓存
            asyncGen->processCompletedChunks();
        }

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
    std::string gatherStats() { return "ChunkCache: async pregen"; }

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
};

#endif
