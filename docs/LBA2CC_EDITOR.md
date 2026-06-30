# LBA2CC_EDITOR

## Getting started

`DEBUG_TOOLS` must be enabled when building. I'm not sure how you're actually supposed to do so with CMake while using this repo's `make run` script, but it's possible to build once, then edit `build/CMakeCache.txt` and flip `DEBUG_TOOLS` from `OFF` to `ON`, then build again.

Create a text file `editor_output_dir.txt` next to your `lba2.cfg` file (e.g. in `~/Library/Application Support/Twinsen/LBA2/`) containing the full path to your desired output directory, e.g. `/Users/Baldino/Projects/lba2cc/mods`. The editor will then save edits to a JSON-based authoring format, e.g. at `mods/citadel/terrain_edits.json`.

Distributing mods works through `data_patch.py` and LBA2CC_EDITOR-specific patch manifest files. See the EFR branch for examples. The data patch manifest format is still under development and should not yet be considered stable.


### Settings location

| Platform | Base Path |
|----------|-----------|
| Linux | `~/.local/share/Twinsen/LBA2/` |
| macOS | `~/Library/Application Support/Twinsen/LBA2/` |
| Windows | `%APPDATA%\Twinsen\LBA2\` |


### Creating new islands

This is kind tedious right now. See the EFR branch for an example. The current approach is based on copying an empty exterior cube full of water to new cubes. When creating a new island, use the freecam mode with fog turned off to capture an image and related camera data for the island's holomap. 


## Hotkeys

| Key | Function |
|-----|----------|
| `.` / `,` | Cycle editor tool |
| `Esc` | Editor off / unfocus inspector | 
| `Ctrl+A` | Select all |
| `Enter` | Deselect all |
| `Shift+F` | Toggle editor fog |
| `Tab` | Toggle inspector focus |
| `Backtick` | Toggle inspector visibility |
| `Shift+Backtick` | Toggle free camera (move with WASD and Q/E) |
| `Ctrl+S` | Save editor authoring JSON |


### Camera hotkeys

| Key | Function |
|-----|----------|
| `1` | Front view |
| `3` | Right view |
| `7` | Top view |
| `5` | Cycle between perspective and orthographic views |
| `Alt+Shift+Arrows` | Pan |
| `-` / `Numpad -` | Zoom out |
| `=` / `Numpad +` | Zoom in |


### Free camera hotkeys

| Key | Function |
|-----|----------|
| `F` | Toggle fog |
| `P` | Save image and print camera properties to console |


### Select / Move tool hotkeys

| Key | Function |
|-----|----------|
| `Arrows` | Nudge selection |
| `Shift+Arrows` | Nudge selection 256-unit steps |
| `Alt+Arrows` | Nudge selection 1-unit steps |
| `Pg Up` / `Pg Down` | Up-axis nudge selection |
| `Delete` / `Backspace` | Delete selection |
| `N` | Open “New” menu (press A, W, or Z afterwards) |
| `P` / `Insert` | Place selected decor body |
| `Ctrl+D` | Duplicate selected exterior decor |
| `[` / `]` | Rotate selected |
| `A` | Toggle actor overlay |
| `C` | Toggle cube boundary overlay |
| `D` | Toggle decor overlay |
| `W` | Toggle waypoint (track points) overlay |
| `Z` | Toggle zone overlay |
| `I` | Toggle overlay identifiers |
| `Shift+C` | Toggle allocate cube mode |


### Place Decor tool hotkeys

| Key | Function |
|-----|----------|
| `[` / `]` | Change decor body |
| `Shift+[` / `Shift+]` | Change decor library |


### Edit Terrain tool hotkeys

| Key | Function |
|-----|----------|
| `Arrows` | Up-axis nudge selected terrain |
| `T` | Create teleport zone at selected cube border points |


### Texture Terrain tool hotkeys

| Key | Function |
|-----|----------|
| `Q` | Cycle between quad and triangles modes |
| `[` / `]` | Cycle between palette pages |
| `L]` | Log selected terrain textures to console |


### Auto Terrain tool hotkeys

| Key | Function |
|-----|----------|
| `F` | Cycle between freehand and rect fill modes |
| `M` | Cycle between paint mode and select mode |
| `Delete` / `Backspace` | Delete selection |


## Scripting

The data patch tooling includes a scene script compiler and decompiler, based on LBArchitect's source format for life and track scripts. A bulk decompilation mode exists, making it easy to search through all scripts from the retail data for patterns. The decompiler maps text bank messages and community-sourced game variable descriptions into the decompilation output.


## PERSO-Edit 26

The editor suite includes a model editor, known as PERSO-Edit 26, accessible under the _Editing_ item in the main menu.


### PERSO-Edit 26 hotkeys

See the source code for hotkeys.


## Philosophy

Source code changes are required for the type of mods that sparked the creation of this editor. Nevertheless, the editor is written with the principle in mind that most things should work on the vanilla version of the game. For example, while the amount of islands in the engine is hardcoded, it should be possible to use the editor to replace existing islands. Similarly, while we could have chosen to extend the engine to natively support auto-texture regions, we instead chose a baking method that produces compatible terrain data.

**Note:** For ease of experimentation during mod-authoring, the engine has been extended to support rendering exterior decor objects from other libraries, e.g. placing Desert Island decors on Citadel Island. However, this could eventually be replaced with a bake method as well that would copy over decors between libraries. So for now, if you plan to distribute your mod for the vanilla version of the game, don't add decors from other islands to your scene.


## Roadmap

- A clean "New Island" mode eventually.
- Camera bookmark for capturing holomap images so the view can be consistent while authoring an island.
- Improvements to auto terrain.

