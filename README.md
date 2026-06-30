# EIEM Importing Endfield MMD

[English](README_EN.md) | 中文

为《明日方舟：终末地》提供 MMD 动画播放能力。支持肌肉动作、面部表情、手指动画、相机运动和背景音乐同步，通过游戏内 GUI 面板控制。

演示动画: [bilibili](https://www.bilibili.com/video/BV1YdEC6bEfP/)

## 用户协议与免责声明

<details>
<summary>在下载、安装或使用本插件（EIEM）之前，请您仔细阅读本协议。<b>使用本插件即代表您已完整阅读、充分理解并同意遵守以下所有条款。</b></summary>

### 1. 开源许可与最终用户权利
- 本插件基于 **AGPL-3.0** 许可证在 GitHub 平台完全开源。用户可在遵守该许可证的前提下自由使用、修改和分发本插件的源代码。
- 最终用户（End User）在不对本插件进行修改的前提下，使用和分发本插件**不受任何限制**。此权利不因用户是否违反本协议而改变。

### 2. 反欺诈声明
- 您**不得**在网络销售平台公然售卖本插件**软件本体**且未提供 GitHub 仓库地址与售后服务。
- 本插件在 GitHub 平台完全免费开源，如果您是付费购买获取的，请知悉本插件可从 GitHub 免费获取。

### 3. 内容合规与行为约束
- 本插件本身不包含任何游戏美术资产。用户知悉并同意，《明日方舟：终末地》游戏内置的官方动画、场景、模型等资产其版权完全隶属于鹰角网络，并不适用 AGPL-3.0 协议。您**不应该且不得**利用本插件，或利用游戏内置的官方动画、场景、模型等游戏资产，制作、播放或传播任何不合适的动作/动画（包括但不限于色情、暴力、政治敏感等违反法律法规或引起社区不适的内容）。

### 4. 风险与免责声明
- 本项目仅供学习、技术研究和交流目的。本插件中使用的明日方舟游戏数据资产版权均隶属于鹰角网络。使用本工具可能违反游戏服务条款，存在账号封禁的风险。因使用本插件而直接或间接导致的任何损失（包括但不限于账号封禁、游戏数据损坏等），**本项目不承担任何法律或经济责任**。用户需自行承担所有风险，强烈建议您在测试账号上运行。

</details>

## 功能

### 已实装
- **肌肉驱动动作**：通过 95 个 muscle 值，驱动全身动作
- **手指动画**：30 根手指骨骼的独立旋转控制
- **面部表情**：AIUEO、眨眼、笑眼等基础表情
- **相机运动**：VMD 相机关键帧（含角色朝向对齐）
- **音频同步**：MCI 后端播放 WAV/MP3 BGM

### 已计划
- VMD直接播放模式
- 多角色同屏播放
- ...

## 下载

您可以在 [Releases](https://github.com/Sasye/EIEM/releases) 下载最新发布版或从源代码自行编译。

## 安装

将以下文件复制到游戏目录（`Endfield.exe` 所在文件夹）：

```
bin/eiem.dll             → 游戏目录/plugin/eiem.dll
bin/vulkan-1.dll         → 游戏目录/vulkan-1.dll
bin/d3dcompiler_47.dll   → 游戏目录/d3dcompiler_47.dll
```

> **注意**：`d3dcompiler_47.dll`（DX环境）和 `vulkan-1.dll`（Vulkan环境）为代理加载器，二者放其一或全放均可。如果你同时在使用其他共用的代理加载器插件（如 [AntiKick](https://github.com/Sasye/EndFieldAntiKick) [SynchroFocus](https://github.com/Sasye/EndfieldSynchroFocus) 或 [EndfieldCombatHUD](https://github.com/Sasye/EndfieldCombatHUD)等），无需重复放置代理加载器。

> 如果您**没有安装过此类型的插件**，您可能需要自行创建plugin文件夹。

## 资源文件准备

自动扫描 `游戏目录/plugin/` 或手动指定以下文件：

| 文件 | 说明 | 必要性 |
|------|------|--------|
| `muscle_anim.bin` | MUS4 格式动作数据（由ExportMuscleAnimation.cs导出） | **必须** |
| `*.vmd` | VMD 文件（面部表情 morph 数据） | 可选（自动扫描 plugin 目录下的 .vmd） |
| `camera.vmd` | 相机运动数据 | 可选 |
| `bgm.wav` 或 `bgm.mp3` | 背景音乐 | 可选 |

## 使用方法

1. 按上述方式安装后启动游戏，进入游戏。
2. 按 **Insert** 键打开 GUI 面板。
3. 加载指定文件后在「控制」页点击 **播放** 按钮开始播放动画。

## 动作导出（VMD → MUS4）

需要先通过 Unity 编辑器将 VMD 动画转换为 MUS4 格式的 `muscle_anim.bin`。

### 前置条件

- Unity 编辑器
- [MMD4Mecanim](https://stereoarts.jp/#:~:text=MMD4Mecanim_Beta_20200105.zip) — 用于将 VMD 转换为 Unity AnimationClip
- 任意 Humanoid MMD 模型

### 步骤

1. 将 `ExportMuscleAnimation.cs` 放入 Unity 项目的 `Assets/Editor/` 文件夹
2. 导入 MMD 模型（设为 Humanoid Rig），使用 MMD4Mecanim 将 VMD 转换为 AnimationClip
3. 创建 Animator Controller，将转换好的 AnimationClip 添加为默认状态
4. 将 Animator Controller 挂载到场景中的模型上
5. **选中模型**，点击菜单栏 `Tools > Export Muscle Animation`
6. 导出文件 `Assets/muscle_anim.bin`，将其复制到游戏的 `plugin/` 目录即可

> 面部表情不走此导出流程 — 直接将原始 `.vmd` 文件放到 `plugin/` 目录下即可，本插件将自动解析 morph 数据。
