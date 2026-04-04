#include "RandomLevelSource.h"

#include "feature/FeatureInclude.h"
#include "../Level.h"
#include "../ChunkPos.h"
#include "../MobSpawner.h"
#include "../biome/Biome.h"
#include "../biome/BiomeSource.h"
#include "../chunk/LevelChunk.h"
#include "../material/Material.h"
#include "../tile/Tile.h"
#include "../tile/HeavyTile.h"
#include "../../../util/Random.h"
#include "../../../client/Minecraft.h"   // <-- 添加此行
const float RandomLevelSource::SNOW_CUTOFF = 0.5f;
const float RandomLevelSource::SNOW_SCALE = 0.3f;
static const int MAX_BUFFER_SIZE = 1024;

RandomLevelSource::RandomLevelSource(Level* level, long seed, int version, bool spawnMobs)
:   random(seed),
    level(level),
    lperlinNoise1(&random, 16),
    lperlinNoise2(&random, 16),
    perlinNoise1(&random, 8),
    perlinNoise2(&random, 4),
    perlinNoise3(&random, 4),
    scaleNoise(&random, 10),
    depthNoise(&random, 16),
    forestNoise(&random, 8),
    spawnMobs(spawnMobs),
    pnr(NULL), ar(NULL), br(NULL), sr(NULL), dr(NULL), fi(NULL), fis(NULL),
    offsetX(0), offsetZ(0)
{
    for (int i=0; i<32; ++i)
        for (int j=0; j<32; ++j)
            waterDepths[i][j] = 0;

    buffer = new float[MAX_BUFFER_SIZE];

    Random randomCopy = random;
    printf("random.get : %d\n", randomCopy.nextInt());

    if (Minecraft::instance) {
        std::string xStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_X);
        std::string zStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_Z);
        if (!xStr.empty()) {
            int64_t rawX = atoll(xStr.c_str());
            int64_t alignedXBlocks = (rawX / 16) * 16;
            offsetX = alignedXBlocks / 16;
            if (rawX != alignedXBlocks) {
                LOGI("World Offset X adjusted from %lld blocks to %lld blocks (chunks: %lld)\n", rawX, alignedXBlocks, offsetX);
            }
        }
        if (!zStr.empty()) {
            int64_t rawZ = atoll(zStr.c_str());
            int64_t alignedZBlocks = (rawZ / 16) * 16;
            offsetZ = alignedZBlocks / 16;
            if (rawZ != alignedZBlocks) {
                LOGI("World Offset Z adjusted from %lld blocks to %lld blocks (chunks: %lld)\n", rawZ, alignedZBlocks, offsetZ);
            }
        }
    }
    customSeaLevel = 63;
    if (Minecraft::instance) {
        std::string seaStr = Minecraft::instance->options.getStringValue(OPTIONS_SEA_LEVEL);
        if (!seaStr.empty()) {
            int sl = atoi(seaStr.c_str());
            if (sl >= 0 && sl <= 127) customSeaLevel = sl;
            else LOGI("Sea level adjusted from %d to %d (must be 0-127)\n", sl, customSeaLevel);
        }
    }
}

RandomLevelSource::~RandomLevelSource() {

	// chunks are deleted in the chunk cache instead
	//ChunkMap::iterator it = chunkMap.begin();
	//while (it != chunkMap.end()) {
	//	it->second->deleteBlockData(); //@attn: we delete the block data here, for now
	//	delete it->second;
	//	++it;
	//}

	delete[] buffer;
	delete[] pnr;
	delete[] ar;
	delete[] br;
	delete[] sr;
	delete[] dr;
	delete[] fi;
	delete[] fis;
}

