#ifndef TERRAIN_REGION_EDITOR_H
#define TERRAIN_REGION_EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR
extern void TerrainRegionEditorSetSelect(S32 select);
extern S32 TerrainRegionEditorGetSelect(void);
extern void TerrainRegionEditorBeginFrame(void);
extern void TerrainRegionEditorEndFrame(void);
extern void TerrainRegionEditorOverlayDraw(void);
extern S32 TerrainRegionEditorHasSelected(void);
extern void TerrainRegionEditorClearSelection(void);
extern S32 TerrainRegionEditorGetFieldValue(S32 field);
extern S32 TerrainRegionEditorSetFieldValue(S32 field, S32 value);
extern S32 TerrainRegionEditorActivateField(S32 field);

#define TERRAIN_REGION_FIELD_BAKE 0
#define TERRAIN_REGION_FIELD_TYPE 1
#define TERRAIN_REGION_FIELD_SEED 2
#define TERRAIN_REGION_FIELD_PATCHINESS 3
#define TERRAIN_REGION_FIELD_RANDOMIZE 4
#define TERRAIN_REGION_FIELD_X0 5
#define TERRAIN_REGION_FIELD_Z0 6
#define TERRAIN_REGION_FIELD_X1 7
#define TERRAIN_REGION_FIELD_Z1 8
#define TERRAIN_REGION_FIELD_COUNT 9
#endif

#endif /* TERRAIN_REGION_EDITOR_H */
