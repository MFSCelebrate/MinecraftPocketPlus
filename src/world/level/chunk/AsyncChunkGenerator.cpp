#include "AsyncChunkGenerator.h"
#include "LevelChunk.h"                     // 同一目录下
#include "ChunkCache.h"                     // 同一目录下
#include "../levelgen/RandomLevelSource.h"  // 往上一级到 levelgen

AsyncChunkGenerator::AsyncChunkGenerator(RandomLevelSource* source, ChunkCache* cache, Level* level)
    : source(source), cache(cache), level(level), running(false) {}

AsyncChunkGenerator::~AsyncChunkGenerator() {
    stop();
}

void AsyncChunkGenerator::start() {
    running = true;
    worker = std::thread(&AsyncChunkGenerator::workerLoop, this);
}

void AsyncChunkGenerator::stop() {
    {
        std::lock_guard<std::mutex> lock(mutex);
        running = false;
    }
    cv.notify_one();
    if (worker.joinable()) worker.join();
}

void AsyncChunkGenerator::requestChunk(int64_t x, int64_t z) {
    std::lock_guard<std::mutex> lock(mutex);
    taskQueue.push({x, z});
    cv.notify_one();
}

void AsyncChunkGenerator::processCompletedChunks() {
    std::unordered_map<std::pair<int64_t, int64_t>, LevelChunk*, ChunkPosHash> completed;
    {
        std::lock_guard<std::mutex> lock(mutex);
        completed.swap(resultMap);
    }
    for (auto& [pos, chunk] : completed) {
        if (chunk) {
            cache->putChunk(pos.first, pos.second, chunk);
        }
    }
}

void AsyncChunkGenerator::workerLoop() {
    while (true) {
        std::pair<int64_t, int64_t> task;
        {
            std::unique_lock<std::mutex> lock(mutex);
            cv.wait(lock, [this]{ return !running || !taskQueue.empty(); });
            if (!running) break;
            task = taskQueue.front();
            taskQueue.pop();
        }
        LevelChunk* chunk = source->getChunk(task.first, task.second);
        {
            std::lock_guard<std::mutex> lock(mutex);
            resultMap[task] = chunk;
        }
    }
}
