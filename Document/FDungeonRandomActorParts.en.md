# FDungeonRandomActorParts Guide

FDungeonRandomActorParts collects **one or more actor assets and picks one at random** when the dungeon is generated. Use it for props like torches, crates, or decorative pieces that should vary.

## Typical use
- Register several Actor/Blueprint assets, then let the system choose one for each placement slot.
- Place the part inside a mesh or actor set so different props can appear across rooms and corridors.

## Key UPROPERTY fields
- **Actors (`TArray<UClass*>`, EditAnywhere/BlueprintReadWrite)**  
  The list of actor classes to choose from. Empty entries are skipped. All items in the list are treated with equal probability.

## Editing tips
- Keep the list short and focused (e.g., 3–5 options) so randomness remains controlled and easy to test.
- If an actor needs specific offsets or rotation, handle it inside the actor Blueprint itself so the placement stays consistent.
