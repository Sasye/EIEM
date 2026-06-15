# EIEM Importing Endfield MMD

[English](README_EN.md) | 中文

为《明日方舟：终末地》提供 MMD 动画播放能力。支持肌肉动作、面部表情、手指动画、相机运动和背景音乐同步，通过游戏内 GUI 面板控制。

演示动画: [bilibili](https://www.bilibili.com/video/BV1YdEC6bEfP/)

## 使用须知

- 您**不应该**利用本插件或游戏内置的官方动画、场景、模型等游戏资产制作、播放任何不合适的动作/动画(包括但不限于色情、暴力等违反法律法规或引起社区不适的内容)。
- 您**不应该**在网络销售平台公然售卖本插件本体且未提供 GitHub 仓库地址与售后服务。
- 您**可以**在遵循 AGPL-3.0 license 的前提下使用、修改和分发本插件的二进制文件及其源代码。
- 下载、安装或使用本插件，即代表您已知悉并同意上述内容。

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

## 与 EFMI (XXMI) 的兼容性问题

如果与 EFMI (XXMI) 一起使用本插件，则**可能会**导致游戏无法打开或崩溃。
该问题是由于 EFMI 进行了全局的 DirectX 底层劫持，试图强行接管 EIEM 创建的独立 UI 渲染管线。这种越界操作会导致 EFMI 自身触发严重的内存异常，并最终连带导致整个游戏进程崩溃。
由于 hook 时机的必要性，此冲突目前**无法由 EIEM 侧进行简单修复**，因此本项目**暂无主动支持 EFMI 的计划**。
您可以尝试以下替代方案：
- 向 EFMI 侧提交 Issue 进行反馈，或等待 EFMI 侧的后续更新。
- 非常欢迎您提交 PR 来协助修复此冲突！

## 下载

您可以在 [Releases](https://github.com/Sasye/EIEM/releases) 下载最新发布版或从源代码自行编译。

## 安装

将以下文件复制到游戏目录（`Endfield.exe` 所在文件夹）：

```
bin/eiem.dll             → 游戏目录/plugin/eiem.dll
bin/vulkan-1.dll         → 游戏目录/vulkan-1.dll
bin/d3dcompiler_47.dll   → 游戏目录/d3dcompiler_47.dll
```

> **注意**：`d3dcompiler_47.dll`（DX环境）和 `vulkan-1.dll`（Vulkan环境）为代理加载器，二者放其一或全放均可。如果你同时在使用其他共用的代理加载器插件（如 [AntiKick](https://github.com/Sasye/EndFieldAntiKick) [SynchroFocus](https://github.com/Sasye/EndfieldSynchroFocus) 或 [BetterBuffBar](https://github.com/Sasye/EndfieldBetterBuffBar)等），无需重复放置代理加载器。

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

## 免责声明

本项目仅供学习和研究目的。使用本工具可能违反游戏服务条款，存在账号封禁风险。
请在测试账号上使用，风险自负。
