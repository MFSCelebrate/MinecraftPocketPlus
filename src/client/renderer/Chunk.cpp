#include "Chunk.h"
#include "Tesselator.h"
#include "TileRenderer.h"
#include "culling/Culler.h"
#include "../../world/entity/Entity.h"
#include "../../world/level/tile/Tile.h"
#include "../../world/level/Region.h"
#include "../../world/level/chunk/LevelChunk.h"
#include "../../util/Mth.h"

/*static*/ int Chunk::updates = 0;

Chunk::Chunk(Level* level_, int64_t x, int64_t y, int64_t z, int size, int lists_, GLuint* ptrBuf)
    : level(level_),
      visible(false),
      compiled(false),
      _empty(true),
      xs(size), ys(size), zs(size),
      dirty(false),
      occlusion_visible(true),
      occlusion_querying(false),
      lists(lists_),
      vboBuffers(ptrBuf),
      bb(0,0,0,1,1,1),
      t(Tesselator::instance)
{
    for (int l = 0; l < NumLayers; l++) {
        empty[l] = false;
    }

    radius = Mth::sqrt((float)(xs * xs + ys * ys + zs * zs)) * 0.5f;

    this->x = -999;
    setPos(x, y, z);
}

void Chunk::setPos(int64_t x, int64_t y, int64_t z)
{
    if (x == this->x && y == this->y && z == this->z) return;

    reset();
    this->x = x;
    this->y = y;
    this->z = z;
    xm = x + xs / 2;
    ym = y + ys / 2;
    zm = z + zs / 2;

    const float xzg = 1.0f;
    const float yp = 2.0f;
    const float yn = 0.0f;
    bb.set((float)(x - xzg), (float)(y - yn), (float)(z - xzg),
           (float)(x + xs + xzg), (float)(y + ys + yp), (float)(z + zs + xzg));

    setDirty();
}

void Chunk::translateToPos()
{
    glTranslatef2((float)x, (float)y, (float)z);
}

void Chunk::rebuild()
{
    if (!dirty) return;
    updates++;

    int64_t x0 = x;
    int64_t y0 = y;
    int64_t z0 = z;
    int64_t x1 = x + xs;
    int64_t y1 = y + ys;
    int64_t z1 = z + zs;
    for (int l = 0; l < NumLayers; l++) {
        empty[l] = true;
    }
    _empty = true;

    LevelChunk::touchedSky = false;

    int r = 1;
    Region region(level, x0 - r, y0 - r, z0 - r, x1 + r, y1 + r, z1 + r);
    TileRenderer tileRenderer(&region);

    bool doRenderLayer[NumLayers] = {true, false, false};
    for (int l = 0; l < NumLayers; l++) {
        if (!doRenderLayer[l]) continue;
        bool renderNextLayer = false;
        bool rendered = false;
        bool started = false;
        int cindex = -1;

        for (int64_t y = y0; y < y1; y++) {
            for (int64_t z = z0; z < z1; z++) {
                for (int64_t x = x0; x < x1; x++) {
                    ++cindex;
                    int tileId = region.getTile(x, y, z);
                    if (tileId > 0) {
                        if (!started) {
                            started = true;

#ifndef USE_VBO
                            glNewList(lists + l, GL_COMPILE);
                            glPushMatrix2();
                            translateToPos();
                            float ss = 1.000001f;
                            glTranslatef2(-zs / 2.0f, -ys / 2.0f, -zs / 2.0f);
                            glScalef2(ss, ss, ss);
                            glTranslatef2(zs / 2.0f, ys / 2.0f, zs / 2.0f);
#endif
                            t.begin();
                            t.offset((double)(-this->x), (double)(-this->y), (double)(-this->z));
                        }

                        Tile* tile = Tile::tiles[tileId];
                        int renderLayer = tile->getRenderLayer();

                        if (renderLayer > l) {
                            renderNextLayer = true;
                            doRenderLayer[renderLayer] = true;
                        } else if (renderLayer == l) {
                            rendered |= tileRenderer.tesselateInWorld(tile, (int)x, (int)y, (int)z);
                        }
                    }
                }
            }
        }

        if (started) {
#ifdef USE_VBO
            renderChunk[l] = t.end(true, vboBuffers[l]);
            renderChunk[l].pos.x = (float)this->x;
            renderChunk[l].pos.y = (float)this->y;
            renderChunk[l].pos.z = (float)this->z;
            renderChunk[l].baseX = this->x;
            renderChunk[l].baseY = this->y;
            renderChunk[l].baseZ = this->z;
#else
            t.end(false, -1);
            glPopMatrix2();
            glEndList();
#endif
            t.offset(0, 0, 0);
        } else {
            rendered = false;
        }
        if (rendered) {
            empty[l] = false;
            _empty = false;
        }
        if (!renderNextLayer) break;
    }

    skyLit = LevelChunk::touchedSky;
    compiled = true;
    return;
}

float Chunk::distanceToSqr(const Entity* player) const
{
    float xd = (float)(player->x - xm);
    float yd = (float)(player->y - ym);
    float zd = (float)(player->z - zm);
    return xd * xd + yd * yd + zd * zd;
}

float Chunk::squishedDistanceToSqr(const Entity* player) const
{
    float xd = (float)(player->x - xm);
    float yd = (float)(player->y - ym) * 2;
    float zd = (float)(player->z - zm);
    return xd * xd + yd * yd + zd * zd;
}

void Chunk::reset()
{
    for (int i = 0; i < NumLayers; i++) {
        empty[i] = true;
    }
    visible = false;
    compiled = false;
    _empty = true;
}

int Chunk::getList(int layer)
{
    if (!visible) return -1;
    if (!empty[layer]) return lists + layer;
    return -1;
}

RenderChunk& Chunk::getRenderChunk(int layer)
{
    return renderChunk[layer];
}

int Chunk::getAllLists(int displayLists[], int p, int layer)
{
    if (!visible) return p;
    if (!empty[layer]) displayLists[p++] = (lists + layer);
    return p;
}

void Chunk::cull(Culler* culler)
{
    visible = culler->isVisible(bb);
}

void Chunk::renderBB()
{
    // glCallList(lists + 2);
}

bool Chunk::isEmpty()
{
    return compiled && _empty;
}

void Chunk::setDirty()
{
    dirty = true;
}

void Chunk::setClean()
{
    dirty = false;
}

bool Chunk::isDirty()
{
    return dirty;
}

void Chunk::resetUpdates()
{
    updates = 0;
}
