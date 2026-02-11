# UDungeonInteriorDatabase Guide

UDungeonInteriorDatabase is a **product-only database for interior themes**. It lets you assign meshes and actors that define the overall look of a dungeon’s rooms.

## Typical use
- Prepare themed interior sets (walls, floors, ceilings, props) and list them here.
- Reference this database in generation settings to apply the chosen interior set to rooms.

## Key UPROPERTY fields
- **InteriorData (`FDungeonInteriorData`, EditAnywhere/BlueprintReadOnly)**  
  The collection of interior assets and rules. Customize the meshes/actors and how they should be used.

## Editing tips
- Keep each interior set cohesive—use consistent materials and lighting-friendly meshes.
- If you need multiple moods, create several database assets and swap them per level or mission.
