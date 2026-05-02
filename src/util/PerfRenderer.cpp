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
    _workerRunning = true;
    _worker = std::thread(&PerfRenderer::workerLoop, this);
}

PerfRenderer::~PerfRenderer() {
    _workerRunning = false;
    _cv.notify_one();
    if (_worker.joinable()) _worker.join();
}

void PerfRenderer::workerLoop() {
    while (true) {
        std::unique_lock<std::mutex> lock(_mutex);
        _cv.wait(lock, [this]{ return _needsUpdate.load() || !_workerRunning.load(); });
        if (!_workerRunning) break;
        _needsUpdate = false;
        lock.unlock();   // 解锁后再进行耗时计算

        // 深拷贝整个 times 表，完全线程安全
        PerfTimer::TimeMap timesCopy = PerfTimer::getTimesCopy();

        // 利用拷贝后的数据计算当前路径的饼图数据
        std::vector<PerfTimer::ResultField> tempList;
        PerfTimer::ResultField tempNode("", 0, 0);

        // ---------- 复刻 getLog 核心逻辑（但不修改原表） ----------
        std::string path = _debugPath;
        auto itRoot = timesCopy.find("root");
        float globalTime = (itRoot != timesCopy.end())? itRoot->second : 0;
        auto itPath = timesCopy.find(path);
        float totalTime2 = (itRoot != timesCopy.end())? itRoot->second : -1;
        std::vector<PerfTimer::ResultField> result;
        if (path.length() > 0) path += ".";
        float totalTime = 0;
        for (auto& kv : timesCopy) {
            const std::string& key = kv.first;
            if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
                totalTime += kv.second;
            }
        }
        float oldTime = totalTime;
        if (totalTime < totalTime2) totalTime = totalTime2;
        if (globalTime < totalTime) globalTime = totalTime;
        for (auto& kv : timesCopy) {
            const std::string& key = kv.first;
            if (key.length() > path.length() && Util::startsWith(key, path) && key.find(".", path.length() + 1) == std::string::npos) {
                float time = kv.second;
                float timePercentage = time * 100.0f / totalTime;
                float globalPercentage = time * 100.0f / globalTime;
                result.push_back(PerfTimer::ResultField(key.substr(path.length()), timePercentage, globalPercentage));
            }
        }
        if (totalTime > oldTime)
            result.push_back(PerfTimer::ResultField("unspecified", (totalTime - oldTime) * 100.0f / totalTime, (totalTime - oldTime) * 100.0f / globalTime));
        std::sort(result.begin(), result.end());
        result.insert(result.begin(), PerfTimer::ResultField(_debugPath, 100, totalTime * 100.0f / globalTime));
        // ---------- 计算完毕 ----------

        // 更新共享数据
        lock.lock();
        _latestList = std::move(result);
        if (!_latestList.empty()) {
            _latestNode = _latestList[0];
            _latestList.erase(_latestList.begin());
        }
    }
}

void PerfRenderer::debugFpsMeterKeyPress( int key ) {
    // 导航仍需主线程直接获取最新子项（使用 forceUpdate 保护）
    auto list = PerfTimer::getLog(_debugPath, true);
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

    // 通知后台线程更新饼图
    _needsUpdate = true;
    _cv.notify_one();
}

void PerfRenderer::renderFpsMeter( float tickTime ) {
    if (!PerfTimer::enabled) return;

    // 每 4 帧请求后台更新一次数据
    static int frameCounter = 0;
    if (++frameCounter % 4 == 0) {
        _needsUpdate = true;
        _cv.notify_one();
    }

    // 尝试获取后台最新数据
    std::vector<PerfTimer::ResultField> list;
    PerfTimer::ResultField node("", 0, 0);
    {
        std::lock_guard<std::mutex> lock(_mutex);
        if (!_latestList.empty()) {
            list = _latestList;
            node = _latestNode;
        }
    }

    // 后备数据防止消失
    if (list.empty() && _hasValidData) {
        list = _lastValidList;
        node = _lastValidNode;
    } else if (!list.empty()) {
        _lastValidList = list;
        _lastValidNode = node;
        _hasValidData = true;
    } else {
        return;
    }

    // ---------- 以下为完全原始的绘制代码（波形图、饼图、文字）----------
    long usPer60Fps = 1000000l / 60;
    if (lastTimer == -1) lastTimer = getTimeS();
    float now = getTimeS();
    tickTimes[ frameTimePos ] = tickTime;
    frameTimes[frameTimePos ] = now - lastTimer;
    lastTimer = now;
    if (++frameTimePos >= (int)frameTimes.size()) frameTimePos = 0;

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

    // 波形图
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

    // 饼图
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

    {
        std::stringstream msg;
        if (node.name != "unspecified") msg << "[0] ";
        if (node.name.empty()) msg << "ROOT ";
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
