// PerfRenderer.cpp (完整版本，可直接编译)
#include "PerfRenderer.h"
#include "PerfTimer.h"
#include "Mth.h"
#include "../client/gui/Font.h"
#include "../client/renderer/gles.h"
#include "../client/renderer/Tesselator.h"
#include "../client/Minecraft.h"

extern bool gPerfTimerEnabled;

PerfRenderer::PerfRenderer( Minecraft* mc, Font* font )
    : _mc(mc), _font(font), _debugPath("root"),
      frameTimePos(0), lastTimer(-1),
      m_pieVisible(false)
{
    for (int i = 0; i < 512; ++i) {
        frameTimes.push_back(0);
        tickTimes.push_back(0);
    }
    gPerfTimerEnabled = false;
}

void PerfRenderer::togglePie() {
    m_pieVisible = !m_pieVisible;
    gPerfTimerEnabled = m_pieVisible;
    if (!m_pieVisible) {
        PerfTimer::reset();   // 关闭时清理剖析栈，防止下次开启崩溃
    }
}

void PerfRenderer::navigateBack() {
    if (_debugPath == "root") return;
    size_t pos = _debugPath.rfind(".");
    if (pos != std::string::npos)
        _debugPath = _debugPath.substr(0, pos);
    else
        _debugPath = "root";
}

void PerfRenderer::debugFpsMeterKeyPress(int key) {
    std::vector<PerfTimer::ResultField> list = PerfTimer::getLog(_debugPath);

    // 0 键在任何情况下都直接切换饼图/剖析器
    if (key == 0) {
        togglePie();
        return;
    }

    // 1-9 键需要剖析器已经激活并有数据
    if (list.empty()) return;

    list.erase(list.begin());   // 移除当前节点

    int index = key - 1;
    if (index < (int)list.size() && list[index].name != "unspecified") {
        if (!_debugPath.empty()) _debugPath += ".";
        _debugPath += list[index].name;
    }
}
void PerfRenderer::renderFpsMeter(float tickTime) {
    std::vector<PerfTimer::ResultField> list = PerfTimer::getLog(_debugPath);
    if (list.empty() || !m_pieVisible) return;   // 隐藏时不绘制

    // 保存当前节点信息（用于文字标题）
    PerfTimer::ResultField node = list[0];
    list.erase(list.begin());   // 移除当前节点

    // ---- 帧时间、波形图等完全不变的代码 ----
    long usPer60Fps = 1000000l / 60;
    if (lastTimer == -1) lastTimer = getTimeS();
    float now = getTimeS();
    tickTimes[ frameTimePos ] = tickTime;
    frameTimes[frameTimePos ] = now - lastTimer;
    lastTimer = now;
    if (++frameTimePos >= (int)frameTimes.size()) frameTimePos = 0;

    // 投影与状态设置
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glEnable2(GL_COLOR_MATERIAL);
    glLoadIdentity2();
    glOrthof(0, (GLfloat)_mc->width, (GLfloat)_mc->height, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity2();
    glTranslatef2(0, 0, -2000);

    // ---------- FPS 波形图（保持不变） ----------
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

    // ---------- 饼图 ----------
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

    float totalPercentage = 0;
    for (unsigned int i = 0; i < list.size(); i++) {
        PerfTimer::ResultField& result = list[i];
        int steps = Mth::floor(result.percentage / 4) + 1;

        t.begin(GL_TRIANGLE_FAN);
        t.color(result.getColor());
        t.vertex((float)x, (float)y, 0);
        for (int j = steps; j >= 0; j--) {
            float dir = (float) ((totalPercentage + (result.percentage * j / steps)) * Mth::PI * 2 / 100);
            float xx = Mth::sin(dir) * r;
            float yy = Mth::cos(dir) * r * 0.5f;
            t.vertex(x + xx, y - yy, 0);
        }
        t.draw();
        t.begin(GL_TRIANGLE_STRIP);
        t.color((result.getColor() & 0xfefefe) >> 1);
        for (int j = steps; j >= 0; j--) {
            float dir = (float) ((totalPercentage + (result.percentage * j / steps)) * Mth::PI * 2 / 100);
            float xx = Mth::sin(dir) * r;
            float yy = Mth::cos(dir) * r * 0.5f;
            t.vertex(x + xx, y - yy, 0);
            t.vertex(x + xx, y - yy + 10, 0);
        }
        t.draw();
        totalPercentage += result.percentage;
    }

    glEnable(GL_TEXTURE_2D);

    // ---------- 文字标题与列表 ----------
    {
        std::stringstream msg;
        if (node.name != "unspecified") msg << "[0] ";
        if (node.name.length() == 0) msg << "ROOT ";
        else msg << node.name << " ";
        _font->drawShadow(msg.str(), (float)(x - r), (float)(y - r / 2 - 16), 0xffffff);
        std::string msg2 = toPercentString(node.globalPercentage);
        _font->drawShadow(msg2, (float)(x + r - _font->width(msg2)), (float)(y - r / 2 - 16), 0xffffff);
    }

    for (unsigned int i = 0; i < list.size(); i++) {
        PerfTimer::ResultField& result = list[i];
        std::stringstream msg;
        if (result.name != "unspecified") msg << "[" << (i + 1) << "] ";
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

std::string PerfRenderer::toPercentString( float percentage ) {
    char buf[32] = {0};
    sprintf(buf, "%3.2f%%", percentage);
    return buf;
}
