#ifndef NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__
#define NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__

//package net.minecraft.client.renderer;

class RenderChunk;

class RenderList
{
	static const int MAX_NUM_OBJECTS = 1024 * 3;

public:
    void setUseRelativeTranslation(bool use) { m_useRelativeTranslation = use; }
	RenderList();
	~RenderList();

    void init(float xOff, float yOff, float zOff);

	void add(int list);
	void addR(const RenderChunk& chunk);

	__inline void next() { ++listIndex; }

    void render();
	void renderChunks();

    void clear();


	float xOff, yOff, zOff;
	int* lists;
	RenderChunk* rlists;

	int listIndex;
	bool inited;
	bool rendered;

private:
	int bufferLimit;
    bool m_useRelativeTranslation;   // 新增
};

#endif /*NET_MINECRAFT_CLIENT_RENDERER__RenderList_H__*/
