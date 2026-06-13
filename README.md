
# MCRe NoiseFarlands

> **基于 Minecraft PE 0.6.1 Alpha 泄露源码改造的边境之地研究专用项目**
>
> 🏔️ 将有限的 MCPE 0.6.1 世界彻底改造为**真正的、丝般顺滑的、高精度的无限世界**

![Platform](https://img.shields.io/badge/platform-Android%20%7C%20Windows-lightgrey)
![License](https://img.shields.io/badge/license-Research%20Only-red)
![Status](https://img.shields.io/badge/status-Development-blue)
![Arch](https://img.shields.io/badge/arch-armeabi--v7a%20%7C%20arm64--v8a%20%7C%20x86__64-green)

> [!WARNING]
> 该版本为 Development 开发版，可能有不稳定性。稳定版请从 Release 页面下载。
> 开发版本包含测试中的功能和实验性改动，不适合常规游戏用途。

---

## 📖 项目简介

**MCRe NoiseFarlands** 是一个深度改造的 Minecraft Pocket Edition 客户端，基于 **MCPE 0.6.1 Alpha 泄露源码** 构建。

项目核心目标：
- 🧮 **真无限世界**：引入 `BigWorldCoordinate`（Boost 50 位十进制精度）+ `WorldOrigin` 动态原点系统，实现真正无限、高精度的地形生成
- 🔢 **Double 精度化**：实体坐标、噪声采样、地物生成全链路 double 化
- 🏔️ **边境之地探索**：支持世界偏移/缩放、64-bit 噪声、Double 噪声、海平面自定义等，便于研究远距离现象
- ⚡ **高性能**：OpenGL ES 1.1 + VBO 渲染，帧率稳定 90-125 FPS

---

## 🧮 技术架构

### 大数类型体系 (`src/util/WorldCoordinate.h`)

| 类型 | 底层 | 阈值 | 用途 |
|------|------|------|------|
| `WorldCoordinate` | `double` | — | offset/scale 存储 |
| `BigWorldCoordinate` | `cpp_dec_float<50, et_off>` | 2^48 | 绝对坐标精确计算 |
| `WorldCoordinate_Integer` | `int64_t` | — | 区块/方块坐标 |
| `BigWorldCoordinate_Integer` | `cpp_int, et_off` | 2^48 | 整数防溢出计算 |

### 噪声体系（8 噪声 + B1.7.3 特征常数）

| # | 噪声 | 维度 | 倍频 | 核心常数 | 32bit 溢出 |
|---|------|------|------|----------|-----------|
| 1 | `lperlinNoise1` (Low) | 3D | 16 | 684.412 | ~12.5M |
| 2 | `lperlinNoise2` (High) | 3D | 16 | 684.412 | ~12.5M |
|perNoise1` (Selector) | 3D | 8 | 684.412 | ~10 亿 ⭐ |
| 4 | `perlinNoise2` (Sand) | 2D | 4 | 1/32 | ~2.75e11 |
| 5 | `perlinNoise3` (Gravel) | 2D | 4 | 1/64 | ~1.37e11 |
| 6 | `scaleNoise` | 2D | 10 | 1/80 | ~7.66e9 |
| 7 | `depthNoise` | 2D | 16 | 1/200 | ~43M |
| 8 | `forestNoise` | 2D | 8 | 0.5 | ~4.29e9 |

> 选择器 32 位溢出 ≈ 10 亿格 = 经典遥远之地（Farther Lands）  
> Double 精度条纹在 2^53 ≈ 9e15（已用 WorldOrigin 动态原点修复）

### WorldOrigin 自动切换系统

每帧用 `BigWorldCoordinate` 算术重算 `local = abs - origin`，若 |local| ≥ 2^48 自动迁移原点，local 精度 ≤ 0.03 格。

### Entity::move() 精度保护策略 (v4)

> **核心矛盾**：`bb.move(xa, 0, 0)` 在 2^48 处因 double ULP=0.0625，0.1 增量舍入为 0

**三重保护方案**：
1. **碰撞箱进 local 空间** — `bb.move(-ox, -oy, -oz)`，碰撞检测精度丝般顺滑
2. **getCubes 独立构造绝对 AABB** — 用 `bb.expand(xa,ya,za).move(ox,oy,oz)` 查方块
3. **Big 最终合成** — `(BigWorldCoordinate(bb.x0+ox) + BigWorldCoordinate(bb.x1+ox)) / 2`

---

## 🔧 构建环境

| 平台 | 构建工具 | 说明 |
|------|---------|------|
| 🤖 Android | NDK r23c (Clang 12, c++_static) | armeabi-v7a + arm64-v8a |
| 🪟 Windows | CMake + llvm-mingw + Boost 1.83.0 | x86_64 |
| 🔄 CI | GitHub Actions | `build.yml` / `build.sh` / `build.ps1` |

---

## ✅ 已完成修复清单 (37 项)

### 🖼️ 渲染 (1-8)
条纹之地修复、64 位条纹修复、地形下沉、星星渲染、云渲染、视锥裁剪、Windows glad

### 🔢 精度 (9-16, 37)
int→int64_t、resortChunks 安全取模、float→double、噪声 double 化、BiomeSource double 化、RandomLevelSource double 化、地物 int64_t、Entity::move() 2^48+ 精度保护 (v4)

### 🖥️ 调试 (17-20)
NaN 修复、%lld 格式、精度颜色编码、面板缩放

### 👆 触控 (21-25)
数字键支持、小数点输入、触控延迟、按钮穿透、_forceCanUse

### 🌐 网络 (26)
版本检查移除

### 🎮 游戏机制 (27-28)
传送物理、相对坐标解析

### 🏗️ 构建 (29-31)
NDK 升级、ChunkCache 双重释放、Region bad_alloc

### 📐 理论 (32-36)
噪声分析、边境之地坐标、大数选型、构建脚本、会话迁移

---

## ⚠️ 待处理

| # | 问题 | 优先级 |
|---|------|--------|
| 1 | 创建世界页面 World 选项卡滚动面板 | 中 |
| 2 | CanyonFeature / DungeonFeature 激活 | 中 |
| 3 | 区块剔除性能优化 | 低 |
| 4 | EGL context lost 恢复 | 低 |
| 5 | 完整物品系统 100% | 低 |

---

## 🎯 代码风格约定

- 新类型统一放 `src/util/`
- Boost 大数必须 `et_off`（防爆栈 + 防 ABI 冲突）
- 调试屏幕大数用 `{}` 块隔离生命周期
- 跨编译单元只用 `double`/`int64_t`，不用 Boost 大数
- 噪声全链路 `double`
- `-fstack-protector` 崩溃：大数组改 `static`，大数用 `et_off` + 块隔离

---

## 📂 关键文件

| 文件 | 关键改动 |
|------|---------|
| `src/util/WorldCoordinate.h` | 大数类型、阈值判断 |
| `src/util/WorldOrigin.h` | tick() 自动原点切换、OriginStep=2^48 |
| `src/world/entity/Entity.cpp/h` | move() local 帧碰撞 + Big 合成 |
| `src/client/player/LocalPlayer.h` | WorldOrigin 成员、Big 位置存储 |
| `src/client/renderer/LevelRenderer.cpp/h` | 动态原点渲染、resortChunks |
| `src/world/level/levelgen/*` | 噪声全 double 化、地物 int64_t |
| `src/client/gui/Gui.cpp` | 调试面板 Big 坐标显示 |

---

## 📸 截图

<p align="center">
  <img src="https://github.com/user-attachments/assets/713ec506-0937-4c7a-96fc-8b2c9d59301b" width="80%" alt="性能截图" />
</p>

---

## 📝 参考与致谢

| 来源 | 说明 |
|------|------|
| 4chan 泄露 MCPE 0.6.1 源码 | 基础引擎 |
| Kolyah35 的 MCPE 0.6.1 Plus | 二改增强 |
| "念" 大佬的边境之地 Mod | Y 轴突破参考 |
| Java 版各大边境之地 Mod | 算法参考 |
| B1.7.3 噪声特征常数 (`684.412`) | 基础缩放因子 |
| Project Mirror (MCBE 1.20 逆向) | 末地生成器参考 |

**致谢**：Mojang AB、4J Studios、Kolyah35、"念" 以及所有边境之地研究社区的探索者 🧊

---

## ⚠️ 免责声明

- 本项目**仅供技术研究与学习**，不得用于任何商业用途
- 源码版权归 Mojang AB / 4J Studios 所有
- 使用者需自行承担风险，开发者不对数据丢失或设备损坏负责
- **不适合生存模式！**

---

## 🔗 链接

- [GitHub 仓库](https://github.com/MFSCelebrate/MCReference_NoiseFarlands)
- [BiliBili 频道](https://b23.tv/pEkoJ0U)

---

> *"在 double 的尾数位上建立殖民地，在整数的边界上盖城堡。"* 🌌
