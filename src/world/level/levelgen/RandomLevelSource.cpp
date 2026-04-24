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
#include "../../../client/Minecraft.h"
#include <cmath>

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
    m_worldOffsetX(0.0), m_worldOffsetY(0.0), m_worldOffsetZ(0.0),
    m_worldScaleX(1.0f), m_worldScaleY(1.0f), m_worldScaleZ(1.0f),
    m_disableSkygrid(false)
{
    for (int i=0; i<32; ++i)
        for (int j=0; j<32; ++j)
            waterDepths[i][j] = 0;

    buffer = new float[MAX_BUFFER_SIZE];

    Random randomCopy = random;
    printf("random.get : %d\n", randomCopy.nextInt());

    // 读取世界缩放 X, Y, Z
    if (Minecraft::instance) {
        std::string scaleXStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_X);
        std::string scaleYStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_Y);
        std::string scaleZStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_SCALE_Z);
        if (!scaleXStr.empty()) {
            m_worldScaleX = (float)atof(scaleXStr.c_str());
            if (m_worldScaleX <= 0.0f) m_worldScaleX = 0.001f;
        } else m_worldScaleX = 1.0f;
        if (!scaleYStr.empty()) {
            m_worldScaleY = (float)atof(scaleYStr.c_str());
            if (m_worldScaleY <= 0.0f) m_worldScaleY = 0.001f;
        } else m_worldScaleY = 1.0f;
        if (!scaleZStr.empty()) {
            m_worldScaleZ = (float)atof(scaleZStr.c_str());
            if (m_worldScaleZ <= 0.0f) m_worldScaleZ = 0.001f;
        } else m_worldScaleZ = 1.0f;
        LOGI("World scale X=%.4f Y=%.4f Z=%.4f\n", m_worldScaleX, m_worldScaleY, m_worldScaleZ);
    }

    // 读取世界偏移 (double)
    if (Minecraft::instance) {
        std::string xStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_X);
        std::string yStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_Y);
        std::string zStr = Minecraft::instance->options.getStringValue(OPTIONS_WORLD_OFFSET_Z);
        if (!xStr.empty()) m_worldOffsetX = atof(xStr.c_str());
        if (!yStr.empty()) m_worldOffsetY = atof(yStr.c_str());
        if (!zStr.empty()) m_worldOffsetZ = atof(zStr.c_str());
        LOGI("RandomLevelSource: world offset = (%.2f, %.2f, %.2f)\n", m_worldOffsetX, m_worldOffsetY, m_worldOffsetZ);
    }

    customSeaLevel = 63;
    if (Minecraft::instance) {
        std::string seaStr = Minecraft::instance->options.getStringValue(OPTIONS_SEA_LEVEL);
        if (!seaStr.empty()) {
            int sl = atoi(seaStr.c_str());
            if (sl >= 0 && sl <= 127) customSeaLevel = sl;
            else LOGI("Sea level adjusted from %d to %d (must be 0-127)\n", sl, customSeaLevel);
        }
        m_disableSkygrid = Minecraft::instance->options.getBooleanValue(OPTIONS_DISABLE_SKYGRID);
    }
}

RandomLevelSource::~RandomLevelSource() {
    delete[] buffer;
    delete[] pnr;
    delete[] ar;
    delete[] br;
    delete[] sr;
    delete[] dr;
    delete[] fi;
    delete[] fis;
}

