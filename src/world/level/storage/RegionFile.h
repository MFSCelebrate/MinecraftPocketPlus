#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__

#include <map>
#include <string>
#include <cstdint>          // int64_t
#include "../../../raknet/BitStream.h"

typedef std::map<int, bool> FreeSectorMap;

class RegionFile
{
public:
    RegionFile(const std::string& basePath);
    virtual ~RegionFile();

    bool open();
    bool readChunk(int x, int z, RakNet::BitStream** destChunkData);
    bool writeChunk(int x, int z, RakNet::BitStream& chunkData);

private:
    // 内部辅助函数
    int  allocateSector(int sectorsNeeded);
    void readSectorMap();
    void writeSectorMap();
    bool write(int sector, RakNet::BitStream& chunkData);  // 原声明，虽未实现但保留
    void close();

    // 成员变量
    FILE* file;
    std::string filename;
    int* offsets;
    int* emptyChunk;
    FreeSectorMap sectorFree;
    bool useOldFormat;

    // 关键：使用 pair 键，匹配 cpp 中的用法
    std::map<std::pair<int64_t, int64_t>, int> chunkToSector;
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__*/
