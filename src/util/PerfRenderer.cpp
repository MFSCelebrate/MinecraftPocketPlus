#include "PerfRenderer.h"
#include "PerfTimer.h"

#include "Mth.h"
#include "../client/gui/Font.h"
#include "../client/renderer/gles.h"
#include "../client/renderer/Tesselator.h"
#include "../client/Minecraft.h"

PerfRenderer::PerfRenderer( Minecraft* mc, Font* font )
:   _mc(mc),
	_font(font),
	_debugPath("root"),
	frameTimePos(0),
	lastTimer(-1)
{
	for (int i = 0; i < 512; ++i) {
		frameTimes.push_back(0);
		tickTimes.push_back(0);
	}
}

void PerfRenderer::debugFpsMeterKeyPress(int key) {
    std::vector<PerfTimer::ResultField> list = PerfTimer::getLog(_debugPath);
    if (list.empty()) return;

    if (key == 0) {
        // 如果在根节点，则切换饼图显示
        if (_debugPath == "root") {
            m_pieVisible = !m_pieVisible;
            return;
        }
        // 否则向上一级
        PerfTimer::ResultField node = list[0];
        if (node.name.length() > 0) {
            int pos = _debugPath.rfind(".");
            if (pos != std::string::npos) _debugPath = _debugPath.substr(0, pos);
        }
    } else {
        // 数字 1-9 进入子项
        key--;
        if (key < (int)list.size() && list[key].name != "unspecified") {
            if (_debugPath.length() > 0) _debugPath += ".";
            _debugPath += list[key].name;
        }
    }
}

void PerfRenderer::renderFpsMeter(float tickTime) {
    std::vector<PerfTimer::ResultField> list = PerfTimer::getLog(_debugPath);
    if (list.empty()) return;

    PerfTimer::ResultField node = list[0];
    list.erase(list.begin());

    // ---------- 帧时间记录 ----------
    long usPer60Fps = 1000000l / 60;
    if (lastTimer == -1) lastTimer = getTimeS();
    float now = getTimeS();
    tickTimes[ frameTimePos ] = tickTime;
    frameTimes[frameTimePos ] = now - lastTimer;
    lastTimer = now;
    if (++frameTimePos >= (int)frameTimes.size()) frameTimePos = 0;

    // ---------- 准备绘制环境 ----------
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glEnable(GL_COLOR_MATERIAL);
    glLoadIdentity();
    glOrthof(0, (GLfloat)_mc->width, (GLfloat)_mc->height, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(0, 0, -2000);

    // ---------- 绘制 FPS 波形图 ----------
glLineWidth(1);
glDisable(GL_TEXTURE_2D);
Tesselator& t = Tesselator::instance;

t.begin(GL_TRIANGLES);
int hh1 = (int) (usPer60Fps / 200);
float count = (float)frameTimes.size();
t.color(0x20000000);
t.vertex(0, (float)(_mc->height - hh1), 0);
t.vertex(0, (float)_mc->height, 0);
t.vertex(count, (float)_mc->height, 0);
t.vertex(count, (float)(_mc->height - hh1), 0);

t.color(0x20200000);
t.vertex(0, (float)(_mc->height - hh1 * 2), 0);
t.vertex(0, (float)(_mc->height - hh1), 0);
t.vertex(count, (float)(_mc->height - hh1), 0);
t.vertex(count, (float)(_mc->height - hh1 * 2), 0);

t.draw();
float totalTime = 0;
for (unsigned int i = 0; i < frameTimes.size(); i++) {
    totalTime += frameTimes[i];
}
int hh = (int) (totalTime / 200 / frameTimes.size());
t.begin();
t.color(0x20400000);
t.vertex(0, (float)(_mc->height - hh), 0);
t.vertex(0, (float)_mc->height, 0);
t.vertex(count, (float)_mc->height, 0);
t.vertex(count, (float)(_mc->height - hh), 0);
t.draw();

t.begin(GL_LINES);
for (unsigned int i = 0; i < frameTimes.size(); i++) {
    int col = ((i - frameTimePos) & (frameTimes.size() - 1)) * 255 / frameTimes.size();
    int cc = col * col / 255;
    cc = cc * cc / 255;
    int cc2 = cc * cc / 255;
    cc2 = cc2 * cc2 / 255;
    if (frameTimes[i] > usPer60Fps) {
        t.color(0xff000000 + cc * 65536);
    } else {
        t.color(0xff000000 + cc * 256);
    }

    float time = 10 * 1000 * frameTimes[i] / 200;
    float time2 = 10 * 1000 * tickTimes[i] / 200;

    t.vertex(i + 0.5f, _mc->height - time + 0.5f, 0);
    t.vertex(i + 0.5f, _mc->height + 0.5f, 0);

    t.color(0xff000000 + cc * 65536 + cc * 256 + cc * 1);
    t.vertex(i + 0.5f, _mc->height - time + 0.5f, 0);
    t.vertex(i + 0.5f, _mc->height - (time - time2) + 0.5f, 0);
}
t.draw();
    // ================= 饼图优化部分 =================
    if (!m_pieVisible) {
        // 只绘制文字，不画饼图，不画圆背景
        drawTextLegend(node, list);
        return;
    }

    // 每4帧重新生成饼图几何体，其他帧复用VBO
    static int frameCounter = 0;
    frameCounter++;

    bool needsRebuild = false;
    if (m_pieVbo == 0 || (frameCounter - m_lastFrameUpdated) >= 4) {
        needsRebuild = true;
    }

    // 检查数据是否变化（可选，若不想每4帧强制重绘可注释）
    std::vector<float> currentPcts;
    for (auto& f : list) currentPcts.push_back(f.percentage);
    if (currentPcts != m_cachedPercentages) {
        needsRebuild = true;
        m_cachedPercentages = currentPcts;
    }

    if (needsRebuild) {
        // 重新生成VBO
        if (m_pieVbo != 0) {
            glDeleteBuffers(1, &m_pieVbo);
        }
        glGenBuffers(1, &m_pieVbo);

        Tesselator& t2 = Tesselator::instance; // 或者直接用同一个？
        // 但为保证状态，我们用独立的 Tesselator 或者直接使用当前 t
        t.begin();
        float totalPercentage = 0;
        const int pieSteps = 8;                    // 固定分段数
        const float radius = 120.0f;
        float cx = _mc->width - radius - 10;
        float cy = _mc->height - radius * 2;
        const float angleConv = Mth::PI * 2.0f / 100.0f;

        for (size_t i = 0; i < list.size(); ++i) {
            PerfTimer::ResultField& result = list[i];
            float segmentAngle = result.percentage * angleConv;
            int steps = pieSteps;   // 直接固定

            // 扇形主体
            for (int j = 0; j < steps; ++j) {
                float a1 = totalPercentage * angleConv + segmentAngle * (j / (float)steps);
                float a2 = totalPercentage * angleConv + segmentAngle * ((j+1) / (float)steps);
                t.color(result.getColor());
                t.vertex(cx, cy, 0);
                t.vertex(cx + sin(a1)*radius, cy - cos(a1)*radius*0.5f, 0);
                t.vertex(cx + sin(a2)*radius, cy - cos(a2)*radius*0.5f, 0);
            }

            // 颜色条带（简单矩形表示，也可省略）
            // 这里简化：画一个很小的三角条表示三维效果，但不重要，可省略以提速
            totalPercentage += result.percentage;
        }
        // 结束
        RenderChunk rc = t.end(true, m_pieVbo);
        m_pieVertexCount = rc.vertexCount;
        m_lastFrameUpdated = frameCounter;
    }

    // 绘制VBO（如果存在）
    if (m_pieVbo != 0 && m_pieVertexCount > 0) {
        glEnableClientState(GL_VERTEX_ARRAY);
        // 我们生成的顶点包含位置和颜色，需要设置对应指针
        // 但 t.begin() 默认生成的是顶点+颜色+纹理坐标？取决于模式。
        // 为简单，我们在重建时只用了颜色和位置，纹理坐标未设置，所以使用 VTC 方式：
        glBindBuffer(GL_ARRAY_BUFFER, m_pieVbo);
        glVertexPointer(3, GL_FLOAT, sizeof(VERTEX), 0);
        glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(VERTEX), (void*)(5*4));
        glEnableClientState(GL_COLOR_ARRAY);
        glDrawArrays(GL_TRIANGLES, 0, m_pieVertexCount);
        glDisableClientState(GL_COLOR_ARRAY);
        glDisableClientState(GL_VERTEX_ARRAY);
    }

    // 绘制文字（始终绘制）
    drawTextLegend(node, list);
}

