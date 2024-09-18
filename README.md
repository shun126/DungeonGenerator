# Dungeon generator plugin for Unreal Engine 5
> Easy generation of levels, mini-maps and missions.

[![Unreal Engine Supported Versions](https://img.shields.io/badge/Unreal_Engine-5.1~5.4-9455CE?logo=unrealengine)](https://www.unrealengine.com/)
[![license](https://img.shields.io/github/license/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/blob/main/LICENSE)
[![release](https://img.shields.io/github/v/release/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/releases)
[![downloads](https://img.shields.io/github/downloads/shun126/DungeonGenerator/total)](https://github.com/shun126/DungeonGenerator/releases)
[![stars](https://img.shields.io/github/stars/shun126/DungeonGenerator?style=social)](https://github.com/shun126/DungeonGenerator/stargazers)
[![youtube](https://img.shields.io/youtube/views/HIW4mRt2_AA?style=social)](https://youtu.be/HIW4mRt2_AA)

![DungeonGeneratorPlugin](https://github.com/shun126/DungeonGenerator/raw/main/Document/DungeonGenerator03.jpg)

Trailer: [YouTube](https://youtu.be/HIW4mRt2_AA)

# 🚩 Table of Contents
- [Why Dungeon Generator?](#-why-dungeon-generator)
- [Features](#-features)
- [Requirements](#-requirements)
- [License](#-license)
- [Demo](#-demo)
- [See also](#-see-also)
- [Author](#-authors)

# 🤔 Why Dungeon Generator?
One day I wanted to create a video game, but I didn't have the level design know-how. So I decided to create a procedural dungeon generator.
The dungeon generator was based on Vazgriz's algorithm. You can read more about [Vazgriz's algorithm here](https://vazgriz.com/119/procedurally-generated-dungeons/).

## Visualization of dungeon generation status

![DungeonGeneratorStatus](https://github.com/shun126/DungeonGenerator/raw/main/Document/DungeonGenerator01.gif)

# 🎨 Features
* DungeonGenerator is a plug-in for UnrealEngine5.
* Tiled Dungeon Generation both In-Editor & Runtime.
* Users can easily generate dungeons by preparing meshes for floors, walls, ceilings, and stairs.
* Supports dungeon replication
* Generates actors for doors and keys by MissionGraph.
* The following features are supported only in the [Unreal Engine marketplace](https://www.unrealengine.com/marketplace/slug/36a8b87d859f44439cfe1515975d7197) version
  * Sub-levels can be applied as dungeon rooms
  * A mini-map of the dungeon can be generated.
  * Interior decoration. [beta version]
  * Foliage decoration. [beta version]
* Supported Development Platforms: Windows,Android
* Supported Target Build Platforms: Windows,Android (should be possible to target all platforms)

# 🔧 Requirements
* [Unreal Engine 5.1 ~ Unreal Engine 5.4](https://www.unrealengine.com/)
* [Visual Studio 2022](https://visualstudio.microsoft.com/)

# 📜 License
you can redistribute it and/or modify it under the terms of the GNU General Public License as published by the Free Software Foundation, either version 3 of the License, or any later version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied arranty of MERCHANTABILITY or FITNESS FOR A ARTICULAR PURPOSE. See the GNU General Public License for more details.

Or, [Unreal Engine marketplace](https://www.unrealengine.com/marketplace/slug/36a8b87d859f44439cfe1515975d7197) is releasing it under Epic license. If you need a license other than the GPL, please consider it. Proceeds will be used to fund the development of our game.

The [Unreal Engine marketplace](https://www.unrealengine.com/marketplace/slug/36a8b87d859f44439cfe1515975d7197) version includes the following enhancements.
* Sub-levels can be applied as dungeon rooms
* A mini-map of the dungeon can be generated.
* Interior decoration. [beta version]
* Foriage decoration. [beta version]

# 👾 Demo
[DungeonGenerator Demo](https://github.com/shun126/UE5-DungeonGeneratorDemo) is a BluePrint sample project for first-person exploration.

![DungeonGeneratorDemo Screenshot](https://github.com/shun126/UE5-DungeonGeneratorDemo/raw/main/Document/Screenshot.gif)

This is an easy to use. Simply drop the DungeonGenerateActor into your level, set the grid scale and number of rooms and start generating out your structures. Please read the [Wiki](https://github.com/shun126/UE5-DungeonGeneratorDemo/wiki) for more information.

# 👀 See also
* [Issues](https://github.com/shun126/UE5-DungeonGeneratorDemo/issues)
* [Discussions](https://github.com/shun126/UE5-DungeonGeneratorDemo/discussions)
* [Wiki](https://github.com/shun126/UE5-DungeonGeneratorDemo/wiki)
* [UnrealEngine marketplace](https://www.unrealengine.com/marketplace/slug/36a8b87d859f44439cfe1515975d7197)

# 👾 Authors
* [Nonbiri](https://www.youtube.com/channel/UCkLXe57GpUyaOoj2ycREU1Q)
* [Shun Moriya](https://twitter.com/moriya_zx25r)
