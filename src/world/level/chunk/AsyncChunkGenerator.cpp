// AsyncChunkGenerator.cpp
#include "AsyncChunkGenerator.h"
#include "../../world/level/chunk/LevelChunk.h"
#include "../../world/level/chunk/ChunkCache.h"
#include "../../world/level/levelgen/RandomLevelSource.h"

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
            // 将生成的区块安装到 ChunkCache 中
            // 需要 ChunkCache 提供 putChunk 方法（见下文）
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

        // 同步调用 RandomLevelSource::getChunk（可能耗时）
        LevelChunk* chunk = source->getChunk(task.first, task.second);

        {
            std::lock_guard<std::mutex> lock(mutex);
            resultMap[task] = chunk;
        }
    }
}