/*public*/
void RandomLevelSource::prepareHeights(int64_t xOffs, int64_t zOffs, unsigned char* blocks, void* biomes, float* temperatures) {
    int waterHeight = customSeaLevel + 1;
    if (waterHeight < 0) waterHeight = 0;
    if (waterHeight > 127) waterHeight = 127;

    int xChunks = 16 / CHUNK_WIDTH;
    int xSize = xChunks + 1;
    int ySize = 128 / CHUNK_HEIGHT + 1;
    int zSize = xChunks + 1;
    buffer = getHeights(buffer, xOffs * xChunks, 0, zOffs * xChunks, xSize, ySize, zSize);

    for (int xc = 0; xc < xChunks; xc++) {
        for (int zc = 0; zc < xChunks; zc++) {
            for (int yc = 0; yc < 128 / CHUNK_HEIGHT; yc++) {
                float yStep = 1 / (float) CHUNK_HEIGHT;
                float s0 = buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s1 = buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 0)];
                float s2 = buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 0)];
                float s3 = buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 0)];

                float s0a = (buffer[((xc + 0) * zSize + (zc + 0)) * ySize + (yc + 1)] - s0) * yStep;
                float s1a = (buffer[((xc + 0) * zSize + (zc + 1)) * ySize + (yc + 1)] - s1) * yStep;
                float s2a = (buffer[((xc + 1) * zSize + (zc + 0)) * ySize + (yc + 1)] - s2) * yStep;
                float s3a = (buffer[((xc + 1) * zSize + (zc + 1)) * ySize + (yc + 1)] - s3) * yStep;

                for (int y = 0; y < CHUNK_HEIGHT; y++) {
                    float xStep = 1 / (float) CHUNK_WIDTH;
                    float _s0 = s0;
                    float _s1 = s1;
                    float _s0a = (s2 - s0) * xStep;
                    float _s1a = (s3 - s1) * xStep;

                    for (int x = 0; x < CHUNK_WIDTH; x++) {
                        int offs = (x + xc * CHUNK_WIDTH) << 11 | (0 + zc * CHUNK_WIDTH) << 7 | (yc * CHUNK_HEIGHT + y);
                        int step = 1 << 7;
                        float zStep = 1 / (float) CHUNK_WIDTH;
                        float val = _s0;
                        float vala = (_s1 - _s0) * zStep;
                        for (int z = 0; z < CHUNK_WIDTH; z++) {
                            float temp = temperatures[(xc * CHUNK_WIDTH + x) * 16 + (zc * CHUNK_WIDTH + z)];
                            int tileId = 0;
                            if (yc * CHUNK_HEIGHT + y < waterHeight) {
                                if (temp < SNOW_CUTOFF && yc * CHUNK_HEIGHT + y >= waterHeight - 1) {
                                    tileId = Tile::ice->id;
                                } else {
                                    tileId = Tile::calmWater->id;
                                }
                            }
                            if (val > 0) {
                                tileId = Tile::rock->id;
                            }
                            blocks[offs] = (unsigned char) tileId;
                            offs += step;
                            val += vala;
                        }
                        _s0 += _s0a;
                        _s1 += _s1a;
                    }
                    s0 += s0a;
                    s1 += s1a;
                    s2 += s2a;
                    s3 += s3a;
                }
            }
        }
    }
}

