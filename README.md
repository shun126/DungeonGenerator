# Dungeon generator plugin for Unreal Engine
> Easy generation of levels, mini-maps and missions.

[![license](https://img.shields.io/github/license/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/blob/main/LICENSE)
[![release](https://img.shields.io/github/v/release/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/releases)
[![downloads](https://img.shields.io/github/downloads/shun126/DungeonGenerator/total)](https://github.com/shun126/DungeonGenerator/releases)
[![stars](https://img.shields.io/github/stars/shun126/DungeonGenerator?style=social)](https://github.com/shun126/DungeonGenerator/stargazers)

![DungeonGeneratorPlugin](https://github.com/shun126/DungeonGenerator/raw/main/Document/DungeonGenerator03.jpg)

# 🚩 Table of Contents
- [Why Dungeon Generator?](#-why-dungeon-generator)
- [Features](#-features)
- [Demo](#-demo)
- [Requirements](#-requirements)
- [License](#-license)
- [Author](#-authors)

# 🤔 Why Dungeon Generator?

One day I wanted to create a video game, but I didn't have the level design know-how. So I decided to create a procedural dungeon generator.
The dungeon generator was based on Vazgriz's algorithm. You can read more about [Vazgriz's algorithm here](https://vazgriz.com/119/procedurally-generated-dungeons/).

## Visualization of dungeon generation status

![DungeonGeneratorStatus](https://github.com/shun126/DungeonGenerator/raw/main/Document/DungeonGenerator01.gif)

# 🎨 Features

* DungeonGenerator is a plug-in for UnrealEngine.
* Tiled Dungeon Generation both In-Editor & Runtime
* Users can easily generate dungeons by preparing meshes for floors, walls, ceilings, and stairs.
* A mini-map of the dungeon can be generated. [beta version]
* Generates actors for doors and keys by MissionGraph. [beta version]
* Supported Development Platforms: Windows (should be possible to build on all platforms)
* Supported Target Build Platforms: Windows (should be possible to target all platforms)

# 👾 Demo

[DungeonGenerator Demo](https://github.com/shun126/DungeonGeneratorDemo) is a BluePrint sample project for first-person exploration.

![DungeonGeneratorDemo Screenshot](https://github.com/shun126/DungeonGeneratorDemo/raw/main/Document/Screenshot.gif)

This is an easy to use. Simply drop the DungeonGenerateActor into your level, set the grid scale and number of rooms and start generating out your structures. Please read the [Wiki](https://github.com/shun126/DungeonGenerator/wiki) for more information.

# 🔧 Requirements
* [Unreal Engine 5.1.1](https://www.unrealengine.com/unreal-engine-5)
* [Visual Studio 2022](https://visualstudio.microsoft.com/)

# 📜 License
* GPL-3.0

[UnrealEngine marketplace](https://www.unrealengine.com/marketplace/slug/36a8b87d859f44439cfe1515975d7197) is releasing it under Epic license. If you need a license other than the GPL, please consider it. Proceeds will be used to fund the development of our game.

## This is the screenshot of our game

![Out game Screenshot](https://github.com/shun126/DungeonGenerator/raw/main/Document/DungeonGenerator02.gif)

# 👾 Authors
* [Shun Moriya](https://twitter.com/moriya_zx25r)
* [Nonbiri](https://www.youtube.com/channel/UCkLXe57GpUyaOoj2ycREU1Q)
