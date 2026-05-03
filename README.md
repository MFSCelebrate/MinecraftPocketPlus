# NoiseFarlands Reference

**基于 Minecraft 0.6.1 alpha 的无限世界改造项目，专为研究距离现象与边境之地设计**

> **开发状态：🛠️ 持续完善中**  
> 本项目正处于活跃开发阶段，已完成地形生成无限化、实体 Double 精度、噪声系统对齐 Wiki 等关键技术重构。实体渲染问题现已被彻底修复，整体已具备在极远坐标下探索边境之地的能力。

> [!WARNING]
> 该版本有时候会不定时闪退，这并非您的手机问题   
> 您的分支版本为 Development 开发版，切换为稳定版请跳转其他分支或从 Release 下载稳定版  
> 开发版本可能会有测试中的功能和不稳定性，除非你想技术开发或提前体验，否则不应使用这些版本!

## 🧭 背景

本项目源于对 Minecraft 携带版 0.6.1 alpha 的一次源码泄露。泄露的 0.6.1 alpha 是 v0.6.2 alpha 的未发布开发测试版本，其有限的世界边界掩盖了许多随距离增加而出现的异常现象——例如著名的“边境之地”和“距离现象”。

NoiseFarlands Reference 的目标是：**打破 0.6.1 的有限世界枷锁，创造出一个真正的、丝般顺滑的、高精度的无限世界，以便在其中深入研究距离效应对地形生成、实体行为及渲染管线的影响。** 它既是一个技术实验平台，也是一个边境之地爱好者的终极探索工具。

## 🎯 项目技术概览

- **🌌 无限世界动态框架**  
  `ChunkCache` 使用 `unordered_map` 动态管理区块，`RandomLevelSource` 坐标升级为 `int64_t`。世界不再有硬性边界。

- **🧨 实体与玩家 Double 化**  
  `Entity`、`Player`、`LocalPlayer` 的坐标、速度、heightOffset 均已改为 `double`；网络包 (`MovePlayerPacket`) 和 NBT 同步也已适配。

- **🗺️ 伪 MCA 多区域存档**  
  基于 EOF 追加扇区的 `RegionFile`，`ExternalFileLevelStorage` 管理多区域文件，支持世界无限扩张与保存。

- **📐 偏移与缩放系统**  
  GUI 设置项 `world_offset` 和 `world_scale` 允许实时调整地形偏移量与比例，`RandomLevelSource` 读取并应用这些参数。

- **🎨 渲染管线部分 Double 化**  
  视锥体裁剪、相机参数、RenderList 等均已改用 `double`，仅在 `glTranslatef` 等 GL 调用处转为 `float`。

- **🌄 地形噪声系统全面升级**  
  噪声生成器已按 Wiki 对齐重命名为 Low、High、Selector、Sand、Gravel、Scale、Depth、Forest 等，并通过 `PerlinNoise` / `ImprovedNoise` 实现 double 精度 3D 噪声采样，支持64位边境之地。

- **🔩 创造模式全方块注册**  
  `Inventory::setupDefault()` 中遍历 id 1~255 补全所有方块，且 `TileItem` 已启用 `setStackedByData(true)`，确保不同数据值方块独立堆叠。

- **🖥️ Debug 面板革命**  
  经过数十次迭代，彻底抛弃滚动面板，改用自适应网格布局，鼠标坐标使用 `Gui::InvGuiScale` 正确转换，所有按钮精准响应；同时内置性能剖析器，可通过数字键切换详细耗时饼图。

- **📋 日志系统**  
  项目引入了基于分类和级别的 `DebugLog` 系统，可在任意关键代码处通过 `DLOG_C(...)` 快速输出诊断信息。

- **✅ 各种修复及可开关内容**  
  带 * 的为可开关内容。  
  GUI 输入框字符重复/删除、天空网格*、世界偏移双重叠加、调试屏幕噪声显示异常、时间不流动（下界反应堆残留永夜模式）等问题均已被解决。

## 🚀 快速开始

### 环境要求

- **Android NDK**（当前构建目标为 `armeabi-v7a`，部分平台可扩展至 `arm64-v8a`）
- **Java / Gradle**（用于生成 Android 壳程序）
- C++14 或更高