void RandomLevelSource::buildSurfaces(int64_t xOffs, int64_t zOffs, unsigned char* blocks, Biome** biomes) {
    int waterHeight = customSeaLevel + 1;
    if (waterHeight < 0) waterHeight = 0;
    if (waterHeight > 127) waterHeight = 127;

    float s = 1 / 32.0f;
    float xf = (float)(xOffs * 16);
    float zf = (float)(zOffs * 16);
    perlinNoise2.getRegion(sandBuffer, xf, zf, 0, 16, 16, 1, s, s, 1);
    perlinNoise2.getRegion(gravelBuffer, xf, 109.01340f, zf, 16, 1, 16, s, 1, s);
    perlinNoise3.getRegion(depthBuffer, xf, zf, 0, 16, 16, 1, s * 2, s * 2, s * 2);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            float temp = 1;
            Biome* b = biomes[x + z * 16];
            bool sand = (sandBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 0;
            bool gravel = (gravelBuffer[x + z * 16] + random.nextFloat() * 0.2f) > 3;
            int runDepth = (int)(depthBuffer[x + z * 16] / 3 + 3 + random.nextFloat() * 0.25f);
            int run = -1;
            char top = b->topMaterial;
            char material = b->material;
            for (int y = 127; y >= 0; y--) {
                int offs = (z * 16 + x) * 128 + y;
                if (y <= 0 + random.nextInt(5)) {
                    blocks[offs] = (char) Tile::unbreakable->id;
                } else {
                    int old = blocks[offs];
                    if (old == 0) {
                        run = -1;
                    } else if (old == Tile::rock->id) {
                        if (run == -1) {
                            if (runDepth <= 0) {
                                top = 0;
                                material = (char) Tile::rock->id;
                            } else if (y >= waterHeight - 4 && y <= waterHeight + 1) {
                                top = b->topMaterial;
                                material = b->material;
                                if (gravel) {
                                    top = 0;
                                    material = (char) Tile::gravel->id;
                                }
                                if (sand) {
                                    top = (char) Tile::sand->id;
                                    material = (char) Tile::sand->id;
                                }
                            }
                            if (y < waterHeight && top == 0) {
                                if (temp < 0.15f)
                                    top = (char) Tile::ice->id;
                                else
                                    top = (char) Tile::calmWater->id;
                            }
                            run = runDepth;
                            if (y >= waterHeight - 1) blocks[offs] = top;
                            else blocks[offs] = material;
                        } else if (run > 0) {
                            run--;
                            blocks[offs] = material;
                            if (run == 0 && material == Tile::sand->id) {
                                run = random.nextInt(4);
                                material = (char) Tile::sandStone->id;
                            }
                        }
                    }
                }
            }
        }
    }
}


/*public*/

