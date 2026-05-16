# FDungeonRandomActorParts Reference

`FDungeonRandomActorParts` is the container used when you want multiple actor candidates at the same spawn point and want them to appear by probability. It inherits from `FDungeonActorPartsWithDirection`, so it keeps direction and offset data while adding `Frequency` to control how often it appears.

## Main usage
- Register shrine, treasure chest, or environmental-object variations that should appear only sometimes at the same location.
- Add them to random-actor arrays in generation settings so they are drawn during generation.

## What the UPROPERTY means
- **Frequency (`float`, EditAnywhere/BlueprintReadWrite, 0.0 to 1.0)**  
  Spawn probability. `1.0` means the actor appears every time, while `0.5` means a 50% chance. When multiple parts are listed, each is evaluated independently, so it is possible for more than one to appear. Lower values make an object feel rarer.

## Editing tips
- Register multiple actors with the same role and adjust `Frequency` to create effects such as "a rare object appears only occasionally."
- Use the base-class transform settings for rotation and position alignment. Matching the actor's forward direction makes placement feel more natural.

