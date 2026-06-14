# Extra Ferry Routes

Extra Ferry Routes is a mod for LBA2 Classic Community. It aims to add islands from LBA1 into LBA2 as an optional small side quest.

It also adds tomatoes as a new weapon for some reason. The current tomato work adds a new inventory item, text entries, and a replacement `OBJFIX.HQR` model entry through the mod data patch manifest.

## Data patch workflow

EFR keeps three data layers separate:

- **Retail game files:** The user's original LBA2 data. These are input only.
- **Editable working data:** A throwaway data directory produced by applying patch manifests to retail data. The game/editor runs against this directory.
- **Distributable mod patch:** JSON manifests and mod-source edit files under `mods/`. This is the source of truth for EFR changes.

`scripts/dev/data_patch.py` is the installer/build step that turns the distributable mod patch into editable or playable game data. The in-game editor should not write patched HQR/ILE/OBL files directly as the source of truth. It should export small authoring edit files, which `data_patch.py` then applies.

Current manifests:

| Manifest | Purpose |
|----------|---------|
| `mods/efr.json` | Top-level EFR recipe. Includes tomatoes and the current Proxima workspace bootstrap. |
| `mods/efr_bootstrap.json` | Builds `PROXIMA.ILE` from the allocated cubes in `mods/proxima/island_layout.json`, copies the Citadel decor library for temporary reuse, and clones empty-water scene `94` into Proxima scene `223`. |
| `mods/proxima_island.json` | Proxima's ongoing authoring recipe. Citadel-derived prototype edits are preserved but disabled while blank-island authoring starts. |
| `mods/proxima/island_layout.json` | Explicit list of allocated Proxima terrain cube coordinates. The blank bootstrap starts with cube `8,8`. |
| `mods/proxima/terrain_edits.json`, `decor_edits.json`, `zone_edits.json` | Editor-source files for Proxima terrain, decor, and zone edits. |

The blank island builder reuses Citadel's ground and object texture HQR entries, deduplicates Citadel's island-wide terrain texture-definition pairs into each blank cube, and takes lighting metadata from the configured template cube. It generates zero-height water terrain and an empty decor-placement list for every allocated cube; no Citadel terrain geometry, decor placement, or zones are copied.

## Proxima authoring workflow

The current Proxima workflow is explicit and rebuild-based:

1. Start from clean retail data.
2. Run `scripts/dev/data_patch.py` with `mods/efr.json` or `mods/efr_bootstrap.json` to build a working data folder.
3. Run the game/editor against that working data folder.
4. Manually create `editor_output_dir.txt` in the normal LBA2 app-support folder. On macOS this is `~/Library/Application Support/Twinsen/LBA2/editor_output_dir.txt`.
5. Put the repo mod root in that file, for example `/Users/You/Projects/LBA2CC/mods`.

The editor treats `editor_output_dir.txt` as the root mods directory, not as an island-specific folder. When the current island is Proxima (`12`), editor output resolves to `<editor_output_dir>/proxima/`. Citadel Island (`0`) resolves to `<editor_output_dir>/citadel/`. Other islands currently refuse editor output with a clear log/status message.

With `LBA2CC_EDITOR` enabled, press `.` and `,` to cycle through Editor Off, Select/Move, Decor Place, Terrain Edit, and Texture Select. Tool changes briefly appear in the bottom-left. Press `D` to toggle the decor overlay, `Z` to toggle the zone overlay, and `I` to switch overlay detail between boxes only and boxes plus ids.

In Terrain Edit, select points along exactly one edge of the current cube and press `T` to create a `512`-thick Change Cube zone to the adjacent allocated scene. The new zone is selected, but the active tool remains Terrain Edit.

`Ctrl+S` is the intended editor save path. It validates `editor_output_dir.txt`, resolves island `12` to `mods/proxima/`, and writes runtime exterior decor and terrain edits to the island authoring JSON files. Island `0` resolves to `mods/citadel/`.

Exterior decor authoring is currently runtime-only, enough to start shaping Proxima in-game:

- Cycle to Select/Move with `.` or `,` to select exterior decor and related zones.
- Left-click an exterior decor object to select it in Select/Move.
- Press `Delete` or `Backspace` to delete the selected decor object.
- Switch to Decor Place, press `[` or `]` to change the simple Decor picker body id, and use left/right to rotate the preview by 90 degrees.
- Left-click or press `P` to place the picker body. `Insert` also works on keyboards that have it.
- Press `Ctrl+D` to duplicate the selected object.

Placement reuses a deleted decor slot in the current exterior cube when one exists. If no deleted slot exists, the editor appends a runtime decor object in memory and exports that appended object to `decor_edits.json`; runtime import appends it again while authoring. After placement it can be selected and moved with the existing arrow-key nudge controls. `Ctrl+S` persists these runtime decor edits into `mods/proxima/decor_edits.json`.

## Inventory expansion

EFR expands the inventory room used by the mod. Vanilla LBA2 has 40 inventory state entries, with visible inventory boxes ending before the scaphandre scenario flag. The mod raises `MAX_INVENTORY` to 46 and `MAX_BOX_INVENTORY` to 40, creating five new inventory slots:

| Slot | Purpose |
|------|---------|
| 41 | `FLAG_TOMATOES` |
| 42 | Reserved |
| 43 | Reserved |
| 44 | Reserved |
| 45 | Reserved |

Slot 40 remains `FLAG_SCAPHANDRE` and is still not a normal inventory box.

## Save compatibility

The extra inventory entries change the save-game context layout. EFR bumps `NUM_VERSION` from 36 to 37, so new mod saves use version byte `0x25` when uncompressed or `0xA5` when compressed.

The inventory block grows from 40 entries to 46 entries. Each entry is 10 bytes (`PtMagie`, `FlagInv`, `IdObj3D`), so the block grows from 400 bytes to 460 bytes and shifts later fields by 60 bytes.

To keep existing saves usable, the loader reads only the historical 40 inventory entries when loading saves older than layout 37, then initializes the new EFR slots from `InitTabInv`. Layout 37 saves read and write the full expanded inventory block. This is targeted compatibility for the inventory expansion only; unrelated save layout mismatches can still be unsafe.