void RandomLevelSource::postProcess(ChunkSource* parent, int64_t xt, int64_t zt) {
    int64_t realXt = xt + offsetX;
    int64_t realZt = zt + offsetZ;
    if (!level->hasChunk(realXt-1, realZt-1) || !level->hasChunk(realXt, realZt-1) ||
        !level->hasChunk(realXt-1, realZt) || !level->hasChunk(realXt, realZt)) {
        return;
    }
    level->isGeneratingTerrain = true;
    HeavyTile::instaFall = true;

    int xo = (int)(realXt * 16);
    int zo = (int)(realZt * 16);
    Biome* biome = level->getBiomeSource()->getBiome(xo + 16, zo + 16);
    random.setSeed(level->getSeed());
    int xScale = random.nextInt() / 2 * 2 + 1;
    int zScale = random.nextInt() / 2 * 2 + 1;
    random.setSeed(((realXt * xScale) + (realZt * zScale)) ^ level->getSeed());

    // 以下所有使用 xo, zo 的地方不变，因为它们已经是 int 范围内的世界坐标
    // ... 原有代码（生成树、矿石、花朵等）保持不变 ...
	// //@todo: hide those chunks if they are aren't visible
//    if (random.nextInt(4) == 0) {
//        int x = xo + random.nextInt(16) + 8;
//        int y = random.nextInt(128);
//        int z = zo + random.nextInt(16) + 8;
//        LakeFeature feature(Tile::calmWater->id);
//		feature.place(level, &random, x, y, z);
//        LOGI("Adding underground lake @ (%d,%d,%d)\n", x, y, z);
//    }

	////@todo: hide those chunks if they are aren't visible
 //   if (random.nextInt(8) == 0) {
 //       int x = xo + random.nextInt(16) + 8;
 //       int y = random.nextInt(random.nextInt(120) + 8);
 //       int z = zo + random.nextInt(16) + 8;
 //       if (y < 64 || random.nextInt(10) == 0) {
	//		LakeFeature feature(Tile::calmLava->id);
	//		feature.place(level, &random, x, y, z);
	//	}
 //   }

	static float totalTime = 0;
	const float st = getTimeS();

    //for (int i = 0; i < 8; i++) {
    //    int x = xo + random.nextInt(16) + 8;
    //    int y = random.nextInt(128);
    //    int z = zo + random.nextInt(16) + 8;
    //    MonsterRoomFeature().place(level, random, x, y, z);
    //}

    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        ClayFeature feature(32);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 20; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::dirt->id, 32);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::gravel->id, 32);
		feature.place(level, &random, x, y, z);
    }

    // Coal: common, wide Y range, moderate vein size
    for (int i = 0; i < 16; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::coalOre->id, 14);
        feature.place(level, &random, x, y, z);
    }

    // Iron: common, limited to upper underground
    for (int i = 0; i < 14; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(64);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::ironOre->id, 10);
        feature.place(level, &random, x, y, z);
    }

    // Gold: rarer and deeper
    for (int i = 0; i < 2; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(32);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::goldOre->id, 9);
        feature.place(level, &random, x, y, z);
    }

    // Redstone: somewhat common at low depths
    for (int i = 0; i < 6; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(16);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::redStoneOre->id, 8);
        feature.place(level, &random, x, y, z);
    }

    // Emerald (diamond-equivalent): still rare but slightly more than vanilla
    for (int i = 0; i < 3; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(16);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::emeraldOre->id, 6);
        feature.place(level, &random, x, y, z);
    }

    // Lapis: rare and not in very high Y
    for (int i = 0; i < 1; i++) {
        int x = xo + random.nextInt(16);
        int y = random.nextInt(16) + random.nextInt(16);
        int z = zo + random.nextInt(16);
        OreFeature feature(Tile::lapisOre->id, 6);
        feature.place(level, &random, x, y, z);
    }

    const float ss = 0.5f;
    int oFor = (int) ((forestNoise.getValue(xo * ss, zo * ss) / 8 + random.nextFloat() * 4 + 4) / 3);
    int forests = 0;//1; (java: 0)
    if (random.nextInt(10) == 0) forests += 1;

    if (biome == Biome::forest) forests += oFor + 2; // + 5
    if (biome == Biome::rainForest) forests += oFor + 2; //+ 5
    if (biome == Biome::seasonalForest) forests += oFor + 1; // 2
    if (biome == Biome::taiga) {
		forests += oFor + 1; // + 5
		//LOGI("Biome is taiga!\n");
	}

    if (biome == Biome::desert) forests -= 20;
    if (biome == Biome::tundra) forests -= 20;
    if (biome == Biome::plains) forests -= 20;

    for (int i = 0; i < forests; i++) {
        int x = xo + random.nextInt(16) + 8;
        int z = zo + random.nextInt(16) + 8;
		int y = level->getHeightmap(x, z);
        Feature* tree = biome->getTreeFeature(&random);
		if (tree) {
	        tree->init(1, 1, 1);
		    tree->place(level, &random, x, y, z);
			delete tree;
		}
		//printf("placing tree at %d, %d, %d\n", x, y, z);
    }

    for (int i = 0; i < 2; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::flower->id);
		feature.place(level, &random, x, y, z);
    }

    if (random.nextInt(2) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
		FlowerFeature feature(Tile::rose->id);
        feature.place(level, &random, x, y, z);
    }

    if (random.nextInt(4) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::mushroom1->id);
		feature.place(level, &random, x, y, z);
    }

    if (random.nextInt(8) == 0) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        FlowerFeature feature(Tile::mushroom2->id);
		feature.place(level, &random, x, y, z);
    }
	/*int grassCount = 1;
	for (int i = 0; i < grassCount; i++) {
		int x = xo + random.nextInt(16) + 8;
		int y = random.nextInt(Level::genDepth);
		int z = zo + random.nextInt(16) + 8;
		Feature* grassFeature = biome->getGrassFeature(&random);
		if (grassFeature) {
			grassFeature->place(level, &random, x, y, z);
			delete grassFeature;
		}
	}*/
    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        ReedsFeature feature;
		feature.place(level, &random, x, y, z);
    }
	

    //if (random.nextInt(32) == 0) {
    //    int x = xo + random.nextInt(16) + 8;
    //    int y = random.nextInt(128);
    //    int z = zo + random.nextInt(16) + 8;
    //    PumpkinFeature().place(level, random, x, y, z);
    //}

    int cacti = 0;
    if (biome == Biome::desert) cacti += 5;

    for (int i = 0; i < cacti; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        CactusFeature feature;
        //LOGI("Tried creating a cactus at %d, %d, %d\n", x, y, z);
        feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 50; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(random.nextInt(120) + 8);
        int z = zo + random.nextInt(16) + 8;
        SpringFeature feature(Tile::water->id);
		feature.place(level, &random, x, y, z);
    }

    for (int i = 0; i < 20; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(random.nextInt(random.nextInt(112) + 8) + 8);
        int z = zo + random.nextInt(16) + 8;
        SpringFeature feature(Tile::lava->id);
		feature.place(level, &random, x, y, z);
    }

	if (spawnMobs && !level->isClientSide)
		MobSpawner::postProcessSpawnMobs(level, biome, xo + 8, zo + 8, 16, 16, &random);

    // 注意：所有对 level->getHeightmap 等函数的调用，参数使用 xo + offset 等，但 xo, zo 本身已经是世界坐标，不需要额外转换。

    // 最后处理雪
    float* temperatures = level->getBiomeSource()->getTemperatureBlock(xo + 8, zo + 8, 16, 16);
    for (int x = xo + 8; x < xo + 8 + 16; x++)
        for (int z = zo + 8; z < zo + 8 + 16; z++) {
            int xp = x - (xo + 8);
            int zp = z - (zo + 8);
            int y = level->getTopSolidBlock(x, z);
            float temp = temperatures[xp * 16 + zp] - (y - customSeaLevel) / 64.0f * SNOW_SCALE;
            if (temp < SNOW_CUTOFF) {
                if (y > 0 && y < 128 && level->isEmptyTile(x, y, z) && level->getMaterial(x, y - 1, z)->blocksMotion()) {
                    if (level->getMaterial(x, y - 1, z) != Material::ice) level->setTile(x, y, z, Tile::topSnow->id);
                }
            }
        }

    HeavyTile::instaFall = false;
    level->isGeneratingTerrain = false;
}

