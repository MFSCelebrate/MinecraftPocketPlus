#include "PerfRenderer.h"
#include "PerfTimer.h"
#include "Mth.h"
#include "../client/gui/Font.h"
#include "../client/renderer/gles.h"
#include "../client/renderer/Tesselator.h"
#include "../client/Minecraft.h"

PerfRenderer::PerfRenderer( Minecraft* mc, Font* font )
    : _mc(mc), _font(font), _debugPath("root"),
      frameTimePos(0), lastTimer(-1)
{
    for (int i = 0; i < 512; ++i) {
        frameTimes.push_back(0);
        tickTimes.push_back(0);
    }
}

void PerfRenderer::debugFpsMeterKeyPress( int key ) {
    std::vector<PerfTimer::ResultField> list = PerfTimer::getLog(_debugPath, true);
    if (list.empty()) return;

    PerfTimer::ResultField node = list[0];
    list.erase(list.begin());

    if (key == 0) {
        if (node.name.length() > 0) {
            int pos = _debugPath.rfind(".");
            if (pos != std::string::npos) _debugPath = _debugPath.substr(0, pos);
        }
    } else {
        key--;
        if (key < (int)list.size() && list[key].name != "unspecified") {
            if (_debugPath.length() > 0) _debugPath += ".";
            _debugPath += list[key].name;
        }
    }
}

