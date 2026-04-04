#if !defined(DEMO_MODE) && !defined(APPLE_DEMO_PROMOTION)

#include "LevelData.h"
#include "RegionFile.h"
#include "ExternalFileLevelStorage.h"
#include "FolderMethods.h"
#include "../chunk/LevelChunk.h"
#include "../Level.h"
#include "../LevelConstants.h"
#include "platform/log.h"
#include "../tile/TreeTile.h"
#include "../../entity/EntityFactory.h"
#include "../../../nbt/NbtIo.h"
#include "../../../util/RakDataIO.h"
#include "../../../raknet/GetTime.h"
#include "../tile/entity/TileEntity.h"

static const int ChunkVersion_Light = 1;
static const int ChunkVersion_Entity = 2;

const char* const fnLevelDatOld = "level.dat_old";
const char* const fnLevelDatNew = "level.dat_new";
const char* const fnLevelDat    = "level.dat";
const char* const fnPlayerDat   = "player.dat";

class LevelConverters
{
public:
    static bool v1_ClothIdToClothData(LevelChunk* c) {
        bool changed = false;
        unsigned char* blocks = c->getBlockData();
        unsigned char newTile = Tile::cloth->id;
        for (int i = 0; i < 16*16*128; ++i) {
            unsigned char oldTile = blocks[i];
            if (oldTile >= 101 && oldTile <= 115) {
                int color = 0xf - (oldTile - 101);
                blocks[i] = newTile;
                c->data.set(i, color);
                changed = true;
            }
        }
        return changed;
    }

    static bool ReplaceUnavailableBlocks(LevelChunk* c) {
        bool changed = false;
        unsigned char* blocks = c->getBlockData();
        for (int i = 0; i < 16*16*128; ++i) {
            unsigned char oldTile = blocks[i];
            unsigned char newTile = Tile::transformToValidBlockId(oldTile);
            if (oldTile != newTile) {
                blocks[i] = newTile;
                changed = true;
            }
        }
        return changed;
    }
};

ExternalFileLevelStorage::ExternalFileLevelStorage(const std::string& levelId, const std::string& fullPath)
:   levelId(levelId),
    levelPath(fullPath),
    loadedLevelData(NULL),
    regionFile(NULL),
    entitiesFile(NULL),
    tickCount(0),
    lastSavedEntitiesTick(-999999),
    level(NULL),
    loadedStorageVersion(SharedConstants::StorageVersion)
{
    createFolderIfNotExists(levelPath.c_str());

    std::string datFileName   = levelPath + "/" + fnLevelDat;
    std::string levelFileName = levelPath + "/" + fnPlayerDat;
    loadedLevelData = new LevelData();
    if (readLevelData(levelPath, *loadedLevelData))
    {
        loadedStorageVersion = loadedLevelData->getStorageVersion();
        // 禁用旧 player.dat 读取
        // readPlayerData(levelFileName, *loadedLevelData);
    } else {
        delete loadedLevelData;
        loadedLevelData = NULL;
    }
}

ExternalFileLevelStorage::~ExternalFileLevelStorage()
{
    delete regionFile;
    delete loadedLevelData;
}

void ExternalFileLevelStorage::saveLevelData(LevelData& levelData, std::vector<Player*>* players) {
    ExternalFileLevelStorage::saveLevelData(levelPath, levelData, players);
}

void ExternalFileLevelStorage::saveLevelData(const std::string& levelPath, LevelData& levelData, std::vector<Player*>* players)
{
    std::string directory = levelPath + "/";
    std::string tmpFile = directory + fnLevelDatNew;
    std::string datFile = directory + fnLevelDat;
    std::string oldFile = directory + fnLevelDatOld;

    levelData.setStorageVersion(SharedConstants::StorageVersion);
    if (!writeLevelData(tmpFile, levelData, players))
        return;

    remove(oldFile.c_str());
    if (exists(datFile.c_str())) {
        if (rename(datFile.c_str(), oldFile.c_str())) {
            LOGE("Error@saveLevelData: Couldn't move savefile to level.dat_old\n");
            return;
        }
        remove(datFile.c_str());
    }
    if (rename(tmpFile.c_str(), datFile.c_str())) {
        LOGE("Error@saveLevelData: Couldn't move new file to level.dat\n");
        return;
    }
    remove(tmpFile.c_str());
}