LevelChunk* RandomLevelSource::create(int64_t x, int64_t z) {
    return getChunk(x, z);
}

LevelChunk* RandomLevelSource::getChunk(int64_t xOffs, int64_t zOffs) {
    int64_t realX = xOffs + offsetX;
    int64_t realZ = zOffs + offsetZ;
    int64_t hashedPos = (realX << 32) | (realZ & 0xffffffff);
    ChunkMap::iterator it = chunkMap.find(hashedPos);
    if (it != chunkMap.end())
        return it->second;

    random.setSeed((long)(realX * 341872712l + realZ * 132899541l));

    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    LevelChunk* levelChunk = new LevelChunk(level, blocks, (int)xOffs, (int)zOffs);
    chunkMap.insert(std::make_pair(hashedPos, levelChunk));

    // 注意：getBiomeBlock 和 getTemperatureBlock 等函数仍使用 int，因为它们的参数是方块坐标，而 realX 可能超过 int 范围，但转换后精度丢失，可以接受。
    Biome** biomes = level->getBiomeSource()->getBiomeBlock((int)(realX * 16), (int)(realZ * 16), 16, 16);
    float* temperatures = level->getBiomeSource()->temperatures;
    prepareHeights(realX, realZ, blocks, 0, temperatures);
    buildSurfaces(realX, realZ, blocks, biomes);

    caveFeature.apply(this, level, (int)realX, (int)realZ, blocks, LevelChunk::ChunkBlockCount);
    levelChunk->recalcHeightmap();

    return levelChunk;
}

