#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__

#include <map>
#include <string>
#include <cstdint>
#include "../../../raknet/BitStream.h"

typedef std::map<int, bool> FreeSectorMap;

class RegionFile {
public:
    RegionFile(const std::string& basePath);
    virtual ~RegionFile();

    bool open();
    bool readChunk(int64_t x, int64_t z, RakNet::BitStream** destChunkData);
    bool writeChunk(int64_t x, int64_t z, RakNet::BitStream& chunkData);

private:
    int  allocateSector(int sectorsNeeded);
    void readSectorMap();
    void writeSectorMap();
    bool write(int sector, RakNet::BitStream& chunkData);
    void close();

    FILE* file;
    std::string filename;
    int* offsets;
    int* emptyChunk;
    FreeSectorMap sectorFree;
    bool useOldFormat;
    std::map<std::pair<int64_t, int64_t>, int> chunkToSector;
};

#endif
