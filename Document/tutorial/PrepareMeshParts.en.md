# Prepare Mesh Parts

This page explains **how to prepare mesh parts for the dungeon without guesswork**.  
You do not need many variations at first. Start by preparing these four types: **floor, wall, roof, and slope**. The goal is to get them registered with the correct orientation and pivot.

## Goal
- Understand the basic parts that must be registered in the room and aisle `Mesh set database`
- Understand how orientation, pivot, and thickness affect placement
- Create meshes that are less likely to break when you preview generation

## What to Prepare First
For the first check, you only need the following four types.

- `Floor Parts`
- `Wall Parts`
- `Roof Parts`
- `Slope Parts`

With these four, you can already display the dungeon floor, walls, ceilings, and height transitions.  
The following parts can be added later.

- Indoor mezzanine floor
- Indoor stairs and slopes
- Pillars
- Torches
- Doors

## Assumptions
This page uses the **plugin sample assets with a 4 meter voxel size** as the example.

![PluginAssets](images/PluginAssets.png)

When you build a mesh, pay attention to these three points first.

- **Orientation**  
  Which axis is treated as the front
- **Pivot**  
  Which point is used as the placement reference
- **Thickness**  
  If the mesh is too thin, physics objects can pass through it more easily

When the result looks misaligned, the cause is usually not the mesh shape itself.  
It is more often an **orientation or pivot mismatch**.

## 1. Create the Floor
The floor is the basic part that creates the walkable surface.  
It is required for both rooms and aisles.

### How It Is Used
- It is used as the floor for one grid cell
- During generation, it is placed on the **bottom face of the grid**

### Rules for Building It
- Make the **Z axis point upward**
- Put the **pivot at the center of the surface**
- Give it **enough thickness** to prevent physics objects from falling through

![BaseFloor-en](images/BaseFloor1.png)

### Common Mistakes
- The mesh is too thin, so characters or physics objects fall through
- The pivot is on an edge, so the part is offset by half a cell
- The upward direction is wrong, so placement rotates in an unexpected way

## 2. Create the Wall
The wall creates the boundary of rooms and aisles.  
Like the floor, it is required for the first generation test.

### How It Is Used
- It is used as a wall standing on the side of a voxel
- During generation, it is positioned relative to the **top face of the voxel**

### Rules for Building It
- Make the **Y axis point forward**
- Put the **pivot at the center of the bottom face**

![BaseWall-en](images/BaseWall1.png)

### Common Mistakes
- The pivot is not centered, so the wall clips too far forward or backward
- The front direction is reversed, so decorative front/back details are flipped

## 3. Create the Roof
The roof is used for ceilings and top surfaces.  
Even if you eventually want a more open look, it helps to prepare one roof part for the first test so the result is easier to read.

### How It Is Used
- It is used to close the top side of rooms and aisles

### Rules for Building It
- Make the **Y axis point forward**
- Use **north-facing wall as 0 degrees, east as 90 degrees, south as 180 degrees, and west as -90 degrees**
- Put the **pivot at the center of the bottom face**
- Give it **enough thickness** to prevent physics objects from falling through

![BaseRoof-en](images/BaseRoof1.png)

### Common Mistakes
- The front direction does not match the expected orientation, so patterns or slopes do not line up
- The mesh is too thin, so objects falling from above pass through it

## 4. Create the Slope
The slope is used for aisles and interior movement with height differences.  
If your dungeon includes vertical movement, this part is required.

### How It Is Used
- It is used as a shape that **moves forward 2 grid cells and rises by 1 grid cell**
- The slope angle is **22.5 degrees**

### Rules for Building It
- Make the **Y axis point forward**
- Put the **pivot at the center of the bottom face of the first grid cell**
- Give it **enough thickness** to prevent physics objects from falling through

![BaseSlope-en](images/BaseSlope1.png)

### Common Mistakes
- Building it as a 1 cell forward / 1 cell up shape, which does not match the expected slope
- Placing the pivot too close to the center, so the slope does not connect cleanly to the floor
- Reversing forward and travel direction, so the slope goes up the wrong way

## 5. Additional Mesh Parts You Can Add Later
Everything below is easier to add **after** the first generation preview works.  
If you confirm the minimum setup first, it is much easier to isolate what caused a problem.

### Indoor Mezzanine Floor
When the plugin generates an indoor slope, it can use a separate mesh for the mezzanine floor section.  
If you add a handrail, build it with the assumption that it is attached on the **Y+ side**.

![image](images/BaseFloor2.png)

### Indoor Stairs and Slopes
Indoor slopes use a separate mesh from aisle slopes.  
Because the outer shape is not uniform indoors, it is usually easier to design them with **handrails on both sides**.

![image](images/BaseSlope2.png)

## 6. Additional Actor Parts You Can Add Later
If you want more visual variety or gameplay presentation, you can add actors as well as static meshes.  
However, they are not required for the first preview.

### Pillar
- Spawned between walls
- The **Y axis points forward**
- The **pivot is at the center of the bottom face**

![image](images/BasePillar1.png)

### Torch (Pillar Light)
- Spawned on walls
- The **Y axis points forward**
- The **pivot is at the center of the bottom face**

### Door
- Spawned in the same location as walls
- The **X axis points forward**
- The **pivot is at the center of the bottom face**, and it is generated on the **top face of the voxel**

![BaseDoor-en](images/BaseDoor1.png)

## 7. Register Them in `Mesh set database`
After creating the meshes, register them in the room and aisle `Mesh set database`.  
At first, it is enough to put the following into a single `Mesh Set`.

- Register the floor mesh in `Floor Parts`
- Register the wall mesh in `Wall Parts`
- Register the roof mesh in `Roof Parts`
- Register the slope mesh in `Slope Parts`

Add indoor floors, indoor slopes, pillars, or doors later, only when you need them.  
If you try to prepare everything at once, it becomes harder to find the source of alignment problems.

## 8. Verify the Result
After registration, preview the dungeon from `Window > DungeonGenerator`.

### What to Check
- Whether the floor aligns cleanly to the grid
- Whether the wall stands correctly on the edge of the floor
- Whether the roof orientation matches the wall orientation
- Whether the slope connects naturally to the floor

### If Something Is Wrong
- The position is wrong  
  Recheck the pivot location
- The orientation is wrong  
  Recheck which axis is treated as the front
- Objects pass through  
  Recheck mesh thickness and collision

## Common Mistakes
- One of floor, wall, roof, or slope is missing, so the generated shape does not look correct
- The pivot is offset, so the mesh floats or sinks by half a grid cell
- The slope length or height does not match the expected shape, so it does not connect to the floor
- Too many decorative actors are added before the basic parts are verified

## Read Next
- [StaticMeshFitTool.en.md](./StaticMeshFitTool.en.md)
- [QuickStart.en.md](./QuickStart.en.md)
- [UDungeonMeshSetDatabase.en.md](./UDungeonMeshSetDatabase.en.md)
- [FDungeonMeshParts.en.md](./FDungeonMeshParts.en.md)
