<div align="center">
<img src="Document/DungeonGenerator-Logo.png" />
<h1>Dungeon generator plugin for Unreal Engine 5</h1>
</div>

[![license](https://img.shields.io/github/license/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/blob/main/LICENSE)
[![Unreal Engine Supported Versions](https://img.shields.io/badge/Unreal_Engine-5.1~5.6-9455CE?logo=unrealengine)](https://www.unrealengine.com/)
[![release](https://img.shields.io/github/v/release/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/releases)
[![downloads](https://img.shields.io/github/downloads/shun126/DungeonGenerator/total)](https://github.com/shun126/DungeonGenerator/releases)
[![stars](https://img.shields.io/github/stars/shun126/DungeonGenerator?style=social)](https://github.com/shun126/DungeonGenerator/stargazers)
[![youtube](https://img.shields.io/youtube/views/1igd4pls5x8?style=social)](https://youtu.be/1igd4pls5x8)

Please visit our website for full feature list: [https://happy-game-dev.undo.jp/](https://happy-game-dev.undo.jp/plugins/DungeonGenerator/index.html)

![Screenshot](Document/Screenshot.gif)

# 🚩 Table of Contents
- [Endless Adventures, Instantly Generated](#-endless-adventures-instantly-generated)
- [Features](#-features)
- [Requirements](#-requirements)
- [License](#-license)
- [Demo](#-demo)
- [See also](#-see-also)
- [Author](#-authors)

# 👾 Endless Adventures, Instantly Generated
Bring your roguelike and hack-and-slash games to life with next-level dungeon generation.
Whether you’re aiming for fast-paced, loot-filled battles or deep exploration with infinite replayability,
this Unreal Engine plugin gives you the power to craft unique dungeons every time.

With just a few parameters—such as grid scale, number of rooms, and your own generation rules—you can instantly produce fully playable layouts. The system works seamlessly both in-editor and at runtime, and is available in both C++ and Blueprint, giving you the flexibility to integrate it into any workflow.

## Visualization of dungeon generation status
The dungeon generator was based on Vazgriz's algorithm. You can read more about [Vazgriz's algorithm here](https://vazgriz.com/119/procedurally-generated-dungeons/).

To vazgriz,
The foundational generation algorithm you shared was a major source of inspiration and became the starting point for my development of the DungeonGenerator plugin. Without your ideas, I don't think I would have arrived at its current form. I'm truly grateful for the knowledge you made available and the impact it had on my work.

![DungeonGeneratorStatus](Document/DungeonGenerator01.gif)

# 🎨 Features
* DungeonGenerator is a plug-in for UnrealEngine5.
* Tiled Dungeon Generation both In-Editor & Runtime.
* Users can easily generate dungeons by preparing meshes for floors, walls, ceilings, and stairs.
* Supports dungeon replication
* Generates actors for doors and keys by MissionGraph.
* The following features are supported only in the [Fab](https://fab.com/s/f5587c55bad0) version
  * Sub-levels can be applied as dungeon rooms
  * A mini-map of the dungeon can be generated.
  * Interior decoration. [beta version]
  * Foliage decoration. [beta version]
* Supported Development Platforms: Windows,Android,Linux
* Supported Target Build Platforms: Windows,Android,Linux (should be possible to target all platforms)

# 🔧 Requirements
* [Unreal Engine 5.1 ~ Unreal Engine 5.6](https://www.unrealengine.com/)
* [Visual Studio 2022](https://visualstudio.microsoft.com/)

# 💾 How to install
* Open Unreal Engine Editor and create a project using the First Person template or Third Person template.
* Install the Dungeon Generator plugin via Epic Games Launcher, or copy it to the `Plugins` directory of your project.
* Enable the plugin content.
* Open `Plugins/Dungeon Generator/Contents/Maps/Demonstration`.

![](Document/ContentBrowser.gif)

* Run the project.

# 📜 License
you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied arranty of MERCHANTABILITY or FITNESS FOR A ARTICULAR PURPOSE. See the GNU General Public License for more details.

Or, [Fab](https://fab.com/s/f5587c55bad0) is releasing it under Epic license. If you need a license other than the GPL, please consider it. Proceeds will be used to fund the development of our game.

# 👀 See also
* [Issues](https://github.com/shun126/UE5-DungeonGeneratorDemo/issues)
* [Discussions](https://github.com/shun126/UE5-DungeonGeneratorDemo/discussions)
* [Wiki](https://github.com/shun126/UE5-DungeonGeneratorDemo/wiki)
* [DeepWiki](https://deepwiki.com/shun126/DungeonGenerator)
* [Doxygen](https://happy-game-dev.undo.jp/_doxygen/dungeon_generator/index.html)

The [Fab](https://fab.com/s/f5587c55bad0) version includes the following enhancements.
* Sub-levels can be applied as dungeon rooms
* A mini-map of the dungeon can be generated.
* Interior decoration. [beta version]
* Foriage decoration. [beta version]

## 🎮 Demo
[DungeonGenerator Demo](https://github.com/shun126/UE5-DungeonGeneratorDemo) is a sample project for first-person exploration.

This is an easy to use. Simply drop the DungeonGenerateActor into your level, set the grid scale and number of rooms and start generating out your structures. Please read the [Wiki](https://github.com/shun126/UE5-DungeonGeneratorDemo/wiki) for more information.

Please visit our website for full feature list: [https://happy-game-dev.undo.jp/](https://happy-game-dev.undo.jp/plugins/DungeonGenerator/index.html)

# 😀 Authors
* Nonbiri ([X.com](https://x.com/happy_game_dev) / [YouTube](https://www.youtube.com/channel/UCkLXe57GpUyaOoj2ycREU1Q))
* Shun Moriya ([X.com](https://x.com/monjiro1972))

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/M4M413XDXB)
