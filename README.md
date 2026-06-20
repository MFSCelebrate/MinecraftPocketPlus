# MCRe NoiseFarlands

![Arch](https://img.shields.io/badge/arch-armeabi--v7a%20%7C%20arm64--v8a%20%7C%20x86__64-green)
![License](https://img.shields.io/badge/license-GPLv3-blue)
![Status](https://img.shields.io/badge/status-Development-orange)

> [!WARNING]
> **Development 开发版**，可能包含实验性改动和未完善功能。
> 稳定版请从 [Releases](https://github.com/MFSCelebrate/MCReference_NoiseFarlands/releases) 下载。
> **不适合常规游戏用途！**

---

## 🧊 项目简介

**MCRe NoiseFarlands** 是一个深度改造的 Minecraft Pocket Edition 客户端，
基于 **MCPE 0.6.1 Alpha 泄露源码** 构建，
以 **B1.7.3 视觉风格** 为目标，探索 Minecraft 的算法极限。

> *"在 double 的尾数位上建立殖民地，在整数的边界上盖城堡。"* 🌌

---

### 🔥 核心特性

| 特性 | 说明 |
|------|------|
| 🧮 **真无限世界** | `BigWorldCoordinate`（Boost 50 位十进制精度）+ `WorldOrigin` 动态原点 |
| 🔢 **全链路 Double 精度** | 实体坐标、噪声采样、地物生成全部 double 化 |
| 🏔️ **边境之地探索** | 偏移/缩放/64-bit 噪声/Double 噪声/海平面自定义/条纹修复 |
| 🌌 **末地生成器** | 从 MCBE 1.20 反向工程移植：中央岛 + 外岛 + 黑曜石柱 + 末地环 |
| 🎵 **SimplexNoise** | 2D 单形噪声——MCBE 1.20 / Java 1.18+ 地形生成的基础 |
| 📟 **双生成器调试面板** | 动态判定主世界/末地，显示完整噪声参数 + BigWorldCoordinate 精度 |
| 🖼️ **B1.7.3 视觉风格** | 经典亮绿色草地、怀旧光照、老版本地形 |
| ⚡ **高性能** | OpenGL ES 1.1 + VBO，帧率 90-125 FPS |
| 🐧 **跨平台** | Android (arm32/arm64) + Windows (x86_64) + CI 自动构建 |

---

## 🧮 技术架构

### 双轨坐标系统

```
玩家绝对坐标 (BigWorldCoordinate, 50位)
       │
       ▼
  WorldOrigin.tickBig()
       │
       ├── origin (Big) — 动态原点, 2^48 阈值自动迁移
       └── local (double) — 局部坐标, 用于渲染/碰撞
```

| 类型 | 底层 | 阈值 | 用途 |
|------|------|------|------|
| `WorldCoordinate` | `double` | — | offset/scale 存储 |
| `BigWorldCoordinate` | `cpp_dec_float<50, et_off>` | 2^48 | 绝对坐标精确计算 |
| `WorldCoordinate_Integer` | `int64_t` | — | 区块/方块整数坐标 |
| `BigWorldCoordinate_Integer` | `cpp_int, et_off` | 2^48 | 远距离整数防溢出 |

### 噪声体系

**主世界**：8 个 Perlin/ImprovedNoise × `684.412`（B1.7.3 特征常数）

| # | 噪声 | 维度 | 倍频 | 溢出距离 |
|---|------|------|------|---------|
| 1 | `lperlinNoise1` (Low) | 3D | 16 | ~12.5M |
| 2 | `lperlinNoise2` (High) | 3D | 16 | ~12.5M |
| 3 | `perlinNoise1` (Selector) | 3D | 8 | ~10 亿 ⭐ |
| 4-8 | 沙/砾/缩放/深度/森林 | 2D | 4-16 | ~43M ~ 2.75e11 |

**末地**：3×PerlinNoise + 1×SimplexNoise

| # | 噪声 | 维度 | 倍频 | 作用 |
|---|------|------|------|------|
| 1 | `pNoise1` | 3D | 16 | 低频密度 |
| 2 | `pNoise2` | 3D | 16 | 高频密度 |
| 3 | `pNoise3` | 3D | 8 | 混合选择器 |
| 4 | `sNoise1` | 2D | — | **SimplexNoise** 外岛检测 |

### 末地环理论

> **成因**：`int x²` 溢出 → 负值 → sqrt(负数) → NaN → 自然虚空  
> **周期**：距离² 每 2³² → 世界坐标每 **524,288 块**  
> **第一环**：370,728 ~ 524,288 块  
> **面积恒定**：S = (2³⁷ − 2⁶)π，与环序号 n 无关

通过 `OPTIONS_END_CIRCLES` 开关控制 int 溢出路径——开启后末地出现经典环状虚空结构。

---

## 🔧 构建

| 平台 | 工具 | 目标 |
|------|------|------|
| 🤖 Android | NDK r23c (Clang 12, c++_static) | armeabi-v7a + arm64-v8a |
| 🪟 Windows | CMake + llvm-mingw + Boost 1.83.0 | x86_64 |
| 🔄 CI | GitHub Actions | 自动构建 + artifact |

构建脚本：`build.sh` (Android) / `build.ps1` (Windows)

---

## ✅ 已完成的重大工作 (50+ Bug / 70+ 文件)

### 🧮 坐标系统（14 项核心改动）
- Entity Big 访问器 + 速度 `m_bigVx/y/z`
- `updatePositionFromBB` Big 合成绝对坐标
- `isInWall()` / `isInWater()` Big→BigWorldCoordinate_Integer 精确计算
- `LocalPlayer` Big 位置 + `WorldOrigin` 动态追踪
- `travel()` 重力/摩擦/流体/梯子全用 Big 速度
- `moveRelative()` / `lerpMotion()` / `push()` 速度操作 Big 化
- `move()` 碰撞清零 + 末尾方块检测用 Big 整数坐标

### 🐛 2^48+ 移动修复（5 个关键 BUG）
1. **2^48 完全卡死**：`move()` useLocal 分支缺 `storeAbsolutePosition`
2. **AABB 坐标飞负无穷**：`bb.set()` 后忘 `bb.move(ox)`
3. **2^53 缓慢回弹**：`setPos` 直接用 BigWorldCoordinate(double) 精度全丢
4. **网络回环覆写 Big**：`autoSendPosRot` 发送 MoveEntityPacket 回传污染
5. **lerpTo float 污染**：`lx/ly/lz` 从 float 改为 double

### 🌌 末地生成器（JS 逆向 → C++）
- 3 组 PerlinNoise + 1 组 SimplexNoise
- DENSITY_X=3 × DENSITY_Y=33 × DENSITY_Z=3 (297 采样点)
- 中央岛(半径 ~12.5 chunks) + 外岛(sNoise1 < -0.9) + 末地环(int 溢出)
- 黑曜石柱(10 根, Fisher-Yates, 半径 42, 高度 76~103)
- 偏移/缩放全轴支持
- 全亮度锁定避免光照卡顿 (memset(skyLight, 0xFF))
- 三线性插值 (NoiseCellInterpolator)

### 🎵 SimplexNoise 移植
- 从 Project Mirror (MCBE 1.20) JS 源码移植 2D SimplexNoise
- 替代 `sNoise1` 的 PerlinNoise 近似 → 外岛分布自然圆形

### 🖼️ 渲染修复
- 条纹之地修复 (`int64_t` 原点 + chunk 坐标折回)
- 64 位条纹修复、地形下沉、星星/云/视锥裁剪修复
- Chunk::rebuild / Level::animateTick 空指针防御

### 📟 调试面板
- 双生成器动态判定 (RandomLevelSource vs TheEndLevelSource)
- 末地模式隐藏 Sea Level
- 噪声行替换 + BigWorldCoordinate 坐标 + 精度三色显示
- 64Bit Farlands 双生成器状态显示

### 🏗️ 构建修复
- Windows 端 DLL 打包 (libc++.dll / libunwind.dll / libpng16.dll)
- NDK 升级、ChunkCache 双重释放、Region bad_alloc

---

## ⚠️ 待处理

| # | 问题 | 优先级 |
|---|------|--------|
| 1 | 摄像机 BigWorldCoordinate 渲染改造 | 🔴 高 |
| 2 | CanyonFeature / DungeonFeature 激活+修复 | 🟡 中 |
| 3 | 创建世界页面 World 选项卡滚动面板 | 🟡 中 |
| 4 | 区块剔除性能优化 | 🟢 低 |
| 5 | EGL context lost 恢复 | 🟢 低 |

---

## 📂 关键文件

| 文件 | 说明 |
|------|------|
| `src/util/WorldCoordinate.h` | 大数类型体系 (BigWorldCoordinate, WorldCoordinate_Integer) |
| `src/util/WorldOrigin.h` | 动态原点追踪器 (2^48 阈值自动迁移) |
| `src/world/entity/Entity.h/cpp` | Big 访问器 + 速度全链路 + move() local 帧碰撞 |
| `src/client/player/LocalPlayer.h/cpp` | Big 位置存储 + setPos/moveTo 差分保护 |
| `src/world/level/levelgen/TheEndLevelSource.h/cpp` | 完整末地生成器 (外岛+柱+环+偏移+光照) |
| `src/world/level/levelgen/synth/SimplexNoise.h` | 2D SimplexNoise (MCBE 1.20 / Java 1.18+ 基础噪声) |
| `src/world/level/levelgen/RandomLevelSource.h/cpp` | 主世界生成器 (8 噪声 + 偏移/缩放) |
| `src/client/renderer/LevelRenderer.cpp` | 动态原点渲染Ch条纹|ui.cpp |调试生成 +坐标|/client/Options.h/cpp` | 全量 Options 注册 (末地环/偏移/缩放/64bit/Double 噪声等) |

---

## 🎯 代码约定

- 新类型 → `src/util/`
- Boost 大数 → 必须 `et_off`（防爆栈 + 防 ABI 冲突）
- {} 块隔离生命周期
- 跨编译单元 → 只用 `double`/`int64_t`，不用 Boost 大数
- 噪声全链路 → `double`
- 代码修改 → 🔍分析→🔧修复→📊效果 三段式，先给最小改动量

---

## ⚖️ License

MCRe NoiseFarlands 原创修改代码以 [GNU General Public License v3.0](LICENSE) 发布。

底层 MCPE 0.6.1 Alpha 源码版权归 **Mojang AB / Microsoft Corporation** 所有。

```
Copyright (C) 2025-2026  MFSCelebrate_
Copyright (C) 2025-2026  INF32768
```

**MCRe 原创修改包括但不限于**：BigWorldCoordinate 坐标系统、WorldOrigin 动态原点、
末地生成器 (TheEndLevelSource)、SimplexNoise 移植、偏移/缩放/末地环支持、
调试面板双生成器适配、条纹之地修复、全链路 Double 精度化。

任何基于本项目的衍生作品必须同样以 GPLv3 开源，并保留原作者署名。

---

## 📝 参考与致谢

| 来源 | 说明 |
|------|------|
| MCPE 0.6.1 Alpha 泄露源码 | 基础引擎 |
| Kolyah35 的 MCPE 0.6.1 Plus | 二改增强参考 |
| Project Mirror (MCBE 1.20 逆向) | 末地生成器 + SimplexNoise 算法来源 |
| "念" 大佬的边境之地 Mod | Y 轴突破参考 |
| Java 版各大边境之地 Mod | 算法参考 |
| B1.7.3 噪声特征常数 (`684.412`) | 噪声缩放因子 |

**致谢**：Mojang AB、4J Studios、Kolyah35、Project Mirror (HTMonkeyG)、
"念" 以及所有边境之地研究社区的探索者 🧊

---

## ⚠️ 免责声明

- 本项目**仅供技术研究与学习**，不得用于任何商业用途
- 源码版权归 **Mojang AB / 4J Studios / Microsoft Corporation** 所有
- 使用者需自行承担风险，开发者不对数据丢失或设备损坏负责
- **不适合生存模式！这不适合生存模式！这不适合生存模式！**

---

## 🔗 链接

- [GitHub 仓库](https://github.com/MFSCelebrate/MCReference_NoiseFarlands)
- [BiliBili 频道](https://b23.tv/pEkoJ0U)

---

> *"在 double 的尾数位上建立殖民地，在整数的边界上盖城堡，
> 用 cpp_dec_float<50> 探索 float 永远到不了的距离。"* 🧊🚀

---
