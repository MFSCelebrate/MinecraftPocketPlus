#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__

#include "gles.h"
#include <cstring>   // for strstr

class RenderChunk;

class RenderList
{
    static const int MAX_NUM_OBJECTS = 1024 * 3;

public:
    RenderList();
    ~RenderList();

    void init(double xOff, double yOff, double zOff);
    void add(int list);
    void addR(const RenderChunk& chunk);
    void next() { ++listIndex; }
    void render();
    void renderChunks();
    void clear();
    void setUseRelativeTranslation(bool use) { m_useRelativeTranslation = use; }

    double xOff, yOff, zOff;          // 兼容旧代码
    int* lists;
    RenderChunk* rlists;
    int listIndex;
    bool inited;
    bool rendered;

// 新增合并渲染相关
    bool supportsMerge;            // 是否支持 glMapBuffer
    GLuint mergedVBO;             // 超大一维 VBO 句柄
    int mergedCapacity;           // 当前 VBO 字节容量
    void renderMerged(int layer); // 合并绘制方法
    void initMerged();            // 初始化检查扩展
private:
    int bufferLimit;
    bool m_useRelativeTranslation;
    double m_camX, m_camY, m_camZ;   // 双精度相机坐标
};

#endif
