#ifndef NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__
#define NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__

#include <map>
#include <string>
#include <unordered_map>  // ← 添加这个头文件
#include <cstdint>        // ← 添加这个头文件
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
	bool write(int sector, RakNet::BitStream& chunkData);
	void close();

	FILE* file;
	std::string	filename;
	int* offsets;
	int* emptyChunk;
	FreeSectorMap sectorFree;
	
	// 临时添加，让编译通过 ↓↓↓
	bool useOldFormat;  // 标记是否为旧格式
	std::unordered_map<int64_t, int> chunkToSector;  // 区块坐标 -> 扇区映射
};

#endif /*NET_MINECRAFT_WORLD_LEVEL_STORAGE__RegionFile_H__*/