// 修改：xOffs, zOffs 改为 double 世界坐标
void RandomLevelSource::prepareHeights(double xOffs, double zOffs, unsigned char* blocks, void* biomes, float* temperatures) {
    int waterHeight = customSeaLevel + 1;
    if (waterHeight < 0) waterHeight = 0;
    if (waterHeight > 127) waterHeight = 127;

    int xChunks = 16 / CHUNK_WIDTH;
    int xSize = xChunks + 1;
    int ySize = 128 / CHUNK_HEIGHT + 1;
    int zSize = xChunks + 1;
    buffer = getHeights(buffer, xOffs, 0, zOffs, xSize, ySize, zSize);

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
                            if (m_disableSkygrid && (std::isnan(val) || std::isinf(val))) {
                                tileId = 0;
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

// 修改：xOffs, zOffs 改为 double 世界坐标
void RandomLevelSource::buildSurfaces(double xOffs, double zOffs, unsigned char* blocks, Biome** biomes) {
    int waterHeight = customSeaLevel + 1;
    if (waterHeight < 0) waterHeight = 0;
    if (waterHeight > 127) waterHeight = 127;

    float sx = (1.0f / 32.0f) * m_worldScaleX;
    float sz = (1.0f / 32.0f) * m_worldScaleZ;

    // 直接使用世界坐标缩放，不再添加偏移（偏移已在传入前叠加）
    double xf = xOffs / 4.0;
    double zf = zOffs / 4.0;
    perlinNoise2.getRegion(sandBuffer, (float)xf, (float)zf, 0, 16, 16, 1, sx, sz, 1.0f);
    perlinNoise2.getRegion(gravelBuffer, (float)xf, 109.01340f, (float)zf, 16, 1, 16, sx, 1.0f, sz);
    perlinNoise3.getRegion(depthBuffer, (float)xf, (float)zf, 0, 16, 16, 1, sx * 2.0f, sz * 2.0f, sz * 2.0f);

    for (int x = 0; x < 16; x++) {
        for (int z = 0; z < 16; z++) {
            float temp = 1;
            Biome* b = biomes[x + z * 16];
            float sandVal = sandBuffer[x + z * 16];
            float gravelVal = gravelBuffer[x + z * 16];
            bool sand = (sandVal + random.nextFloat() * 0.2f) > 0;
            bool gravel = (gravelVal + random.nextFloat() * 0.2f) > 3;
            if (m_disableSkygrid) {
                if (std::isnan(sandVal) || std::isinf(sandVal)) sand = false;
                if (std::isnan(gravelVal) || std::isinf(gravelVal)) gravel = false;
            }
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

// 修改：xt, zt 仍是区块坐标 int64_t（不变），内部世界坐标升级为 double
void RandomLevelSource::postProcess(ChunkSource* parent, int64_t xt, int64_t zt) {
    double worldBlockX = xt * 16.0 + m_worldOffsetX;
    double worldBlockZ = zt * 16.0 + m_worldOffsetZ;
    int xo = (int)worldBlockX;
    int zo = (int)worldBlockZ;

    if (!level->hasChunk(xt-1, zt-1) || !level->hasChunk(xt, zt-1) ||
        !level->hasChunk(xt-1, zt) || !level->hasChunk(xt, zt)) {
        return;
    }
    level->isGeneratingTerrain = true;
    HeavyTile::instaFall = true;

    Biome* biome = level->getBiomeSource()->getBiome(xo + 16, zo + 16);
    random.setSeed(level->getSeed());
    int xScale = random.nextInt() / 2 * 2 + 1;
    int zScale = random.nextInt() / 2 * 2 + 1;
    random.setSeed(((xt * xScale) + (zt * zScale)) ^ level->getSeed());

    // 以下所有使用 xo, zo 的地方不变，因为它们已经是 int 范围内的世界坐标
    // 当偏移极大导致 xo/zo 溢出时，这些生成物将出错，但这是保持 32 位生态的已知限制
    // ...（原有生成树、矿石等代码保持不变）
    
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

    if (biome == Biome::forest) forests += oFor + 2;
    if (biome == Biome::rainForest) forests += oFor + 2;
    if (biome == Biome::seasonalForest) forests += oFor + 1;
    if (biome == Biome::taiga) {
		forests += oFor + 1;
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

    for (int i = 0; i < 10; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        ReedsFeature feature;
		feature.place(level, &random, x, y, z);
    }

    int cacti = 0;
    if (biome == Biome::desert) cacti += 5;

    for (int i = 0; i < cacti; i++) {
        int x = xo + random.nextInt(16) + 8;
        int y = random.nextInt(128);
        int z = zo + random.nextInt(16) + 8;
        CactusFeature feature;
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

    // 雪处理
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
    int64_t hashedPos = (xOffs << 32) | (zOffs & 0xffffffff);
    ChunkMap::iterator it = chunkMap.find(hashedPos);
    if (it != chunkMap.end())
        return it->second;

    random.setSeed((long)(xOffs * 341872712l + zOffs * 132899541l));

    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    LevelChunk* levelChunk = new LevelChunk(level, blocks, (int)xOffs, (int)zOffs);
    chunkMap.insert(std::make_pair(hashedPos, levelChunk));

    // ★ 世界坐标改用 double，偏移仅在此叠加一次
    double worldBlockX = xOffs * 16.0 + m_worldOffsetX;
    double worldBlockZ = zOffs * 16.0 + m_worldOffsetZ;

    // 临时截断为 int 传给仍需要 int 的外部接口（当偏移极大时这些会失效，但噪声生成链已 double 化）
    Biome** biomes = level->getBiomeSource()->getBiomeBlock((int)worldBlockX, (int)worldBlockZ, 16, 16);
    float* temperatures = level->getBiomeSource()->temperatures;
    prepareHeights(worldBlockX, worldBlockZ, blocks, 0, temperatures);
    buildSurfaces(worldBlockX, worldBlockZ, blocks, biomes);

    caveFeature.apply(this, level, (int)worldBlockX, (int)worldBlockZ, blocks, LevelChunk::ChunkBlockCount);
    levelChunk->recalcHeightmap();

    return levelChunk;
}

// 修改：x, z 改为 double 世界坐标，y 仍为 int
float* RandomLevelSource::getHeights(float* buffer, double x, int y, double z, int xSize, int ySize, int zSize) {
    float farlandsScale = 1.0f;

    float sx = 684.412f * farlandsScale * m_worldScaleX;
    float sy = 684.412f * farlandsScale * m_worldScaleY;
    float sz = 684.412f * farlandsScale * m_worldScaleZ;

    const int size = xSize * ySize * zSize;
    if (size > MAX_BUFFER_SIZE) {
        LOGI("RandomLevelSource::getHeights: TOO LARGE BUFFER REQUESTED: %d (max %d)\n", size, MAX_BUFFER_SIZE);
    }

    float* temperatures = level->getBiomeSource()->temperatures;
    float* downfalls = level->getBiomeSource()->downfalls;

    // X/Z 偏移已在传入的 worldBlock 中包含，此处仅缩放，不再加偏移
    double noiseX = x / 4.0;
    double noiseY = (y + m_worldOffsetY) / 8.0;   // Y 偏移在此应用（唯一位置）
    double noiseZ = z / 4.0;

    // 为了给 scaleNoise 和 depthNoise 提供 int 输入，我们截断 double
    // 这是 32 位生成器的固有妥协：当坐标超出 int 范围时，噪声将不准确，但不会崩溃
    int intNoiseX = (int)noiseX;
    int intNoiseZ = (int)noiseZ;
    sr = scaleNoise.getRegion(sr, intNoiseX, intNoiseZ, xSize, zSize, 1.121f * m_worldScaleX, 1.121f * m_worldScaleZ, 0.5f);
    dr = depthNoise.getRegion(dr, intNoiseX, intNoiseZ, xSize, zSize, 200.0f * m_worldScaleX, 200.0f * m_worldScaleZ, 0.5f);

    // 主噪声调用：转为 float 喂给 32 位生成器
    float xf = (float)noiseX;
    float yf = (float)noiseY;
    float zf = (float)noiseZ;

    pnr = perlinNoise1.getRegion(pnr, xf, yf, zf, xSize, ySize, zSize, sx / 80.0f, sy / 160.0f, sz / 80.0f);
    ar = lperlinNoise1.getRegion(ar, xf, yf, zf, xSize, ySize, zSize, sx, sy, sz);
    br = lperlinNoise2.getRegion(br, xf, yf, zf, xSize, ySize, zSize, sx, sy, sz);

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

void RandomLevelSource::calcWaterDepths(ChunkSource* parent, int64_t xt, int64_t zt) {
    double worldX = xt * 16.0 + m_worldOffsetX;
    double worldZ = zt * 16.0 + m_worldOffsetZ;
    int xo = (int)worldX;
    int zo = (int)worldZ;
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

Biome::MobList RandomLevelSource::getMobsAt(const MobCategory& mobCategory, int x, int y, int z) {
    BiomeSource* biomeSource = level->getBiomeSource();
    if (biomeSource == NULL) return Biome::MobList();
    Biome* biome = biomeSource->getBiome(x, z);
    if (biome == NULL) return Biome::MobList();
    return biome->getMobs(mobCategory);
}

LevelChunk* PerformanceTestChunkSource::create(int64_t x, int64_t z) {
    unsigned char* blocks = new unsigned char[LevelChunk::ChunkBlockCount];
    memset(blocks, 0, LevelChunk::ChunkBlockCount);
    int xi = (int)x;
    int zi = (int)z;
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