/*private*/
float* RandomLevelSource::getHeights(float* buffer, int64_t x, int y, int64_t z, int xSize, int ySize, int zSize) {
    float farlandsScale = 1.0f;  // 固定为 1.0，不读取选项
    float s = 684.412f * farlandsScale;
    float hs = 684.412f * farlandsScale;

    const int size = xSize * ySize * zSize;
    if (size > MAX_BUFFER_SIZE) {
        LOGI("RandomLevelSource::getHeights: TOO LARGE BUFFER REQUESTED: %d (max %d)\n", size, MAX_BUFFER_SIZE);
    }

    float* temperatures = level->getBiomeSource()->temperatures;
    float* downfalls = level->getBiomeSource()->downfalls;
    sr = scaleNoise.getRegion(sr, (int)x, (int)z, xSize, zSize, 1.121f, 1.121f, 0.5f);
    dr = depthNoise.getRegion(dr, (int)x, (int)z, xSize, zSize, 200.0f, 200.0f, 0.5f);

    float xf = (float)x;
    float zf = (float)z;
    float yf = (float)y;

    pnr = perlinNoise1.getRegion(pnr, xf, yf, zf, xSize, ySize, zSize, s / 80.0f, hs / 160.0f, s / 80.0f);
    ar = lperlinNoise1.getRegion(ar, xf, yf, zf, xSize, ySize, zSize, s, hs, s);
    br = lperlinNoise2.getRegion(br, xf, yf, zf, xSize, ySize, zSize, s, hs, s);

    int p = 0;
    int pp = 0;
    int wScale = 16 / xSize;
    for (int xx = 0; xx < xSize; xx++) {
        int xp = xx * wScale + wScale / 2;
        for (int zz = 0; zz < zSize; zz++) {
            int zp = zz * wScale + wScale / 2;
            float temperature = temperatures[xp * 16 + zp];
            float downfall = downfalls[xp * 16 + zp] * temperature;
            float dd = 1 - downfall;
            dd = dd * dd;
            dd = dd * dd;
            dd = 1 - dd;
            float scale = ((sr[pp] + 256.0f) / 512);
            scale *= dd;
            if (scale > 1) scale = 1;
            float depth = (dr[pp] / 8000.0f);
            if (depth < 0) depth = -depth * 0.3f;
            depth = depth * 3.0f - 2.0f;
            if (depth < 0) {
                depth = depth / 2;
                if (depth < -1) depth = -1;
                depth = depth / 1.4f;
                depth /= 2;
                scale = 0;
            } else {
                if (depth > 1) depth = 1;
                depth = depth / 8;
            }
            if (scale < 0) scale = 0;
            scale = (scale) + 0.5f;
            depth = depth * ySize / 16;
            float yCenter = ySize / 2.0f + depth * 4;
            pp++;
            for (int yy = 0; yy < ySize; yy++) {
                float val = 0;
                float yOffs = (yy - (yCenter)) * 12 / scale;
                if (yOffs < 0) yOffs *= 4;
                float bb = ar[p] / 512;
                float cc = br[p] / 512;
                float v = (pnr[p] / 10 + 1) / 2;
                if (v < 0) val = bb;
                else if (v > 1) val = cc;
                else val = bb + (cc - bb) * v;
                val -= yOffs;
                if (yy > ySize - 4) {
                    float slide = (yy - (ySize - 4)) / (4 - 1.0f);
                    val = val * (1 - slide) + -10 * slide;
                }
                buffer[p] = val;
                p++;
            }
        }
    }
    return buffer;
}

