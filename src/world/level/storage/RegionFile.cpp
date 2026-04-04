#include "RegionFile.h"
#include "../../../platform/log.h"

const int SECTOR_BYTES = 4096;
const int SECTOR_INTS = SECTOR_BYTES / 4;
const int SECTOR_COLS = 32;
const int MAP_MAGIC = 0x4D41505F;
const int MAP_VERSION = 1;

static const char* const REGION_DAT_NAME = "chunks.dat";

static void logAssert(int actual, int expected) {
    if (actual != expected) {
        LOGI("ERROR: I/O operation failed (%d vs %d)\n", actual, expected);
    }
}

RegionFile::RegionFile(const std::string& basePath)
:   file(NULL), useOldFormat(false)
{
    filename = basePath;
    filename += "/";
    filename += REGION_DAT_NAME;

    offsets = new int[SECTOR_INTS];
    emptyChunk = new int[SECTOR_INTS];
    memset(emptyChunk, 0, SECTOR_INTS * sizeof(int));
}

RegionFile::~RegionFile()
{
    close();
    delete [] offsets;
    delete [] emptyChunk;
}

bool RegionFile::open()
{
    close();

    memset(offsets, 0, SECTOR_INTS * sizeof(int));
    sectorFree.clear();
    chunkToSector.clear();
    useOldFormat = false;

    file = fopen(filename.c_str(), "r+b");
    if (file)
    {
        fseek(file, 0, SEEK_END);
        long fileSize = ftell(file);
        if (fileSize >= (int)sizeof(int) * 2) {
            fseek(file, - (int)sizeof(int), SEEK_END);
            int magic;
            fread(&magic, sizeof(int), 1, file);
            if (magic == MAP_MAGIC) {
                fseek(file, - (int)sizeof(int) * 2, SEEK_END);
                int version;
                fread(&version, sizeof(int), 1, file);
                if (version == MAP_VERSION) {
                    useOldFormat = false;
                    readSectorMap();
                } else {
                    useOldFormat = true;
                }
            } else {
                useOldFormat = true;
            }
        } else {
            useOldFormat = true;
        }

        if (useOldFormat) {
            fseek(file, 0, SEEK_SET);
            logAssert(fread(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);
            sectorFree[0] = false;
            for (int sector = 0; sector < SECTOR_INTS; sector++) {
                int offset = offsets[sector];
                if (offset) {
                    int base = offset >> 8;
                    int count = offset & 0xff;
                    for (int i = 0; i < count; i++)
                        sectorFree[base + i] = false;
                }
            }
        } else {
            for (auto& kv : chunkToSector) {
                int base = kv.second;
                sectorFree[base] = false;
            }
        }
    }
    else
    {
        file = fopen(filename.c_str(), "w+b");
        if (!file)
        {
            LOGI("Failed to create chunk file %s\n", filename.c_str());
            return false;
        }
        logAssert(fwrite(offsets, sizeof(int), SECTOR_INTS, file), SECTOR_INTS);
        sectorFree[0] = false;
        useOldFormat = false;
    }

    return file != NULL;
}

void RegionFile::close()
{
    if (file)
    {
        if (!useOldFormat) {
            writeSectorMap();
        }
        fclose(file);
        file = NULL;
    }
}

bool RegionFile::readChunk(int64_t x, int64_t z, RakNet::BitStream** destChunkData)
{
    std::pair<int64_t, int64_t> key(x, z);
    auto it = chunkToSector.find(key);
    if (it == chunkToSector.end() && useOldFormat) {
        int cx = (int)(x & (SECTOR_COLS - 1));
        int cz = (int)(z & (SECTOR_COLS - 1));
        int idx = cx + cz * SECTOR_COLS;
        int offset = offsets[idx];
        if (offset == 0) return false;
        int sectorNum = offset >> 8;
        fseek(file, sectorNum * SECTOR_BYTES, SEEK_SET);
        int length = 0;
        fread(&length, sizeof(int), 1, file);
        assert(length < ((offset & 0xff) * SECTOR_BYTES));
        length -= sizeof(int);
        if (length <= 0) return false;
        unsigned char* data = new unsigned char[length];
        logAssert(fread(data, 1, length, file), length);
        *destChunkData = new RakNet::BitStream(data, length, false);
        return true;
    } else if (it != chunkToSector.end()) {
        int sectorNum = it->second;
        fseek(file, sectorNum * SECTOR_BYTES, SEEK_SET);
        int length = 0;
        fread(&length, sizeof(int), 1, file);
        length -= sizeof(int);
        if (length <= 0) return false;
        unsigned char* data = new unsigned char[length];
        logAssert(fread(data, 1, length, file), length);
        *destChunkData = new RakNet::BitStream(data, length, false);
        return true;
    }
    return false;
}

bool RegionFile::writeChunk(int64_t x, int64_t z, RakNet::BitStream& chunkData)
{
    int size = chunkData.GetNumberOfBytesUsed() + sizeof(int);
    int sectorsNeeded = (size / SECTOR_BYTES) + 1;
    if (sectorsNeeded > 256) {
        LOGI("ERROR: Chunk is too big to be saved to file\n");
        return false;
    }

    std::pair<int64_t, int64_t> key(x, z);
    auto it = chunkToSector.find(key);
    int sectorNum = (it != chunkToSector.end()) ? it->second : 0;

    if (sectorNum != 0) {
        sectorFree[sectorNum] = true;
        sectorNum = 0;
    }

    if (sectorNum == 0) {
        sectorNum = allocateSector(sectorsNeeded);
        if (sectorNum < 0) return false;
        chunkToSector[key] = sectorNum;
    }

    fseek(file, sectorNum * SECTOR_BYTES, SEEK_SET);
    int totalSize = chunkData.GetNumberOfBytesUsed() + sizeof(int);
    fwrite(&totalSize, sizeof(int), 1, file);
    fwrite(chunkData.GetData(), 1, chunkData.GetNumberOfBytesUsed(), file);

    return true;
}

int RegionFile::allocateSector(int sectorsNeeded)
{
    int slot = 0;
    int runLength = 0;
    while (runLength < sectorsNeeded) {
        if (sectorFree.find(slot + runLength) == sectorFree.end()) {
            break;
        }
        if (sectorFree[slot + runLength] == true) {
            runLength++;
        } else {
            slot = slot + runLength + 1;
            runLength = 0;
        }
    }
    if (runLength < sectorsNeeded) {
        fseek(file, 0, SEEK_END);
        long endPos = ftell(file);
        int newSector = endPos / SECTOR_BYTES;
        for (int i = 0; i < sectorsNeeded; i++) {
            fwrite(emptyChunk, sizeof(int), SECTOR_INTS, file);
            sectorFree[newSector + i] = true;
        }
        slot = newSector;
    }
    for (int i = 0; i < sectorsNeeded; i++) {
        sectorFree[slot + i] = false;
    }
    return slot;
}

void RegionFile::writeSectorMap()
{
    fseek(file, 0, SEEK_END);
    long pos = ftell(file);
    int count = (int)chunkToSector.size();
    fwrite(&count, sizeof(int), 1, file);
    for (auto& kv : chunkToSector) {
        fwrite(&kv.first.first, sizeof(int64_t), 1, file);
        fwrite(&kv.first.second, sizeof(int64_t), 1, file);
        fwrite(&kv.second, sizeof(int), 1, file);
    }
    int version = MAP_VERSION;
    fwrite(&version, sizeof(int), 1, file);
    int magic = MAP_MAGIC;
    fwrite(&magic, sizeof(int), 1, file);
}

void RegionFile::readSectorMap()
{
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    fseek(file, - (int)sizeof(int) * 2, SEEK_END);
    int version, magic;
    fread(&version, sizeof(int), 1, file);
    fread(&magic, sizeof(int), 1, file);
    if (magic != MAP_MAGIC || version != MAP_VERSION) {
        useOldFormat = true;
        return;
    }
    fseek(file, - (int)sizeof(int) * 2 - (int)sizeof(int), SEEK_END);
    int count;
    fread(&count, sizeof(int), 1, file);
    for (int i = 0; i < count; i++) {
        int64_t x, z;
        int sector;
        fread(&x, sizeof(int64_t), 1, file);
        fread(&z, sizeof(int64_t), 1, file);
        fread(&sector, sizeof(int), 1, file);
        chunkToSector[std::make_pair(x, z)] = sector;
    }
}