LevelData* ExternalFileLevelStorage::prepareLevel(Level* _level)
{
    level = _level;
    return loadedLevelData;
}

bool ExternalFileLevelStorage::readLevelData(const std::string& directory, LevelData& levelData)
{
    std::string datFilename = directory + "/" + fnLevelDat;
    FILE* file = fopen(datFilename.c_str(), "rb");
    if (!file) {
        datFilename = directory + "/" + fnLevelDatOld;
        file = fopen(datFilename.c_str(), "rb");
    }
    if (!file)
        return false;

    int version = 0;
    int size = 0;
    unsigned char* data = NULL;

    do {
        if (fread(&version, sizeof(version), 1, file) != 1) break;
        if (fread(&size, sizeof(size), 1, file) != 1) break;

        int left = getRemainingFileSize(file);
        if (size > left || size <= 0) break;

        data = new unsigned char[size];
        if (fread(data, 1, size, file) != size) break;

        if (version == 1) {
            RakNet::BitStream bitStream(data, size, false);
            levelData.v1_read(bitStream, version);
        } else if (version >= 2) {
            RakNet::BitStream bitStream(data, size, false);
            RakDataInput stream(bitStream);
            CompoundTag* tag = NbtIo::read(&stream);
            if (tag) {
                levelData.getTagData(tag);
                tag->deleteChildren();
                delete tag;
            }
        }
    } while (false);

    fclose(file);
    delete [] data;
    return true;
}

bool ExternalFileLevelStorage::writeLevelData(const std::string& datFileName, LevelData& levelData, const std::vector<Player*>* players)
{
    LOGI("Writing down level seed as: %ld\n", levelData.getSeed());
    FILE* file = fopen(datFileName.c_str(), "wb");
    if (!file)
        return false;

    RakNet::BitStream data;
    if (levelData.getStorageVersion() == 1)
        levelData.v1_write(data);
    else {
        RakDataOutput buf(data);
        CompoundTag* tag = (players && !players->empty()) ? levelData.createTag(*players) : levelData.createTag();
        NbtIo::write(tag, &buf);
        tag->deleteChildren();
        delete tag;
    }

    int version = levelData.getStorageVersion();
    fwrite(&version, sizeof(version), 1, file);
    int size = data.GetNumberOfBytesUsed();
    fwrite(&size, sizeof(size), 1, file);
    fwrite(data.GetData(), 1, size, file);
    fclose(file);
    return true;
}

bool ExternalFileLevelStorage::readPlayerData(const std::string& filename, LevelData& dest)
{
    return false;
}

void ExternalFileLevelStorage::tick()
{
    tickCount++;
    if ((tickCount % 1000) == 0 && level) {
        LOGI("Saving level...\n");
        for (int z = 0; z < CHUNK_CACHE_WIDTH; z++) {
            for (int x = 0; x < CHUNK_CACHE_WIDTH; x++) {
                LevelChunk* chunk = level->getChunk(x, z);
                if (chunk && chunk->unsaved) {
                    int pos = x + z * CHUNK_CACHE_WIDTH;
                    UnsavedChunkList::iterator prev = unsavedChunkList.begin();
                    for ( ; prev != unsavedChunkList.end(); ++prev) {
                        if ((*prev).pos == pos) {
                            (*prev).addedToList = RakNet::GetTimeMS();
                            break;
                        }
                    }
                    if (prev == unsavedChunkList.end()) {
                        UnsavedLevelChunk unsaved;
                        unsaved.pos = pos;
                        unsaved.addedToList = RakNet::GetTimeMS();
                        unsaved.chunk = chunk;
                        unsavedChunkList.push_back(unsaved);
                    }
                    chunk->unsaved = false;
                }
            }
        }
        savePendingUnsavedChunks(2);
    }
    if (tickCount - lastSavedEntitiesTick > (60 * SharedConstants::TicksPerSecond)) {
        saveEntities(level, NULL);
    }
}