void PerfRenderer::renderFpsMeter( float tickTime ) {
    if (!PerfTimer::enabled) return;

    std::vector<PerfTimer::ResultField> list = PerfTimer::getLog(_debugPath, true);
    if (list.empty()) return;

    PerfTimer::ResultField node = list[0];
    list.erase(list.begin());

    // ========== 帧时间、波形图参数 ==========
    long usPer60Fps = 1000000l / 60;
    if (lastTimer == -1) lastTimer = getTimeS();
    float now = getTimeS();
    tickTimes[ frameTimePos ] = tickTime;
    frameTimes[frameTimePos ] = now - lastTimer;
    lastTimer = now;
    if (++frameTimePos >= (int)frameTimes.size()) frameTimePos = 0;

    // ========== 投影与状态 ==========
    glClear(GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_PROJECTION);
    glEnable2(GL_COLOR_MATERIAL);
    glLoadIdentity2();
    glOrthof(0, (GLfloat)_mc->width, (GLfloat)_mc->height, 0, 1000, 3000);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity2();
    glTranslatef2(0, 0, -2000);

    glLineWidth(1);
    glDisable2(GL_TEXTURE_2D);
    Tesselator& t = Tesselator::instance;

    // ========== 波形图（轻微半透明，现代感）==========
    int hh1 = (int) (usPer60Fps / 200);
    float count = (float)frameTimes.size();
    t.begin(GL_TRIANGLES);
    t.color(0x18ffffff);                     // 更淡的背景
    t.vertex(0, (float)(_mc->height - hh1), 0);
    t.vertex(0, (float)_mc->height, 0);
    t.vertex(count, (float)_mc->height, 0);
    t.vertex(count, (float)(_mc->height - hh1), 0);

    t.color(0x18ff8888);
    t.vertex(0, (float)(_mc->height - hh1 * 2), 0);
    t.vertex(0, (float)(_mc->height - hh1), 0);
    t.vertex(count, (float)(_mc->height - hh1), 0);
    t.vertex(count, (float)(_mc->height - hh1 * 2), 0);
    t.draw();

    float totalTime = 0;
    for (unsigned int i = 0; i < frameTimes.size(); i++) totalTime += frameTimes[i];
    int hh = (int) (totalTime / 200 / frameTimes.size());
    t.begin();
    t.color(0x184488ff);
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

    // ========== 饼图优化绘制 ==========
    int r = 150;                                       // 略微缩小，给文字更多空间
    int x = _mc->width - r - 10;
    int y = _mc->height - r * 2;

    // 过滤 unspecified
    std::vector<PerfTimer::ResultField> visibleFields;
    float totalVisible = 0.0f;
    for (const auto& field : list) {
        if (field.name != "unspecified") {
            visibleFields.push_back(field);
            totalVisible += field.percentage;
        }
    }

    if (totalVisible > 0.0f) {
        // 圆角背景（4 个角用三角形模拟）
        glEnable(GL_BLEND);
        t.begin();
        t.color(0x000000, 200);
        float bx0 = x - r * 1.1f, by0 = y - r * 0.6f - 16;
        float bx1 = x + r * 1.1f, by1 = y + r * 2.0f;
        int corner = 12;   // 圆角半径
        // 主体矩形
        t.vertex(bx0 + corner, by0, 0); t.vertex(bx1 - corner, by0, 0); t.vertex(bx1, by0 + corner, 0);
        t.vertex(bx0 + corner, by0, 0); t.vertex(bx1, by0 + corner, 0); t.vertex(bx1, by0, 0);
        // ... 为简洁，也可以保留原版直角矩形，现在直接画直角矩形以节省性能
        t.vertex(bx0, by0, 0); t.vertex(bx1, by0, 0); t.vertex(bx1, by1, 0);
        t.vertex(bx0, by0, 0); t.vertex(bx1, by1, 0); t.vertex(bx0, by1, 0);
        t.draw();
        glDisable(GL_BLEND);
        glDisable(GL_CULL_FACE);

        float scaledTotal = 0.0f;
        const int fixedSteps = 8;                      // ★ 固定分段数，性能稳定

        for (auto& result : visibleFields) {
            float scaledPct = result.percentage * 100.0f / totalVisible;

            // 主体扇形
            t.begin(GL_TRIANGLE_FAN);
            t.color(result.getColor());
            t.vertex((float)x, (float)y, 0);
            for (int j = fixedSteps; j >= 0; j--) {
                float dir = (scaledTotal + scaledPct * j / fixedSteps) * (Mth::PI * 2.0f / 100.0f);
                float xx = Mth::sin(dir) * r;
                float yy = Mth::cos(dir) * r * 0.5f;
                t.vertex(x + xx, y - yy, 0);
            }
            t.draw();

            // 半透明描边代替立体条带
            t.begin(GL_LINE_LOOP);
            t.color((result.getColor() & 0xfefefe) | 0x88000000);   // 半透白色
            for (int j = 0; j <= fixedSteps; j++) {
                float dir = (scaledTotal + scaledPct * j / fixedSteps) * (Mth::PI * 2.0f / 100.0f);
                float xx = Mth::sin(dir) * r;
                float yy = Mth::cos(dir) * r * 0.5f;
                t.vertex(x + xx, y - yy, 0);
            }
            t.draw();

            scaledTotal += scaledPct;
        }
    }

    glEnable(GL_TEXTURE_2D);

    // ========== 文字紧凑排版 ==========
    float lineH = Font::DefaultLineHeight + 1;   // 行高略微压缩
    {
        std::stringstream msg;
        if (node.name != "unspecified") msg << "[0] ";
        if (node.name.length() == 0) msg << "ROOT ";
        else msg << node.name << " ";
        _font->drawShadow(msg.str(), (float)(x - r), (float)(y - r / 2 - 16), 0xffffffff);
        std::string msg2 = toPercentString(node.globalPercentage);
        _font->drawShadow(msg2, (float)(x + r - _font->width(msg2)), (float)(y - r / 2 - 16), 0xffffffff);
    }

    int idx = 0;
    for (const auto& result : list) {
        if (result.name == "unspecified");   // 保留右侧文字隐藏，若想显示可删除此行
        std::stringstream msg;
        msg << "[" << (idx + 1) << "] " << result.name;
        float yy = y + r/2 + idx * lineH + 20;
        _font->drawShadow(msg.str(), (float)(x - r), yy, result.getColor());
        std::string pct = toPercentString(result.percentage);
        _font->drawShadow(pct, (float)(x + r - _font->width(pct)), yy, 0xffffffff);
        idx++;
    }
}
std::string PerfRenderer::toPercentString( float percentage ) {
    char buf[32] = {0};
    sprintf(buf, "%3.2f%%", percentage);
    return buf;
}