### 获取源码

```bash
git clone https://github.com/MFSCelebrate/MCReference_NoiseFarlands.git
cd MCReference_NoiseFarlands
```

### 编译与运行
项目使用 Android ndk-build 编译 C++ 部分，再配合 Android Studio 或 Gradle 打包 APK。

```bash
# 进入 android 目录
cd android

# 使用 ndk-build 构建本地库
ndk-build

# 返回项目根目录，使用 Gradle 打包 APK
./gradlew assembleDebug
```

> 生成的 APK 位于 build/outputs/apk/ 目录下，可直接在支持 Android 4.0+ 的设备上安装运行。  
> 同时，你也可以通过 Github Actions 获取最新的 Windows / Android 开发版本

### 启用调试与分析

- 游戏中按下 F3 开启调试 HUD，屏幕左上角将显示 FPS、坐标、噪声值、实体数量等详细信息。
- 调试 HUD 开启后，数字键 0~9 将用于切换性能剖析页面，可观察 render/tick 等模块耗时。
- 内置 DebugScreen 面板（默认按键 E）提供了一套触屏/鼠标友好的调试按钮，包括治愈、切换模式、生成生物、清空实体、快进时间等功能。
- 如需启用完整的性能剖析（显示子模块耗时分解），请在 Android.mk 中加入 -DPROFILER 编译宏并重新编译。

### 📁 项目结构概览

```
src/
├── client/                     # 客户端渲染、输入、界面
│   ├── gui/                    # 界面（含 DebugScreen）
│   ├── renderer/               # 渲染器（LevelRenderer, GameRenderer, Tesselator 等）
│   └── player/                 # 本地玩家
├── network/                    # 网络包（MovePlayerPacket 等，已 double 化）
├── network/packet/             # 各类数据包
├── world/
│   ├── entity/                 # 实体（Entity, Mob, Player 等）
│   ├── level/                  # 世界层面（Level, ChunkCache, RandomLevelSource）
│   └── level/chunk/            # 区块实现（LevelChunk）
└── util/                       # 工具类（Mth, DebugLog, PerfTimer）
```

### 🤝 贡献
欢迎提交 Issue 和 Pull Request。请遵循以下规范：

- 代码风格保持与现有代码一致。
- 新功能请先在 Issue 中讨论可行性。
- 提交信息尽量清晰，包含简要描述和修改动机。

---

## 📖 深度阅读

有关其他提供灵感和建议的列表，请参阅以下链接：

- [🔧 主开发者](https://b23.tv/2wM5QWU)
- [🤔 提供灵感的](https://b23.tv/2Y3BsQY)
- [🧩 另一个边境之地 Mod 参考](https://b23.tv/oNqY6Hn)

---

## ⚙️ 附录：关键技术细节
**A1. 实体渲染修复最终方案**

由于网络包在解析实体数据时存在 double/float 混用导致的越界写入，污染了 Level::entities 等全局容器，使得 getAllEntities() 返回垃圾数据。最终修复方案为：

- 放弃依赖全局实体列表，改为在 LevelRenderer::renderEntities 中每帧通过 level->getEntities(NULL, aabb) 从区块动态查找附近实体。
- 将 EntityRenderDispatcher 的相机偏移永久设为玩家世界坐标插值（不再受 OPTIONS_STRIPE_REPAIR 影响）。
- 移除所有临时安全列表和静态数组，保持架构简洁。

**A2. 噪声增强与 Double 采样**

噪声采样坐标均使用 double 计算，但最终传入噪声函数时仍为 float（受原始噪声实现限制）。通过将世界坐标与偏移、缩放运算保持在 double 域，有效避免了缩放偏移浮点数不够导致的“看不到边缘之地”现象。

**A3. 调试屏幕与性能剖析**

PerfRenderer 是内置的性能剖析器，可在调试 HUD 打开时通过 0~9 数字键深入查看不同模块的耗时。要看到子模块分解，需使用 PROFILER 宏编译，并在代码中适当位置插入 TIMER_PUSH("子模块名")。

---

勿在浮沙筑高台，此项目与你一同走向世界的尽头。
