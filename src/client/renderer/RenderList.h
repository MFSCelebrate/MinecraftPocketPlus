#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__

class RenderChunk;

class RenderList
{
    static const int MAX_NUM_OBJECTS = 1024 * 3;

public:
    RenderList();
    ~RenderList();

    void init(float xOff, float yOff, float zOff);
    void add(int list);
    void addR(const RenderChunk& chunk);
    void next() { ++listIndex; }
    void render();
    void renderChunks();
    void clear();
    void setUseRelativeTranslation(bool use) { m_useRelativeTranslation = use; }

    float xOff, yOff, zOff;          // 兼容旧代码
    int* lists;
    RenderChunk* rlists;
    int listIndex;
    bool inited;
    bool rendered;

private:
    int bufferLimit;
    bool m_useRelativeTranslation;
    double m_camX, m_camY, m_camZ;   // 双精度相机坐标
};

#endif
