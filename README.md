# Dungeon Generator for Unreal Engine 5

[![license](https://img.shields.io/github/license/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/blob/main/LICENSE)
[![Unreal Engine Supported Versions](https://img.shields.io/badge/Unreal_Engine-5.1~5.8-9455CE?logo=unrealengine)](https://www.unrealengine.com/)
[![release](https://img.shields.io/github/v/release/shun126/DungeonGenerator)](https://github.com/shun126/DungeonGenerator/releases)
[![downloads](https://img.shields.io/github/downloads/shun126/DungeonGenerator/total)](https://github.com/shun126/DungeonGenerator/releases)
[![stars](https://img.shields.io/github/stars/shun126/DungeonGenerator?style=social)](https://github.com/shun126/DungeonGenerator/stargazers)
[![Discord](https://img.shields.io/discord/768025826008498216)](https://discord.com/invite/Z5JWmk4X8J)
[![youtube](https://img.shields.io/youtube/views/1igd4pls5x8?style=social)](https://youtu.be/1igd4pls5x8)

Create replayable 3D dungeons with branching routes, keys, boss encounters, secrets, and room-by-room gameplay roles.

Dungeon Generator turns your mesh parts and a small set of parameters into playable Unreal Engine 5 dungeon layouts. Preview results in the editor, generate dungeons at runtime, and connect the generated room information to Blueprint or C++ gameplay.

![Screenshot](Document/Screenshot.gif)

Want to try it first? Start with the [demo project](https://github.com/shun126/UE5-DungeonGeneratorDemo) or follow the [Quick Start](Document/tutorial/QuickStart.en.md).

## Why Use It?

- Prototype roguelike, action RPG, dungeon crawler, and exploration layouts quickly
- Try open exploration, clear start-to-goal routes, key-and-lock progression, boss routes, and hub-style quests
- Turn generated rooms into combat, treasure, puzzle, rest, boss, and secret spaces
- Use your own floor, wall, roof, stair, door, and actor meshes
- Preview dungeons in the editor, then generate them during gameplay
- Connect generated room data to Blueprint or C++ systems
- Test multiplayer-oriented projects with dungeon replication support

Dungeon Generator is useful when you want a playable dungeon structure quickly, but still need enough control to shape routes, pacing, room identity, and visual style.

## Choose the Progression Style That Fits Your Game

Use `Path.ProgressionPolicy` to shape the player's route: open exploration with loops, a clear start-to-goal path, key-and-lock progression, a boss build-up route, or a hub quest layout with branches.

![Progression policy styles](Document/ProgressionPolicyStyles.png)

## Design Rooms Around Player Experience

Use `Gameplay.RoomRoles` to mark rooms as combat, treasure, puzzle, rest, boss, or secret spaces, then connect those roles to your own sensors, events, visuals, interiors, fixtures, and Blueprint logic.

![Room gameplay role styles](Document/RoomGameplayRoleStyles.png)

## Epic/Fab Version

The [Epic/Fab version](https://fab.com/s/f5587c55bad0) includes additional production-focused features for teams that need more advanced level-building workflows.

Additional features include:

- Sub-levels as dungeon rooms
- Mini-map generation
- Interior decoration
- Foliage decoration
- Mesh Set and Custom Mesh Selection
- StaticMesh Fit Tool

Important: The StaticMesh Fit Tool is supported only in the Epic/Fab version. [![](Document/Fab_Epic_Games.gif)](https://fab.com/s/f5587c55bad0)

If you need these features or a license other than GPL, please consider the [Epic/Fab version](https://fab.com/s/f5587c55bad0).

Please visit our website for full feature list: [https://happy-game-dev.undo.jp/](https://happy-game-dev.undo.jp/plugins/DungeonGenerator/index.html)

## Open-Source Version

This repository provides the open-source version of Dungeon Generator under the GPL license.

It is a good fit if you want to:

- Try procedural dungeon generation in Unreal Engine 5
- Learn how a grid-based 3D dungeon generator is built
- Prototype roguelike, hack-and-slash, dungeon crawler, or exploration mechanics
- Customize the plugin source for your own project

For the fastest path, start with the tutorial index:

[Dungeon Generator Tutorial](Document/tutorial/README.en.md)

## Main Features

- Procedural 3D dungeon generation for Unreal Engine 5
- Editor generation and runtime generation
- Blueprint and C++ access
- Custom mesh parts for floors, walls, roofs, slopes, pillars, doors, and actors
- MissionGraph support for doors, keys, and progression routes
- Dungeon replication support
- Demo content and tutorial documentation

## Quick Start

1. Install the plugin in your Unreal Engine project.
2. Enable the plugin content.
3. Open the demonstration map from the plugin content.
4. Run the project or preview generation from the editor.
5. Create a `DungeonGenerateParameter` asset and adjust the grid size, room count, and mesh databases.

For a beginner-friendly walkthrough, read:

[QuickStart.en.md](Document/tutorial/QuickStart.en.md)

## Using Your Own Meshes

Dungeon Generator is built around reusable mesh parts. You prepare meshes for floors, walls, roofs, slopes, and other dungeon pieces, then register them in mesh databases.

Start here if you want to replace the sample visuals with your own assets:

[PrepareMeshParts.en.md](Document/tutorial/PrepareMeshParts.en.md)

## Demo Project

The demo project shows a first-person exploration setup using Dungeon Generator:

[DungeonGenerator Demo](https://github.com/shun126/UE5-DungeonGeneratorDemo)

## Requirements

- Unreal Engine 5.1 to 5.8
- Visual Studio 2022

## License

The open-source version is distributed under the GNU General Public License v3.0 or later.

The Epic/Fab version is released under the Epic license. If GPL does not fit your project, use the Epic/Fab version instead.