/*private*/
void RandomLevelSource::calcWaterDepths(ChunkSource* parent, int64_t xt, int64_t zt) {
    int xo = (int)(xt * 16);
    int zo = (int)(zt * 16);
    for (int x = 0; x < 16; x++) {
        int y = level->getSeaLevel();
        for (int z = 0; z < 16; z++) {
            int xp = xo + x + 7;
            int zp = zo + z + 7;
            int h = level->getHeightmap(xp, zp);
            if (h <= 0) {
                if (level->getHeightmap(xp - 1, zp) > 0 || level->getHeightmap(xp + 1, zp) > 0 || level->getHeightmap(xp, zp - 1) > 0 || level->getHeightmap(xp, zp + 1) > 0) {
                    bool hadWater = false;
                    if (hadWater || (level->getTile(xp - 1, y, zp) == Tile::calmWater->id && level->getData(xp - 1, y, zp) < 7)) hadWater = true;
                    if (hadWater || (level->getTile(xp + 1, y, zp) == Tile::calmWater->id && level->getData(xp + 1, y, zp) < 7)) hadWater = true;
                    if (hadWater || (level->getTile(xp, y, zp - 1) == Tile::calmWater->id && level->getData(xp, y, zp - 1) < 7)) hadWater = true;
                    if (hadWater || (level->getTile(xp, y, zp + 1) == Tile::calmWater->id && level->getData(xp, y, zp + 1) < 7)) hadWater = true;
                    if (hadWater) {
                        for (int x2 = -5; x2 <= 5; x2++) {
                            for (int z2 = -5; z2 <= 5; z2++) {
                                int d = (x2 > 0 ? x2 : -x2) + (z2 > 0 ? z2 : -z2);

                                if (d <= 5) {
                                    d = 6 - d;
                                    if (level->getTile(xp + x2, y, zp + z2) == Tile::calmWater->id) {
                                        int od = level->getData(xp + x2, y, zp + z2);
                                        if (od < 7 && od < d) {
                                            level->setData(xp + x2, y, zp + z2, d);
                                        }
                                    }
                                }
                            }
                        }
                        if (hadWater) {
                            level->setTileAndDataNoUpdate(xp, y, zp, Tile::calmWater->id, 7);
                            for (int y2 = 0; y2 < y; y2++) {
                                level->setTileAndDataNoUpdate(xp, y2, zp, Tile::calmWater->id, 8);
                            }
                        }
                    }
                }
            }
        }
    }
}

bool RandomLevelSource::hasChunk(int64_t x, int64_t z) {
    return true;
}

bool RandomLevelSource::tick() {
	return false;
}

bool RandomLevelSource::shouldSave() {
	return true;
}

std::string RandomLevelSource::gatherStats() {
	return "RandomLevelSource";
}

//bool RandomLevelSource::save(bool force, ProgressListener progressListener) {
//    return true;
//}

Biome::MobList RandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
    BiomeSource* biomeSource = level->getBiomeSource();
    if (biomeSource == NULL) {
        return Biome::MobList();
    }
//    static Stopwatch sw; sw.start();
    Biome* biome = biomeSource->getBiome(x, z);
//    sw.stop();
//    sw.printEvery(10, "getBiome::");
    if (biome == NULL) {
        return Biome::MobList();
    }
    return biome->getMobs(mobCategory);
}


LevelChunk* PerformanceTestChunkSource::create(int64_t x, int64_t z) {
    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    memset(blocks, 0, LevelChunk::ChunkBlockCount);

    // 使用 x, z 时可能需要转换为 int，但这里只是用于区块坐标，范围不大，可以强转
    int xi = (int)x;
    int zi = (int)z;
    // 原函数体中的代码使用 xi, zi 替代 x, z
    for (int y = 0; y < 65; y++) {
        if (y < 60) {
            for (int xx = (y + 1) & 1; xx < 16; xx += 2) {
                for (int zz = y & 1; zz < 16; zz += 2) {
                    blocks[xx << 11 | zz << 7 | y] = 3;
                }
            }
        } else {
            for (int xx = 0; xx < 16; xx += 2) {
                for (int zz = 0; zz < 16; zz += 2) {
                    blocks[xx << 11 | zz << 7 | y] = 3;
                }
            }
        }
    }

    LevelChunk* levelChunk = new LevelChunk(level, blocks, xi, zi);
    levelChunk->recalcHeightmap();
    return levelChunk;
}
