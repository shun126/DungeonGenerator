# Dungeon generator plugin for Unreal Engine

![DungeonGeneratorPlugin](Document/DungeonGenerator03.jpg)

One day I wanted to create a video game, but I didn't have the level design know-how. So I decided to create a procedural dungeon generator.
The dungeon generator was based on Vazgriz's algorithm. You can read more about [Vazgriz's algorithm here](https://vazgriz.com/119/procedurally-generated-dungeons/).

We had a try to use Unreal Engine 5 to create the demo, which can be used with any game engine by providing a Vector class, etc.

**NOTE** We are looking for proposals of visual expression from artists. The current implementation is a random choice for each static meshes. As you know, the pattern is visible and can not be looked like a natural terrain.

# Feature

* DungeonGenerator is a plug-in for UnrealEngine.
* It generates meshes along a grid.
* Users can easily generate dungeons by preparing meshes for floors, walls, ceilings, and stairs.
* Dungeons can be generated at runtime. Dungeons can also be generated statically.
* A mini-map of the dungeon can be generated.
* Generates actors for doors and keys by MissionGraph.

This is the screenshot of our sample game.

![DungeonGeneratorSampleGame](Document/DungeonGenerator02.gif)

Visualization of dungeon generation status.

![DungeonGeneratorStatus](Document/DungeonGenerator01.gif)

# Requirements
* [Unreal Engine 5](https://www.unrealengine.com/unreal-engine-5)
# Guide
* Put the DungeonGenerator folder in the PROJECT's Plugins folder.
  * In case of BluePrint project, an error occurs when packaging. Try converting your prohject to C++ project.
* Create DungeonGenerateParameter asset.
* Put a DungeonGenerateActor in the level.

We will add a detailed description to the Wiki

# License
* GPL-3.0

# Author
* [Shun Moriya](https://twitter.com/moriya_zx25r)
* [Nonbiri](https://www.youtube.com/channel/UCkLXe57GpUyaOoj2ycREU1Q)