void PerfRenderer::drawTextLegend(const PerfTimer::ResultField& node,
                                  const std::vector<PerfTimer::ResultField>& list) {
    int r = 160;
    int x = _mc->width - r - 10;
    int y = _mc->height - r * 2;

    glEnable(GL_TEXTURE_2D);
    // 绘制标题
    {
        std::stringstream msg;
        if (node.name != "unspecified") msg << "[0] ";
        if (node.name.empty()) msg << "ROOT ";
        else msg << node.name << " ";
        int col = 0xffffff;
        _font->drawShadow(msg.str(), (float)(x - r), (float)(y - r/2 - 16), col);
        std::string msg2 = toPercentString(node.globalPercentage);
        _font->drawShadow(msg2, (float)(x + r - _font->width(msg2)), (float)(y - r/2 - 16), col);
    }

    // 子项列表
    for (size_t i = 0; i < list.size(); i++) {
        const PerfTimer::ResultField& result = list[i];
        std::stringstream msg;
        if (result.name != "unspecified") msg << "[" << (i+1) << "] ";
        else msg << "[?] ";
        msg << result.name;
        float xx = (float)(x - r);
        float yy = (float)(y + r/2 + i * 8 + 20);
        _font->drawShadow(msg.str(), xx, yy, result.getColor());
        std::string msg2 = toPercentString(result.percentage);
        _font->drawShadow(msg2, xx - 50 - _font->width(msg2), yy, result.getColor());
        msg2 = toPercentString(result.globalPercentage);
        _font->drawShadow(msg2, xx - _font->width(msg2), yy, result.getColor());
    }
}

std::string PerfRenderer::toPercentString( float percentage )
{
	char buf[32] = {0};
	sprintf(buf, "%3.2f%%", percentage);
	return buf;
}