void ExternalFileLevelStorage::save(Level* level, LevelChunk* levelChunk)
{
    if (!regionFile)
    {
        regionFile = new RegionFile(levelPath);
        if (!regionFile->open())
        {
            delete regionFile;
            regionFile = NULL;
            return;
        }
    }

    RakNet::BitStream chunkData;
    chunkData.Write((const char*)levelChunk->getBlockData(), CHUNK_BLOCK_COUNT);
    chunkData.Write((const char*)levelChunk->data.data, CHUNK_BLOCK_COUNT / 2);
    chunkData.Write((const char*)levelChunk->skyLight.data, CHUNK_BLOCK_COUNT / 2);
    chunkData.Write((const char*)levelChunk->blockLight.data, CHUNK_BLOCK_COUNT / 2);
    chunkData.Write((const char*)levelChunk->updateMap, CHUNK_COLUMNS);

    regionFile->writeChunk(levelChunk->x, levelChunk->z, chunkData);
}

LevelChunk* ExternalFileLevelStorage::load(Level* level, int64_t x, int64_t z)
{
    if (!regionFile)
    {
        regionFile = new RegionFile(levelPath);
        if (!regionFile->open())
        {
            delete regionFile;
            regionFile = NULL;
            return NULL;
        }
    }

    RakNet::BitStream* chunkData = NULL;
    if (!regionFile->readChunk(x, z, &chunkData))
        return NULL;

    chunkData->ResetReadPointer();
    unsigned char* blockIds = new unsigned char[CHUNK_BLOCK_COUNT];
    chunkData->Read((char*)blockIds, CHUNK_BLOCK_COUNT);

    LevelChunk* levelChunk = new LevelChunk(level, blockIds, (int)x, (int)z);
    chunkData->Read((char*)levelChunk->data.data, CHUNK_BLOCK_COUNT / 2);
    if (loadedStorageVersion >= ChunkVersion_Light) {
        chunkData->Read((char*)levelChunk->skyLight.data, CHUNK_BLOCK_COUNT / 2);
        chunkData->Read((char*)levelChunk->blockLight.data, CHUNK_BLOCK_COUNT / 2);
    }
    chunkData->Read((char*)levelChunk->updateMap, CHUNK_COLUMNS);

    delete [] chunkData->GetData();
    delete chunkData;

    bool changed = false;
    if (loadedStorageVersion == 1)
        changed |= LevelConverters::v1_ClothIdToClothData(levelChunk);
    changed |= LevelConverters::ReplaceUnavailableBlocks(levelChunk);

    levelChunk->recalcHeightmap();
    levelChunk->unsaved = changed;
    levelChunk->terrainPopulated = true;
    levelChunk->createdFromSave = true;

    return levelChunk;
}

void ExternalFileLevelStorage::saveEntities( Level* level, LevelChunk* levelChunk )
{
    lastSavedEntitiesTick = tickCount;
    int count = 0;
    float st = getTimeS();

    EntityList& entities = level->entities;
    ListTag* entityTags = new ListTag();
    for (unsigned int i = 0; i < entities.size(); ++i) {
        Entity* e = entities[i];
        CompoundTag* tag = new CompoundTag();
        if (e->save(tag)) {
            count++;
            entityTags->add(tag);
        } else
            delete tag;
    }

    TileEntityList& tileEntities = level->tileEntities;
    ListTag* tileEntityTags = new ListTag();
    for (unsigned int i = 0; i < tileEntities.size(); ++i) {
        TileEntity* e = tileEntities[i];
        if (!e->shouldSave()) continue;
        CompoundTag* tag = new CompoundTag();
        if (e->save(tag)) {
            count++;
            tileEntityTags->add(tag);
        } else
            delete tag;
    }

    CompoundTag base;
    base.put("Entities", entityTags);
    base.put("TileEntities", tileEntityTags);

    RakNet::BitStream stream;
    RakDataOutput dos(stream);
    NbtIo::write(&base, &dos);
    int numBytes = stream.GetNumberOfBytesUsed();

    FILE* fp = fopen((levelPath + "/entities.dat").c_str(), "wb");
    if (fp) {
        int version = 1;
        fwrite("ENT\0", 1, 4, fp);
        fwrite(&version, sizeof(int), 1, fp);
        fwrite(&numBytes, sizeof(int), 1, fp);
        fwrite(stream.GetData(), 1, numBytes, fp);
        fclose(fp);
    }

    base.deleteChildren();

    float tt = getTimeS() - st;
    LOGI("Time to save %d entities: %f s. Size: %d bytes\n", count, tt, numBytes);
}

