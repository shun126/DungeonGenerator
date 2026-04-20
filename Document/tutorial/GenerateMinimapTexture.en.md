# Generate Minimap Textures

This page explains generated dungeon minimaps in two ways:  
**saving them as texture assets** and **showing them in a widget at runtime**.

For the first pass, start by confirming that you can generate a texture asset in the editor.  
After that, move on to runtime UI display and icon overlays.

## Goal
- Generate minimap textures in the editor
- Display the minimap in UI at runtime
- Overlay icons when needed

## What to Know First
- To generate a texture in the editor, you must have a **dungeon generated immediately beforehand**
- To show it at runtime, get the minimap object from `ADungeonGenerateActor`
- For easier debugging, confirm the texture first and add icons later

## Prerequisites
- [QuickStart.en.md](./QuickStart.en.md) is complete
- You can generate a dungeon successfully
- You can preview from `Window > DungeonGenerator`, or you already use `ADungeonGenerateActor` in a level

## A. Create Minimap Texture Assets in the Editor
Start by creating minimap textures that are saved into the Content Browser.  
This is useful for checking the look and prototyping UI.

### 1. Generate a Dungeon First
Minimap textures are created from the **most recently generated dungeon data**.  
If you have not generated a dungeon yet, run `Generate dungeon` first.

### 2. Confirm That the Buttons Are Enabled
The following buttons are available only when valid dungeon generation data exists.

- `Generate texture with size`
- `Generate texture with scale`

If the buttons are disabled, the dungeon may not have been generated successfully.

![Texture generation window](images/MiniMap1.png)

### 3. Export the Texture
Use either button depending on what you want to control.

- `Generate texture with size`  
  Use this when you want to decide the output size directly
- `Generate texture with scale`  
  Use this when you want to control output density instead

### 4. Check the Output Location
When you press the button, a texture asset is created in the `ProceduralTextures` folder in the Content Browser.

![](images/MiniMap2.png)

### 5. Check the Per-Floor Result
The minimap is generated as **a separate texture for each floor**.  
This helps players understand multi-floor dungeons more easily.  
Lower floors are shown in lighter colors.

## B. Show the Minimap in a Widget at Runtime
If you generate the dungeon during gameplay, you can display the minimap directly in UI without saving texture assets.

### 1. Get the Minimap Object After Dungeon Generation
After `ADungeonGenerateActor` generates the dungeon,  
you can obtain a `DungeonMinimapTextureLayer` object.

![](images/MiniMap3.png)

### 2. Select the Texture for the Current Floor
Use `DungeonMinimapTextureLayer` to get the texture that matches the player's current height.

![](images/MiniMap4.png)

### 3. Assign It to a Widget
Set the retrieved texture on a widget brush and display it.

![](images/MiniMap5.png)

## C. Overlay Icons
Adding icons for the player or targets makes exploration easier.  
Use `DungeonIconWidget` to help display these icons.

### 1. Prepare the Icon You Want to Show
First, set the icon you want to display on the brush.

![](images/MiniMapIcon1.png)

![](images/MiniMapIcon2.png)

### 2. Register the Icon
Use `Register or Set` to register the display location.  
If the same ID already exists, it can update the existing icon.

### 3. Unregister the Icon
Use `Unregister` when the icon is no longer needed.

![](images/MiniMapIcon3.png)

## Verify the Result
- Pressing the texture generation button creates assets in `ProceduralTextures`
- Separate textures exist for separate floors
- The minimap changes to the correct floor based on player height
- Icons appear in the expected positions

## Common Mistakes
- `Generate texture with size` or `Generate texture with scale` is disabled  
  Make sure a dungeon was generated immediately beforehand
- The texture was created, but nothing appears in the UI  
  Make sure the texture retrieved from `DungeonMinimapTextureLayer` is actually assigned to the widget
- The displayed floor is wrong  
  Recheck the logic that maps player height to floor selection
- The icon does not appear  
  Recheck the `Register or Set` ID, display coordinates, and brush setup

## Notes
For a concrete integration example, see `Content/Widget/Main/PlayGameWidget` in the sample project.

## Read Next
- [ADungeonGenerateActor.en.md](./ADungeonGenerateActor.en.md)
- [QuickStart.en.md](./QuickStart.en.md)
