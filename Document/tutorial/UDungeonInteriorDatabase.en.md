# UDungeonInteriorDatabase Guide

`UDungeonInteriorDatabase` is the database used to spawn furniture, decoration, and vegetation by tag.  
Use it when you want rooms to feel different from each other, or when only tagged rooms should receive extra furniture or plants.

## What it manages
- `Interior Parts`  
  Interior parts spawned as actors, such as furniture and decoration
- `VegetationParts`  
  Vegetation-style decoration such as grass, vines, and foliage

## Tag flow
Interior selection happens when tags returned from the room side match tags configured in the database.  
The two main input sources are:

- `ADungeonRoomSensorBase::GetInquireInteriorTags`
- `UDungeonInteriorLocationComponent::InquireInteriorTags`

For example, if the room side returns `library` and an interior part in the database also has `library`, that furniture becomes a candidate.

## Main properties
- `Interior Parts`
  - Actor class for furniture or decoration
  - System tags
  - Additional tags
  - Spawn frequency
  - Overlap check
  - Spawn method
- `VegetationParts`
  - Mesh for vegetation
  - Tags
  - Density
  - Slope conditions
  - Culling distance

## Why `Build` is required
`Interior Parts` precompute actor bounding-box information when `Build` is run.  
If you change furniture size or actor classes and skip `Build`, placement checks may still use outdated information.

## Typical flow
1. Register furniture or decoration Blueprint / C++ actors in `Interior Parts`.
2. Register vegetation in `VegetationParts` if needed.
3. Return the tags you want to use from the room side or from `DungeonInteriorLocationComponent`.
4. Run `Build`.
5. Assign the asset to `DungeonInteriorDatabase` in `UDungeonGenerateParameter`.

## Editing tips
- Start with broad tags such as `start`, `goal`, and `hall`. They are easier to manage.
- Add narrower tags such as `kitchen` or `library` later as the layout grows.
- Edit `Interior Parts` when you only want to swap furniture, and `VegetationParts` when you only want to change plants.

## Read Next
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)  
  Review how room events can provide interior tags.
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)  
  Review where this database is assigned in the main settings asset.

## Related Pages
- [ADungeonRoomSensorBase.en.md](./ADungeonRoomSensorBase.en.md)
- [UDungeonGenerateParameter.en.md](./UDungeonGenerateParameter.en.md)