void ExternalFileLevelStorage::loadEntities(Level* level, LevelChunk* chunk) {
    lastSavedEntitiesTick = tickCount;
    FILE* fp = fopen((levelPath + "/entities.dat").c_str(), "rb");
    if (fp) {
        char header[5];
        int version, numBytes;
        fread(header, 1, 4, fp);
        fread(&version, sizeof(int), 1, fp);
        fread(&numBytes, sizeof(int), 1, fp);

        int left = getRemainingFileSize(fp);
        if (numBytes <= left && numBytes > 0) {
            unsigned char* buf = new unsigned char[numBytes];
            fread(buf, 1, numBytes, fp);

            RakNet::BitStream stream(buf, numBytes, false);
            RakDataInput dis(stream);
            CompoundTag* tag = NbtIo::read(&dis);

            if (tag->contains("Entities", Tag::TAG_List)) {
                ListTag* entityTags = tag->getList("Entities");
                for (int i = 0; i < entityTags->size(); ++i) {
                    Tag* _et = entityTags->get(i);
                    if (!_et || _et->getId() != Tag::TAG_Compound) continue;
                    CompoundTag* et = (CompoundTag*)_et;
                    if (Entity* e = EntityFactory::loadEntity(et, level)) {
                        level->addEntity(e);
                    }
                }
            }
            if (tag->contains("TileEntities", Tag::TAG_List)) {
                ListTag* tileEntityTags = tag->getList("TileEntities");
                for (int i = 0; i < tileEntityTags->size(); ++i) {
                    Tag* _et = tileEntityTags->get(i);
                    if (!_et || _et->getId() != Tag::TAG_Compound) continue;
                    CompoundTag* et = (CompoundTag*)_et;
                    if (TileEntity* e = TileEntity::loadStatic(et)) {
                        LevelChunk* chunk = level->getChunkAt(e->x, e->z);
                        if (chunk && !chunk->hasTileEntityAt(e)) {
                            chunk->addTileEntity(e);
                        } else {
                            delete e;
                        }
                    }
                }
            }

            tag->deleteChildren();
            delete tag;
            delete[] buf;
        }
        fclose(fp);
    }
}

void ExternalFileLevelStorage::saveGame(Level* level) {
    saveEntities(level, NULL);
}

int ExternalFileLevelStorage::savePendingUnsavedChunks( int maxCount ) {
    if (maxCount < 0)
        maxCount = unsavedChunkList.size();

    int count = 0;
    while (++count <= maxCount && !unsavedChunkList.empty()) {
        UnsavedChunkList::iterator it = unsavedChunkList.begin();
        UnsavedChunkList::iterator remove = unsavedChunkList.begin();
        UnsavedLevelChunk* oldest = &(*it);
        for ( ; it != unsavedChunkList.end(); ++it) {
            if ((*it).addedToList < oldest->addedToList) {
                oldest = &(*it);
                remove = it;
            }
        }
        LevelChunk* chunk = oldest->chunk;
        unsavedChunkList.erase(remove);
        save(level, chunk);
    }
    return count;
}

void ExternalFileLevelStorage::saveAll( Level* level, std::vector<LevelChunk*>& levelChunks ) {
    ChunkStorage::saveAll(level, levelChunks);
    int numChunks = savePendingUnsavedChunks(-1);
    LOGI("Saving %d additional chunks.\n", numChunks);
}

#endif /*DEMO_MODE*/
