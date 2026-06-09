# Codename: Extra Ferry Routes

Codename: Extra Ferry Routes is a mod for LBA2 Classic Community. It aims to add islands from LBA1 into LBA2 as an optional small side quest, reached through extra ferry routes rather than as a replacement for the vanilla game flow.

It also adds tomatoes as a new weapon for some reason. The current tomato work adds a new inventory item, text entries, and a replacement `OBJFIX.HQR` model entry through the mod data patch manifest.

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
