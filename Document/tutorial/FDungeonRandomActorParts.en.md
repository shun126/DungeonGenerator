# FDungeonRandomActorParts Reference

`FDungeonRandomActorParts` stores an Actor candidate with direction, offset, and an easy-to-read percentage chance.

## Main property

- `Spawn Chance` (`float`, 0% to 100%): Chance that this Actor participates in one selection. `0%` never participates and `100%` always participates.

Spawn Chance is not a selection weight. It decides whether this entry participates; a weighted selector decides which participating entry is preferred.

## Editing tips

- Use a low Spawn Chance to mix rare props into common decoration.
- Use the inherited transform settings to align the Actor's forward direction and placement offset.
