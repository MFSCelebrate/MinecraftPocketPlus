// AsyncChunkGenerator.h
#ifndef ASYNC_CHUNK_GENERATOR_H
#define ASYNC_CHUNK_GENERATOR_H

#include <cstdint>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <unordered_map>
#include <utility>

class LevelChunk;
class RandomLevelSource;
class ChunkCache;
class Level;

class AsyncChunkGenerator {
public:
    AsyncChunkGenerator(RandomLevelSource* source, ChunkCache* cache, Level* level);
    ~AsyncChunkGenerator();

    void start();
    void stop();
    void requestChunk(int64_t x, int64_t z);
    void processCompletedChunks();   // 主线程每帧调用

    struct ChunkPosHash {
        size_t operator()(const std::pair<int64_t, int64_t>& p) const {
            return p.first * 1000000007 + p.second;
        }
    };

private:
    void workerLoop();

    RandomLevelSource* source;
    ChunkCache* cache;
    Level* level;

    std::thread worker;
    bool running;

    std::mutex mutex;
    std::condition_variable cv;
    std::queue<std::pair<int64_t, int64_t>> taskQueue;
    std::unordered_map<std::pair<int64_t, int64_t>, LevelChunk*, ChunkPosHash> resultMap;
};

#endif
