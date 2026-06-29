#ifndef ACTOR_EDITOR_H
#define ACTOR_EDITOR_H

#include <SYSTEM/ADELINE_TYPES.H>

#ifdef LBA2CC_EDITOR

#define ACTOR_EDITOR_FIELD_INDEX 0
#define ACTOR_EDITOR_FIELD_MODEL 1
#define ACTOR_EDITOR_FIELD_BODY 2
#define ACTOR_EDITOR_FIELD_ANIMATION 3
#define ACTOR_EDITOR_FIELD_X 4
#define ACTOR_EDITOR_FIELD_Y 5
#define ACTOR_EDITOR_FIELD_Z 6
#define ACTOR_EDITOR_FIELD_SPRITE 7
#define ACTOR_EDITOR_FIELD_BETA 8
#define ACTOR_EDITOR_FIELD_LIFE 9
#define ACTOR_EDITOR_FIELD_ARMOUR 10
#define ACTOR_EDITOR_FIELD_MOVE 11
#define ACTOR_EDITOR_FIELD_FLAGS 12
#define ACTOR_EDITOR_FIELD_FLAG_FIRST 13
#define ACTOR_EDITOR_FIELD_FLAG_LAST 34
#define ACTOR_EDITOR_FIELD_COUNT 35

extern void ActorEditorOnSceneLoaded(S32 scene);
extern void ActorEditorRestoreAddedActorsAfterSaveLoad(void);
extern void ActorEditorClearSelection(void);
extern S32 ActorEditorGetSelectedIndex(void);
extern void ActorEditorResolvePick(S32 index, S32 toggle);
extern S32 ActorEditorIsDeleted(S32 index);
extern S32 ActorEditorCanEditField(S32 field);
extern void ActorEditorDeleteSelected(void);
extern S32 ActorEditorGetFieldValue(S32 field);
extern S32 ActorEditorSetFieldValue(S32 field, S32 value);
extern void ActorEditorNudgeSelected(S32 dx, S32 dy, S32 dz);
extern void ActorEditorBeginPlace(void);
extern S32 ActorEditorIsPlacing(void);
extern void ActorEditorCancelPlace(void);
extern void ActorEditorPlaceAt(S32 x, S32 y, S32 z);
extern S32 ActorEditorSaveRuntimeEdits(const char *island_dir, S32 island);

#else

#define ActorEditorOnSceneLoaded(scene) ((void)0)
#define ActorEditorRestoreAddedActorsAfterSaveLoad() ((void)0)
#define ActorEditorClearSelection() ((void)0)
#define ActorEditorGetSelectedIndex() (-1)
#define ActorEditorResolvePick(index, toggle) ((void)0)
#define ActorEditorIsDeleted(index) (0)
#define ActorEditorCanEditField(field) (0)
#define ActorEditorDeleteSelected() ((void)0)
#define ActorEditorGetFieldValue(field) (0)
#define ActorEditorSetFieldValue(field, value) (0)
#define ActorEditorNudgeSelected(dx, dy, dz) ((void)0)
#define ActorEditorBeginPlace() ((void)0)
#define ActorEditorIsPlacing() (0)
#define ActorEditorCancelPlace() ((void)0)
#define ActorEditorPlaceAt(x, y, z) ((void)0)
#define ActorEditorSaveRuntimeEdits(island_dir, island) (1)

#endif

#endif
