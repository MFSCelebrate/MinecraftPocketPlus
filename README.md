# MCReference_NoiseFarlands

> **基于 Minecraft PE 0.6.1 alpha 泄露源码改造的边境之地（Far Lands）研究专用 Mod**
> 
> 🏔️ 将有限的 MCPE 0.6.1 世界彻底改造为**真正的、丝般顺滑的、高精度的无限世界**

![GitHub release (latest by date)](https://img.shields.io/badge/release-v250503--0315-blue)
![Platform](https://img.shields.io/badge/platform-Android%20%7C%20Windows-lightgrey)
![License](https://img.shields.io/badge/license-Research%20Only-red)
![Status](https://img.shields.io/badge/status-Untable-brightgreen)

> [!WARNING]
> 该版本有时候会不定时闪退，这并非您的手机问题   
> 您的分支版本为 Development 开发版，切换为稳定版请跳转其他分支或从 Release 下载稳定版  
> 开发版本可能会有测试中的功能和不稳定性，除非你想技术开发或提前体验，否则不应使用这些版本!  
> 该 MCBigInteger_Test 分支将会把极限推至几乎无限大的位置而不精度丢失，但是性能会极度下降!!!

---

## 📖 项目简介

**NoiseFarlands（未重置版）** 是一个深度改造的 Minecraft Pocket Edition 客户端，基于 2013 年泄露的 **v0.6.2 alpha 未发布开发测试版** 源码构建。

项目核心目标：
- ✅ **无限世界**：完全移除原版的有限世界边界，实现真正的无限地形生成
- ✅ **双精度化（Double Precision）**：实体坐标、玩家位置、相机系统全面升级为 `double` 类型，保障远距离精度
- ✅ **边境之地探索**：支持世界偏移、缩放、64-bit 噪声、Double 噪声等选项，便于研究距离现象
- ✅ **高性能**：经过数十次剖析迭代，帧率稳定 90-125 FPS，峰值破 120

---

## 🎯 适用场景

| ✅ 适合 | ❌ 不适合 |
|--------|----------|
| 边境之地/遥远之地地形研究 | 常规生存游戏 |
| 噪声生成算法实验 | 多人联机娱乐 |
| MCPE 早期版本考古 | Bug 较少的稳定体验 |
| 渲染管线性能剖析 | 原版特性体验 |

---

## 📦 分支策略

| 分支类型 | 说明 |
|---------|------|
| **稳定版本（Stable）** | 零影响探索的大 Bug，帧率稳定，适合长时间边境之地探索 |
| **开发版本（Dev）** | 激进的新功能测试，可能不稳定 |

---

## 🚀 快速开始

### Windows
1. 下载 Release 中的 `Minecraft.Windows.exe`
2. 双击运行即可

### Android
1. 下载 Release 中的 `MinecraftPE.apk`
2. 安装至 Android 设备
3. 首次启动需给予存储权限

> **注意**：本项目已取消 Web 和 Linux 平台的构建，但保留了源码备份。

---

## ⚙️ 核心功能

### 🌍 世界生成
- **无限世界动态框架**：`ChunkCache` 使用 `unordered_map` 动态管理区块
- **64-bit / Double 噪声选项**：支持 32-bit、64-bit、Double 三种精度模式
- **世界缩放与偏移**：通过设置界面实时调整 `world_scale_x/y/z` 和 `world_offset_x/y/z`
- **海平面调整**：可自定义海平面高度（0-127）
- **天空网格修复**：可选禁用天空网格现象

### 🎨 渲染系统
- **实体渲染彻底修复**：解决了全局实体列表内存污染问题，采用动态区块查找
- **LOD 系统**：256 格外不透明、透明、水面层均不渲染
- **视距滑块**：设置界面支持 0-3 级视距调整
- **条纹修复**：可选修复远距离渲染时的条纹闪烁问题

### 📊 性能剖析器
- **饼图实时刷新**：隐藏 `unspecified` 分支，其余等比放大
- **剖析栈平衡**：修复了 `root.root` 叠层异常
- **自适应压缩**：超 128 条目时自动剪除微小占比
- **精度监测**：调试屏幕底部显示当前坐标的 `double`/`float` 精度丢失值

### 🛠️ 调试面板
- 数字键 `0-9` 专用于剖析器页面切换
- `Backspace` 键后退导航
- 全新增补调试热键：Toggle Difficulty、NoPvP、NoPvM、NoMvP、Immutable World、NameTags 等
- Teleport 传送功能

---

## 📸 性能数据

经性能剖析器开启后的实际帧数表现：

<p align="center">
  <img src="https://github.com/user-attachments/assets/713ec506-0937-4c7a-96fc-8b2c9d59301b" width="80%" alt="性能截图" />
</p>

> 图示显示开启调试屏幕且高性能剖析器全速运行状态下的游戏帧率，画面稳定运行在 110~120 FPS。

---

## 📝 参考源码

| 来源 | 说明 |
|------|------|
| 4chan 泄露原生 MCPE 0.6.1 源码 | 基础引擎 |
| Kolyah35 的 MCPE 0.6.1 Plus | 二改增强 |
| "念" 大佬的边境之地 Mod | Y 轴突破参考 |
| Java 版各大边境之地 Mod | 算法参考 |

---

## 🤝 致谢

- **Mojang AB** — 创造了 Minecraft
- **4J Studios** — 负责了早期携带版的移植开发
- **Kolyah35** — 0.6.1 Plus 版本作者
- **"念" (Nian)** — 边境之地 Mod 先驱
- 所有在边境之地研究路上探索的社区成员

---

## ⚠️ 免责声明

- 本项目**仅供技术研究与学习**，不得用于任何商业用途
- 源码版权归 Mojang AB / 4J Studios 所有
- 使用者需自行承担使用风险，开发者不对任何数据丢失或设备损坏负责
- **不适合生存模式！**

---

## 🔗 相关链接

- [GitHub 仓库](https://github.com/MFSCelebrate/MCReference_NoiseFarlands)
- [BiliBili 频道](https://b23.tv/pEkoJ0V)

---
