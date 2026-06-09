# EIEM Importing Endfield MMD

English | [中文](README.md)

Brings MMD animation playback to *Arknights: Endfield*. Supports muscle-driven body motion, facial expressions, finger animation, camera motion, and synced background music, all controlled through an in-game GUI panel.

## Usage Guidelines

- You **should not** use this plugin or the game's built-in official animation assets to create or play any inappropriate motions/animations (including but not limited to content that is pornographic, violent, violates laws and regulations, or causes community discomfort).
- You **should not** openly sell this plugin itself on online retail platforms without providing the GitHub repository address and after-sales service.
- You **can** use, modify, and distribute the binary files and source code of this plugin under the terms of the AGPL-3.0 license.
- By downloading, installing, or using this plugin, you acknowledge that you have read and agreed to the above terms.

## Features

### Implemented
- **Muscle-driven motion**: Drives full-body animation via 95 muscle values
- **Finger animation**: Independent rotation control for 30 finger bones
- **Facial expressions**: Basic expressions including AIUEO, blinks, and smiling eyes
- **Camera motion**: VMD camera keyframes (with character-facing alignment)
- **Audio sync**: MCI backend plays WAV/MP3 BGM

### Planned
- Direct VMD playback mode
- Multi-character screen playback
- ...

## Installation

Copy the following files into the game directory (the folder containing `Endfield.exe`):

```
bin/eiem.dll             → game_dir/plugin/eiem.dll
bin/vulkan-1.dll         → game_dir/vulkan-1.dll
bin/d3dcompiler_47.dll   → game_dir/d3dcompiler_47.dll
```

> **Note**: `d3dcompiler_47.dll` (DX environment) and `vulkan-1.dll` (Vulkan environment) are proxy loaders. You can place either one or both. If you already use another plugin that shares a proxy loader (such as [AntiKick](https://github.com/Sasye/EndFieldAntiKick), [SynchroFocus](https://github.com/Sasye/EndfieldSynchroFocus), or [BetterBuffBar](https://github.com/Sasye/EndfieldBetterBuffBar), etc.), there is no need to place the proxy loader again.

## Preparing Resource Files

Auto-scans `game_dir/plugin/` or manually specify the following files:

| File | Description | Required |
|------|-------------|----------|
| `muscle_anim.bin` | MUS4-format motion data (exported via ExportMuscleAnimation.cs) | **Yes** |
| `*.vmd` | VMD file (facial expression morph data) | Optional (auto-scans .vmd in plugin dir) |
| `camera.vmd` | Camera motion data | Optional |
| `bgm.wav` or `bgm.mp3` | Background music | Optional |

## Usage

1. Install as described above, launch the game, and enter the game.
2. Press **Insert** to open the GUI panel.
3. Load the desired files, then click the **Play** button on the "Control" tab to start playback.

## Motion Export (VMD → MUS4)

You must first convert VMD animations to MUS4 format (`muscle_anim.bin`) using the Unity editor.

### Prerequisites

- Unity Editor
- MMD4Mecanim — converts VMD to Unity AnimationClip
- Any Humanoid MMD model

### Steps

1. Place `ExportMuscleAnimation.cs` into your Unity project's `Assets/Editor/` folder
2. Import an MMD model (set to Humanoid Rig) and use MMD4Mecanim to convert a VMD file into an AnimationClip
3. Create an Animator Controller and add the converted AnimationClip as the default state
4. Assign the Animator Controller to the model in the scene
5. **Select the model**, then click `Tools > Export Muscle Animation` in the menu bar
6. The exported file `Assets/muscle_anim.bin` will be created — copy it to the game's `plugin/` directory

> Facial expressions do not go through this export — simply place the original `.vmd` file in the `plugin/` directory and the plugin will parse the morph data automatically.

## Disclaimer

This project is for educational and research purposes only. Using this tool may violate the game's terms of service and carries a risk of account ban.
Use it on a test account at your own risk.
