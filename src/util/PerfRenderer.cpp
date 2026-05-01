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

    // ---------- 帧时间、波形图参数 ----------
    long usPer60Fps = 1000000l / 60;
    if (lastTimer == -1) lastTimer = getTimeS();
    float now = getTimeS();
    tickTimes[ frameTimePos ] = tickTime;
    frameTimes[frameTimePos ] = now - lastTimer;
    lastTimer = now;
    if (++frameTimePos >= (int)frameTimes.size()) frameTimePos = 0;

    // ---------- 投影 ----------
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glEnable2(GL_COLOR_MATERIAL);
    glLoadIdentity2();
    glOrthof(0, (GLfloat)_mc->width, (GLfloat)_mc->height, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity2();
    glTranslatef2(0, 0, -2000);

    // ---------- 底部 FPS 波形图（始终保持） ----------
    glLineWidth(1);
    glDisable2(GL_TEXTURE_2D);
    Tesselator& t = Tesselator::instance;

    {
        int hh1 = (int) (usPer60Fps / 200);
        float count = (float)frameTimes.size();
        t.begin(GL_TRIANGLES);
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
        for (unsigned int i = 0; i < frameTimes.size(); i++) totalTime += frameTimes[i];
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
            if (frameTimes[i] > usPer60Fps) t.color(0xff000000 + cc * 65536);
            else t.color(0xff000000 + cc * 256);

            float time = 10 * 1000 * frameTimes[i] / 200;
            float time2 = 10 * 1000 * tickTimes[i] / 200;
            t.vertex(i + 0.5f, _mc->height - time + 0.5f, 0);
            t.vertex(i + 0.5f, _mc->height + 0.5f, 0);
            t.color(0xff000000 + cc * 65536 + cc * 256 + cc * 1);
            t.vertex(i + 0.5f, _mc->height - time + 0.5f, 0);
            t.vertex(i + 0.5f, _mc->height - (time - time2) + 0.5f, 0);
        }
        t.draw();
    }

    // ---------- 饼图与文字区域（由 m_pieVisible 控制） ----------
    if (!m_pieVisible) {
        // 只显示当前节点标题（保留简洁的性能信息）
        drawTextLegend(node, list);
        return;
    }

    // 背景矩形
    int r = 160;
    int x = _mc->width - r - 10;
    int y = _mc->height - r * 2;
    glEnable(GL_BLEND);
    t.begin();
    t.color(0x000000, 200);
    t.vertex(x - r * 1.1f, y - r * 0.6f - 16, 0);
    t.vertex(x - r * 1.1f, y + r * 2.0f, 0);
    t.vertex(x + r * 1.1f, y + r * 2.0f, 0);
    t.vertex(x + r * 1.1f, y - r * 0.6f - 16, 0);
    t.draw();
    glDisable(GL_BLEND);

    glDisable(GL_CULL_FACE);

    // 更新饼图缓存（每4帧或数据变化时重建）
    static int frameSkip = 0;
    frameSkip++;
    std::vector<float> currentPcts;
    for (auto& f : list) currentPcts.push_back(f.percentage);
    if (m_pieVbo == 0 || (frameSkip % 4 == 0) || currentPcts != m_cachedPercentages) {
        if (m_pieVbo != 0) glDeleteBuffers(1, &m_pieVbo);
        glGenBuffers(1, &m_pieVbo);
        m_cachedPercentages = currentPcts;

        t.begin();
        float totalPercentage = 0;
        const int pieSteps = 8;          // 固定分段数，三角形数量适中
        for (size_t i = 0; i < list.size(); i++) {
            PerfTimer::ResultField& result = list[i];
            float segmentAngle = result.percentage * (Mth::PI * 2.0f / 100.0f);

            // 扇形主体
            t.color(result.getColor());
            for (int j = 0; j < pieSteps; ++j) {
                float a1 = totalPercentage * (Mth::PI * 2.0f / 100.0f) + segmentAngle * (j / (float)pieSteps);
                float a2 = totalPercentage * (Mth::PI * 2.0f / 100.0f) + segmentAngle * ((j+1) / (float)pieSteps);
                t.vertex((float)x, (float)y, 0);
                t.vertex(x + sin(a1) * r, y - cos(a1) * r * 0.5f, 0);
                t.vertex(x + sin(a2) * r, y - cos(a2) * r * 0.5f, 0);
            }

            // 颜色条带（可选，保留原版立体感）
            t.color((result.getColor() & 0xfefefe) >> 1);
            for (int j = 0; j < pieSteps; ++j) {
                float a1 = totalPercentage * (Mth::PI * 2.0f / 100.0f) + segmentAngle * (j / (float)pieSteps);
                float a2 = totalPercentage * (Mth::PI * 2.0f / 100.0f) + segmentAngle * ((j+1) / (float)pieSteps);
                t.vertex(x + sin(a1) * r, y - cos(a1) * r * 0.5f, 0);
                t.vertex(x + sin(a1) * r, y - cos(a1) * r * 0.5f + 10, 0);
                t.vertex(x + sin(a2) * r, y - cos(a2) * r * 0.5f, 0);
            }

            totalPercentage += result.percentage;
        }
        RenderChunk rc = t.end(true, m_pieVbo);
        m_pieVertexCount = rc.vertexCount;
    }

    // 绘制饼图VBO
    if (m_pieVbo != 0 && m_pieVertexCount > 0) {
        glEnableClientState2(GL_VERTEX_ARRAY);
        glEnableClientState2(GL_COLOR_ARRAY);
        glBindBuffer2(GL_ARRAY_BUFFER, m_pieVbo);
        glVertexPointer2(3, GL_FLOAT, sizeof(VERTEX), 0);
        glColorPointer2(4, GL_UNSIGNED_BYTE, sizeof(VERTEX), (GLvoid*)(5 * 4));
        glDrawArrays2(GL_TRIANGLES, 0, m_pieVertexCount);
        glDisableClientState2(GL_COLOR_ARRAY);
        glDisableClientState2(GL_VERTEX_ARRAY);
    }

    glEnable(GL_TEXTURE_2D);
    // 文字始终显示（会被 pieVisible 控制，所以只有开启时才会调用）
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
