#include "RenderList.h"
#include "gles.h"
#include "RenderChunk.h"
#include "Tesselator.h"

RenderList::RenderList()
    : mergedVBO(0), mergedCapacity(0), supportsMerge(false),
      inited(false), rendered(false), m_useRelativeTranslation(false),
      m_camX(0.0), m_camY(0.0), m_camZ(0.0)
{
    lists = new int[MAX_NUM_OBJECTS];
    rlists = new RenderChunk[MAX_NUM_OBJECTS];
    for (int i = 0; i < MAX_NUM_OBJECTS; ++i)
        rlists[i].vboId = -1;
    initMerged();
}

RenderList::~RenderList() {
    delete[] lists;
    delete[] rlists;
}

void RenderList::init(double xOff, double yOff, double zOff) {
    inited = true;
    listIndex = 0;
    this->xOff = xOff;          // xOff 改为 double 成员（原本是 float？检查一下：RenderList 有 float xOff, yOff, zOff 成员。需要同步改为 double）
    this->yOff = yOff;
    this->zOff = zOff;
    m_camX = xOff;
    m_camY = yOff;
    m_camZ = zOff;
}

void RenderList::initMerged() {
    const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
    supportsMerge = (extensions && strstr(extensions, "GL_OES_mapbuffer") != nullptr);
    LOGI("RenderList: mapbuffer support = %d", supportsMerge);
}

void RenderList::renderMerged(int layer) {
    if (!supportsMerge) {
        render();  // 回退到原版绘制
        return;
    }
    if (!inited || bufferLimit == 0) return;

    const int stride = sizeof(VERTEX);
    int totalVertices = 0;
    for (int i = 0; i < bufferLimit; ++i) {
        totalVertices += rlists[i].vertexCount;
    }
    if (totalVertices == 0) return;

    // 确保 VBO 容量足够
    int neededBytes = totalVertices * stride;
    if (mergedVBO == 0) {
        glGenBuffers(1, &mergedVBO);
    }
    if (neededBytes > mergedCapacity) {
        mergedCapacity = neededBytes * 1.5;   // 留余量
        glBindBuffer(GL_ARRAY_BUFFER, mergedVBO);
        glBufferData(GL_ARRAY_BUFFER, mergedCapacity, nullptr, GL_STREAM_DRAW);
    } else {
        glBindBuffer(GL_ARRAY_BUFFER, mergedVBO);
    }

    // 映射合并后 VBO 的内存
    VERTEX* dst = (VERTEX*)glMapBufferOES(GL_ARRAY_BUFFER, GL_WRITE_ONLY_OES);
    if (!dst) {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        render();   // 映射失败回退
        return;
    }

    // 逐区块复制，并应用世界偏移
    int offset = 0;
    for (int i = 0; i < bufferLimit; ++i) {
        RenderChunk& rc = rlists[i];
        glBindBuffer(GL_ARRAY_BUFFER, rc.vboId);
        VERTEX* src = (VERTEX*)glMapBufferOES(GL_ARRAY_BUFFER, GL_READ_ONLY_OES);
        if (!src) {   // 个别区块映射失败则跳过
            glUnmapBufferOES(GL_ARRAY_BUFFER);
            continue;
        }
        float dx = rc.baseX - m_camX;
        float dy = rc.baseY - m_camY;
        float dz = rc.baseZ - m_camZ;
        for (int v = 0; v < rc.vertexCount; ++v) {
            dst[offset + v] = src[v];
            dst[offset + v].x += dx;
            dst[offset + v].y += dy;
            dst[offset + v].z += dz;
        }
        glUnmapBufferOES(GL_ARRAY_BUFFER);
        offset += rc.vertexCount;
    }
    glUnmapBufferOES(GL_ARRAY_BUFFER);   // 提交合并数据

    // 设置顶点属性
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    glVertexPointer(3, GL_FLOAT, stride, 0);
    glTexCoordPointer(2, GL_FLOAT, stride, (const void*) (3*sizeof(float)));
    glColorPointer(4, GL_UNSIGNED_BYTE, stride, (const void*) (5*sizeof(float)));

    glDrawArrays(GL_TRIANGLES, 0, totalVertices);

    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void RenderList::add(int list) {
    lists[listIndex] = list;
    if (listIndex == MAX_NUM_OBJECTS) render();
}

void RenderList::addR(const RenderChunk& chunk) {
    rlists[listIndex] = chunk;
}

void RenderList::render() {
    if (!inited) return;
    if (!rendered) {
        bufferLimit = listIndex;
        listIndex = 0;
        rendered = true;
    }
    if (listIndex < bufferLimit) {
        glPushMatrix2();
        if (!m_useRelativeTranslation) {
            glTranslatef2((float)-xOff, (float)-yOff, (float)-zOff);
        }
#ifndef USE_VBO
        glCallLists(bufferLimit, GL_UNSIGNED_INT, lists);
#else
        renderChunks();
#endif
        glPopMatrix2();
    }
}

void RenderList::renderChunks() {
    glEnableClientState2(GL_VERTEX_ARRAY);
    glEnableClientState2(GL_COLOR_ARRAY);
    glEnableClientState2(GL_TEXTURE_COORD_ARRAY);

    const int Stride = VertexSizeBytes;

    for (int i = 0; i < bufferLimit; ++i) {
        RenderChunk& rc = rlists[i];

        glPushMatrix2();

        if (m_useRelativeTranslation) {
            // 相对平移：区块世界坐标减去相机位置
            double transX = (double)rc.baseX - m_camX;
            double transY = (double)rc.baseY - m_camY;
            double transZ = (double)rc.baseZ - m_camZ;
            glTranslatef2((float)transX, (float)transY, (float)transZ);
        } else {
            // 传统模式：直接平移区块到世界位置（顶点是局部坐标）
            glTranslatef2(rc.pos.x, rc.pos.y, rc.pos.z);
        }

        glBindBuffer2(GL_ARRAY_BUFFER, rc.vboId);
        glVertexPointer2(3, GL_FLOAT, Stride, 0);
        glTexCoordPointer2(2, GL_FLOAT, Stride, (GLvoid*)(3 * 4));
        glColorPointer2(4, GL_UNSIGNED_BYTE, Stride, (GLvoid*)(5 * 4));
        glDrawArrays2(GL_TRIANGLES, 0, rc.vertexCount);

        glPopMatrix2();
    }

    glDisableClientState2(GL_VERTEX_ARRAY);
    glDisableClientState2(GL_COLOR_ARRAY);
    glDisableClientState2(GL_TEXTURE_COORD_ARRAY);
}

void RenderList::clear() {
    inited = false;
    rendered = false;
}
