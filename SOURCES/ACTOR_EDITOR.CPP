#include "ACTOR_EDITOR.H"

#ifdef LBA2CC_EDITOR

#include "C_EXTERN.H"
#include "DIRECTORIES.H"
#include "EDITOR.H"
#include "EDITOR_JSON.H"
#include "OBJECT.H"

#include <SYSTEM/FILES.H>
#include <SYSTEM/LOG.H>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern S32 Island;

typedef struct {
    S32 Model;
    S32 Body;
    S32 Animation;
    S32 X;
    S32 Y;
    S32 Z;
    S32 Sprite;
    S32 Beta;
    S32 Life;
    S32 Armour;
    S32 Move;
    U32 Flags;
} T_ACTOR_EDITOR_STATE;

typedef struct {
    S32 Used;
    S32 Scene;
    S32 Index;
    S32 Added;
    S32 Deleted;
    U32 Changed;
    T_ACTOR_EDITOR_STATE Base;
    T_ACTOR_EDITOR_STATE Target;
} T_ACTOR_EDITOR_EDIT;

#define ACTOR_EDITOR_MAX_EDITS 512
#define ACTOR_EDITOR_CHANGED_MODEL (1U << 0)
#define ACTOR_EDITOR_CHANGED_BODY (1U << 1)
#define ACTOR_EDITOR_CHANGED_ANIMATION (1U << 2)
#define ACTOR_EDITOR_CHANGED_X (1U << 3)
#define ACTOR_EDITOR_CHANGED_Y (1U << 4)
#define ACTOR_EDITOR_CHANGED_Z (1U << 5)
#define ACTOR_EDITOR_CHANGED_SPRITE (1U << 6)
#define ACTOR_EDITOR_CHANGED_BETA (1U << 7)
#define ACTOR_EDITOR_CHANGED_LIFE (1U << 8)
#define ACTOR_EDITOR_CHANGED_ARMOUR (1U << 9)
#define ACTOR_EDITOR_CHANGED_MOVE (1U << 10)
#define ACTOR_EDITOR_CHANGED_FLAGS (1U << 11)
#define ACTOR_EDITOR_CHANGED_ALL ((1U << 12) - 1)

static T_ACTOR_EDITOR_EDIT ActorEditorEdits[ACTOR_EDITOR_MAX_EDITS];
static T_ACTOR_EDITOR_STATE ActorEditorInitial[MAX_OBJETS];
static S32 ActorEditorInitialCount = 0;
static S32 ActorEditorScene = -1;
static S32 ActorEditorIsland = -1;
static S32 ActorEditorLoadedAuthoring = FALSE;
static S32 ActorEditorSelectedIndex = -1;
static S32 ActorEditorPlaceMode = FALSE;
static S32 ActorEditorPlaceTemplate = -1;
static U8 ActorEditorEmptyTrack[] = {TM_END};
static U8 ActorEditorEmptyLife[] = {LM_END};

static void ActorEditorApplyDeleted(T_OBJET *actor) {
    actor->LifePoint = 0;
    actor->Move = NO_MOVE;
    actor->Flags |= INVISIBLE;
    actor->WorkFlags |= OBJ_DEAD;
    actor->Obj.Body.Num = -1;
    actor->Obj.Anim.Num = -1;
    actor->PtrTrack = ActorEditorEmptyTrack;
    actor->PtrLife = ActorEditorEmptyLife;
    actor->OffsetTrack = -1;
    actor->OffsetLife = -1;
}

static void ActorEditorStateFromActor(T_ACTOR_EDITOR_STATE *state, T_OBJET *actor) {
    state->Model = actor->IndexFile3D;
    state->Body = actor->GenBody;
    state->Animation = actor->GenAnim;
    state->X = actor->Obj.X;
    state->Y = actor->Obj.Y;
    state->Z = actor->Obj.Z;
    state->Sprite = actor->Sprite;
    state->Beta = actor->Obj.Beta;
    state->Life = actor->LifePoint;
    state->Armour = actor->Armure;
    state->Move = actor->Move;
    state->Flags = actor->Flags;
}

static void ActorEditorApplyState(T_OBJET *actor, S32 index, T_ACTOR_EDITOR_STATE *state, S32 initialize) {
    actor->IndexFile3D = state->Model;
    actor->GenBody = (U8)state->Body;
    actor->GenAnim = (U16)state->Animation;
    actor->OldPosX = actor->Obj.X = state->X;
    actor->OldPosY = actor->Obj.Y = state->Y;
    actor->OldPosZ = actor->Obj.Z = state->Z;
    actor->Sprite = (S16)state->Sprite;
    actor->OldBeta = actor->Obj.Beta = state->Beta & 4095;
    actor->LifePoint = (S16)state->Life;
    actor->Armure = (U8)state->Armour;
    actor->Move = (U8)state->Move;
    actor->Flags = state->Flags;

    if (actor->Flags & SPRITE_3D) {
        InitSprite(actor->Sprite, actor);
    } else {
        actor->PtrFile3D = LoadFile3D(actor->IndexFile3D);
        if (initialize) {
            actor->Obj.Body.Num = -1;
            InitBody(actor->GenBody, (U8)index);
            actor->Obj.Anim.Num = -1;
            InitAnim(actor->GenAnim, ANIM_REPEAT, (U8)index);
        }
    }
}

static T_ACTOR_EDITOR_EDIT *ActorEditorFindEdit(S32 scene, S32 index) {
    S32 i;

    for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
        T_ACTOR_EDITOR_EDIT *edit = &ActorEditorEdits[i];
        if (edit->Used AND edit->Scene == scene AND edit->Index == index)
            return edit;
    }
    return NULL;
}

static T_ACTOR_EDITOR_EDIT *ActorEditorCreateEdit(S32 index) {
    S32 i;
    T_ACTOR_EDITOR_EDIT *edit;

    edit = ActorEditorFindEdit(ActorEditorScene, index);
    if (edit)
        return edit;
    if (index < 0 OR index >= ActorEditorInitialCount)
        return NULL;

    for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
        edit = &ActorEditorEdits[i];
        if (!edit->Used) {
            memset(edit, 0, sizeof(*edit));
            edit->Used = TRUE;
            edit->Scene = ActorEditorScene;
            edit->Index = index;
            edit->Base = ActorEditorInitial[index];
            edit->Target = edit->Base;
            return edit;
        }
    }

    Editor_SetStatus("Actor edit refused: edit table is full");
    Log_Warn("[editor] actor edit table is full");
    return NULL;
}

static T_ACTOR_EDITOR_STATE *ActorEditorGetAuthoredState(S32 index) {
    T_ACTOR_EDITOR_EDIT *edit = ActorEditorFindEdit(ActorEditorScene, index);

    if (edit)
        return &edit->Target;
    if (index >= 0 AND index < ActorEditorInitialCount)
        return &ActorEditorInitial[index];
    return NULL;
}

static U32 ActorEditorFieldMask(S32 field) {
    if (field >= ACTOR_EDITOR_FIELD_FLAG_FIRST
            AND field <= ACTOR_EDITOR_FIELD_FLAG_LAST)
        return ACTOR_EDITOR_CHANGED_FLAGS;
    switch (field) {
    case ACTOR_EDITOR_FIELD_MODEL: return ACTOR_EDITOR_CHANGED_MODEL;
    case ACTOR_EDITOR_FIELD_BODY: return ACTOR_EDITOR_CHANGED_BODY;
    case ACTOR_EDITOR_FIELD_ANIMATION: return ACTOR_EDITOR_CHANGED_ANIMATION;
    case ACTOR_EDITOR_FIELD_X: return ACTOR_EDITOR_CHANGED_X;
    case ACTOR_EDITOR_FIELD_Y: return ACTOR_EDITOR_CHANGED_Y;
    case ACTOR_EDITOR_FIELD_Z: return ACTOR_EDITOR_CHANGED_Z;
    case ACTOR_EDITOR_FIELD_SPRITE: return ACTOR_EDITOR_CHANGED_SPRITE;
    case ACTOR_EDITOR_FIELD_BETA: return ACTOR_EDITOR_CHANGED_BETA;
    case ACTOR_EDITOR_FIELD_LIFE: return ACTOR_EDITOR_CHANGED_LIFE;
    case ACTOR_EDITOR_FIELD_ARMOUR: return ACTOR_EDITOR_CHANGED_ARMOUR;
    case ACTOR_EDITOR_FIELD_MOVE: return ACTOR_EDITOR_CHANGED_MOVE;
    case ACTOR_EDITOR_FIELD_FLAGS: return ACTOR_EDITOR_CHANGED_FLAGS;
    }
    return 0;
}

static S32 ActorEditorFieldValueValid(S32 field, S32 value) {
    if (field == ACTOR_EDITOR_FIELD_MODEL
            OR field == ACTOR_EDITOR_FIELD_X
            OR field == ACTOR_EDITOR_FIELD_Y
            OR field == ACTOR_EDITOR_FIELD_Z)
        return value >= -32768 AND value <= 32767;
    if (field == ACTOR_EDITOR_FIELD_SPRITE)
        return value >= 0 AND value < PtrZvExtraCount;
    if (field == ACTOR_EDITOR_FIELD_ANIMATION)
        return value >= 0 AND value <= 65535;
    if (field == ACTOR_EDITOR_FIELD_BODY
            OR field == ACTOR_EDITOR_FIELD_ARMOUR
            OR field == ACTOR_EDITOR_FIELD_MOVE)
        return value >= 0 AND value <= 255;
    if (field == ACTOR_EDITOR_FIELD_LIFE)
        return value >= 0 AND value <= 255;
    return TRUE;
}

static void ActorEditorSetStateField(T_ACTOR_EDITOR_STATE *state, S32 field, S32 value) {
    if (field >= ACTOR_EDITOR_FIELD_FLAG_FIRST
            AND field <= ACTOR_EDITOR_FIELD_FLAG_LAST) {
        U32 bit = 1U << (field - ACTOR_EDITOR_FIELD_FLAG_FIRST);
        if (value)
            state->Flags |= bit;
        else
            state->Flags &= ~bit;
        return;
    }
    switch (field) {
    case ACTOR_EDITOR_FIELD_MODEL: state->Model = value; break;
    case ACTOR_EDITOR_FIELD_BODY: state->Body = value; break;
    case ACTOR_EDITOR_FIELD_ANIMATION: state->Animation = value; break;
    case ACTOR_EDITOR_FIELD_X: state->X = value; break;
    case ACTOR_EDITOR_FIELD_Y: state->Y = value; break;
    case ACTOR_EDITOR_FIELD_Z: state->Z = value; break;
    case ACTOR_EDITOR_FIELD_SPRITE: state->Sprite = value; break;
    case ACTOR_EDITOR_FIELD_BETA: state->Beta = value & 4095; break;
    case ACTOR_EDITOR_FIELD_LIFE: state->Life = value; break;
    case ACTOR_EDITOR_FIELD_ARMOUR: state->Armour = value; break;
    case ACTOR_EDITOR_FIELD_MOVE: state->Move = value; break;
    case ACTOR_EDITOR_FIELD_FLAGS: state->Flags = (U32)value; break;
    }
}

static S32 ActorEditorStateField(T_ACTOR_EDITOR_STATE *state, S32 field) {
    if (field >= ACTOR_EDITOR_FIELD_FLAG_FIRST
            AND field <= ACTOR_EDITOR_FIELD_FLAG_LAST)
        return (state->Flags & (1U << (field - ACTOR_EDITOR_FIELD_FLAG_FIRST))) ? TRUE : FALSE;
    switch (field) {
    case ACTOR_EDITOR_FIELD_MODEL: return state->Model;
    case ACTOR_EDITOR_FIELD_BODY: return state->Body;
    case ACTOR_EDITOR_FIELD_ANIMATION: return state->Animation;
    case ACTOR_EDITOR_FIELD_X: return state->X;
    case ACTOR_EDITOR_FIELD_Y: return state->Y;
    case ACTOR_EDITOR_FIELD_Z: return state->Z;
    case ACTOR_EDITOR_FIELD_SPRITE: return state->Sprite;
    case ACTOR_EDITOR_FIELD_BETA: return state->Beta;
    case ACTOR_EDITOR_FIELD_LIFE: return state->Life;
    case ACTOR_EDITOR_FIELD_ARMOUR: return state->Armour;
    case ACTOR_EDITOR_FIELD_MOVE: return state->Move;
    case ACTOR_EDITOR_FIELD_FLAGS: return (S32)state->Flags;
    }
    return 0;
}

static void ActorEditorMergeChangedState(T_ACTOR_EDITOR_STATE *target,
                                         T_ACTOR_EDITOR_STATE *changes,
                                         U32 changed) {
    S32 field;

    for (field = ACTOR_EDITOR_FIELD_MODEL; field < ACTOR_EDITOR_FIELD_COUNT; field++) {
        if (changed & ActorEditorFieldMask(field))
            ActorEditorSetStateField(target, field, ActorEditorStateField(changes, field));
    }
}

static S32 ActorEditorParseState(const char *json,
                                 jsmntok_t *tokens,
                                 S32 count,
                                 S32 object,
                                 T_ACTOR_EDITOR_STATE *state,
                                 U32 *changed,
                                 const char *prefix) {
    static const char *names[] = {
        "model", "body", "animation", "x", "y", "z", "sprite", "beta", "life", "armour", "move", "flags"
    };
    S32 fields[] = {
        ACTOR_EDITOR_FIELD_MODEL,
        ACTOR_EDITOR_FIELD_BODY,
        ACTOR_EDITOR_FIELD_ANIMATION,
        ACTOR_EDITOR_FIELD_X,
        ACTOR_EDITOR_FIELD_Y,
        ACTOR_EDITOR_FIELD_Z,
        ACTOR_EDITOR_FIELD_SPRITE,
        ACTOR_EDITOR_FIELD_BETA,
        ACTOR_EDITOR_FIELD_LIFE,
        ACTOR_EDITOR_FIELD_ARMOUR,
        ACTOR_EDITOR_FIELD_MOVE,
        ACTOR_EDITOR_FIELD_FLAGS
    };
    S32 i;
    S32 parsed = 0;

    for (i = 0; i < 12; i++) {
        char key[40];
        S32 value;
        snprintf(key, sizeof(key), "%s%s", prefix, names[i]);
        if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, object, key), &value))
            continue;
        ActorEditorSetStateField(state, fields[i], value);
        if (changed)
            *changed |= ActorEditorFieldMask(fields[i]);
        parsed++;
    }
    return parsed;
}

static void ActorEditorLoadAuthoringEdits(void) {
    char dir[ADELINE_MAX_PATH];
    char path[ADELINE_MAX_PATH];
    char *json;
    jsmntok_t *tokens;
    S32 count;
    S32 file_island;
    S32 scenes;
    S32 scene_token;
    S32 loaded = 0;
    S32 skipped = 0;

    if (ActorEditorLoadedAuthoring AND ActorEditorIsland == Island)
        return;
    memset(ActorEditorEdits, 0, sizeof(ActorEditorEdits));
    ActorEditorIsland = Island;
    ActorEditorLoadedAuthoring = TRUE;

    if (!Editor_ResolveOutputDir(dir, sizeof(dir)))
        return;
    snprintf(path, sizeof(path), "%s%sactor_edits.json", dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';
    if (!EditorJsonParseFile(path, &json, &tokens, &count)) {
        if (EditorJsonFileExists(path))
            Log_Warn("[editor] cannot load actor edits: invalid JSON: %s", path);
        return;
    }
    if (EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, 0, "island"), &file_island)
            AND file_island != Island) {
        Log_Warn("[editor] actor edits ignored: %s is for island %d, current island is %d",
                 path,
                 (int)file_island,
                 (int)Island);
        free(tokens);
        free(json);
        return;
    }

    scenes = EditorJsonFindValue(json, tokens, count, 0, "scenes");
    if (scenes >= 0 AND tokens[scenes].type == JSMN_ARRAY) {
        S32 scenes_end = EditorJsonTokenEnd(tokens, count, scenes);
        for (scene_token = scenes + 1; scene_token < scenes_end; scene_token = EditorJsonTokenEnd(tokens, count, scene_token)) {
            S32 scene;
            S32 actors;
            S32 actor_token;

            if (tokens[scene_token].parent != scenes OR tokens[scene_token].type != JSMN_OBJECT)
                continue;
            if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, scene_token, "scene"), &scene)) {
                skipped++;
                continue;
            }
            actors = EditorJsonFindValue(json, tokens, count, scene_token, "actors");
            if (actors < 0 OR tokens[actors].type != JSMN_ARRAY)
                continue;
            {
                S32 actors_end = EditorJsonTokenEnd(tokens, count, actors);
                for (actor_token = actors + 1; actor_token < actors_end; actor_token = EditorJsonTokenEnd(tokens, count, actor_token)) {
                    T_ACTOR_EDITOR_EDIT *edit = NULL;
                    S32 index;
                    S32 added = FALSE;
                    S32 deleted = FALSE;
                    S32 i;

                    if (tokens[actor_token].parent != actors OR tokens[actor_token].type != JSMN_OBJECT)
                        continue;
                    if (!EditorJsonParseInt(json, tokens, count, EditorJsonFindValue(json, tokens, count, actor_token, "actor"), &index)) {
                        skipped++;
                        continue;
                    }
                    EditorJsonParseBool(json, tokens, count, EditorJsonFindValue(json, tokens, count, actor_token, "added"), &added);
                    EditorJsonParseBool(json, tokens, count, EditorJsonFindValue(json, tokens, count, actor_token, "deleted"), &deleted);
                    edit = ActorEditorFindEdit(scene, index);
                    if (!edit) {
                        for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
                            if (!ActorEditorEdits[i].Used) {
                                edit = &ActorEditorEdits[i];
                                break;
                            }
                        }
                    }
                    if (!edit) {
                        skipped++;
                        continue;
                    }
                    memset(edit, 0, sizeof(*edit));
                    edit->Used = TRUE;
                    edit->Scene = scene;
                    edit->Index = index;
                    edit->Added = added;
                    edit->Deleted = deleted;
                    ActorEditorParseState(json, tokens, count, actor_token, &edit->Target, &edit->Changed, "");
                    ActorEditorParseState(json, tokens, count, actor_token, &edit->Base, NULL, "expect_");
                    if (added)
                        edit->Changed = ACTOR_EDITOR_CHANGED_ALL;
                    loaded++;
                }
            }
        }
    }

    Log_Info("[editor] loaded actor edits: %s (loaded=%d skipped=%d)", path, (int)loaded, (int)skipped);
    free(tokens);
    free(json);
}

static void ActorEditorApplyLoadedEdits(void) {
    S32 i;

    for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
        T_ACTOR_EDITOR_EDIT *edit = &ActorEditorEdits[i];

        if (!edit->Used OR edit->Scene != ActorEditorScene OR edit->Added)
            continue;
        if (edit->Index <= 0 OR edit->Index >= NbObjets) {
            Log_Warn("[editor] actor edit skipped scene %d actor %d: index is out of range",
                     (int)edit->Scene,
                     (int)edit->Index);
            continue;
        }
        {
            T_ACTOR_EDITOR_STATE expected = edit->Base;
            T_ACTOR_EDITOR_STATE changes = edit->Target;
            edit->Base = ActorEditorInitial[edit->Index];
            ActorEditorMergeChangedState(&edit->Base, &expected, edit->Changed);
            edit->Target = edit->Base;
            ActorEditorMergeChangedState(&edit->Target, &changes, edit->Changed);
        }
        ActorEditorApplyState(&ListObjet[edit->Index], edit->Index, &edit->Target, FALSE);
        if (edit->Deleted)
            ActorEditorApplyDeleted(&ListObjet[edit->Index]);
        ActorEditorInitial[edit->Index] = edit->Target;
    }

    for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
        T_ACTOR_EDITOR_EDIT *edit = &ActorEditorEdits[i];
        T_OBJET *actor;

        if (!edit->Used OR edit->Scene != ActorEditorScene OR !edit->Added)
            continue;
        if (edit->Index != NbObjets OR NbObjets >= MAX_OBJETS) {
            Log_Warn("[editor] added actor skipped scene %d actor %d: expected next index %d",
                     (int)edit->Scene,
                     (int)edit->Index,
                     (int)NbObjets);
            continue;
        }
        InitObject((U8)NbObjets);
        actor = &ListObjet[NbObjets];
        actor->PtrTrack = ActorEditorEmptyTrack;
        actor->PtrLife = ActorEditorEmptyLife;
        actor->OffsetTrack = -1;
        actor->OffsetLife = 0;
        ActorEditorApplyState(actor, NbObjets, &edit->Target, FALSE);
        if (edit->Deleted)
            ActorEditorApplyDeleted(actor);
        ActorEditorInitial[NbObjets] = edit->Target;
        NbObjets++;
    }
    ActorEditorInitialCount = NbObjets;
}

void ActorEditorOnSceneLoaded(S32 scene) {
    S32 i;

    ActorEditorScene = scene;
    ActorEditorSelectedIndex = -1;
    ActorEditorPlaceMode = FALSE;
    ActorEditorInitialCount = NbObjets;
    for (i = 0; i < NbObjets; i++)
        ActorEditorStateFromActor(&ActorEditorInitial[i], &ListObjet[i]);
    ActorEditorLoadAuthoringEdits();
    ActorEditorApplyLoadedEdits();
}

void ActorEditorRestoreAddedActorsAfterSaveLoad(void) {
    S32 appended;
    S32 i;

    for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
        T_ACTOR_EDITOR_EDIT *edit = &ActorEditorEdits[i];

        if (!edit->Used
                OR edit->Scene != ActorEditorScene
                OR edit->Added
                OR !edit->Deleted
                OR edit->Index <= 0
                OR edit->Index >= NbObjets)
            continue;
        ActorEditorApplyDeleted(&ListObjet[edit->Index]);
    }

    do {
        appended = FALSE;
        for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
            T_ACTOR_EDITOR_EDIT *edit = &ActorEditorEdits[i];
            T_OBJET *actor;

            if (!edit->Used
                    OR edit->Scene != ActorEditorScene
                    OR !edit->Added
                    OR edit->Index != NbObjets)
                continue;
            if (NbObjets >= MAX_OBJETS) {
                Log_Warn("[editor] cannot restore added actor scene %d actor %d after save load: actor limit reached",
                         (int)edit->Scene,
                         (int)edit->Index);
                return;
            }
            InitObject((U8)NbObjets);
            actor = &ListObjet[NbObjets];
            actor->PtrTrack = ActorEditorEmptyTrack;
            actor->PtrLife = ActorEditorEmptyLife;
            actor->OffsetTrack = -1;
            actor->OffsetLife = 0;
            ActorEditorApplyState(actor, NbObjets, &edit->Target, TRUE);
            if (edit->Deleted)
                ActorEditorApplyDeleted(actor);
            ActorEditorInitial[NbObjets] = edit->Target;
            NbObjets++;
            appended = TRUE;
            Log_Info("[editor] restored added actor scene %d actor %d after save load",
                     (int)edit->Scene,
                     (int)edit->Index);
        }
    } while (appended);
    ActorEditorInitialCount = NbObjets;
}

void ActorEditorClearSelection(void) {
    ActorEditorSelectedIndex = -1;
}

S32 ActorEditorGetSelectedIndex(void) {
    if (ActorEditorSelectedIndex < 0 OR ActorEditorSelectedIndex >= NbObjets)
        return -1;
    return ActorEditorSelectedIndex;
}

void ActorEditorResolvePick(S32 index, S32 toggle) {
    if (index < 0 OR index >= NbObjets OR ActorEditorIsDeleted(index))
        return;
    if (toggle AND ActorEditorSelectedIndex == index)
        ActorEditorSelectedIndex = -1;
    else
        ActorEditorSelectedIndex = index;
}

S32 ActorEditorIsDeleted(S32 index) {
    T_ACTOR_EDITOR_EDIT *edit = ActorEditorFindEdit(ActorEditorScene, index);
    return edit AND edit->Deleted;
}

S32 ActorEditorCanEditField(S32 field) {
    T_ACTOR_EDITOR_EDIT *edit;
    T_ACTOR_EDITOR_STATE *state;

    if (field == ACTOR_EDITOR_FIELD_INDEX)
        return FALSE;
    state = ActorEditorGetAuthoredState(ActorEditorSelectedIndex);
    if (!state)
        return FALSE;
    if (field == ACTOR_EDITOR_FIELD_SPRITE)
        return (state->Flags & SPRITE_3D) ? TRUE : FALSE;
    edit = ActorEditorFindEdit(ActorEditorScene, ActorEditorSelectedIndex);
    if (field == ACTOR_EDITOR_FIELD_FLAG_FIRST + 18)
        return FALSE;
    if (field == ACTOR_EDITOR_FIELD_FLAG_FIRST + 10 AND(!edit OR !edit->Added))
        return FALSE;
    return TRUE;
}

void ActorEditorDeleteSelected(void) {
    T_ACTOR_EDITOR_EDIT *edit;
    S32 index = ActorEditorSelectedIndex;

    if (index <= 0 OR index >= NbObjets)
        return;
    edit = ActorEditorCreateEdit(index);
    if (!edit)
        return;
    edit->Deleted = TRUE;
    ActorEditorApplyDeleted(&ListObjet[index]);
    ActorEditorSelectedIndex = -1;
    Editor_SetStatus("Deleted actor (index preserved)");
    Log_Info("[editor] deleted actor scene %d actor %d with index-preserving tombstone",
             (int)ActorEditorScene,
             (int)index);
    FirstTime = AFF_ALL_FLIP;
}

S32 ActorEditorGetFieldValue(S32 field) {
    T_ACTOR_EDITOR_STATE *state;

    if (field == ACTOR_EDITOR_FIELD_INDEX)
        return ActorEditorSelectedIndex;
    state = ActorEditorGetAuthoredState(ActorEditorSelectedIndex);
    return state ? ActorEditorStateField(state, field) : 0;
}

S32 ActorEditorSetFieldValue(S32 field, S32 value) {
    T_ACTOR_EDITOR_EDIT *edit;
    U32 mask;

    if (ActorEditorSelectedIndex <= 0 OR ActorEditorSelectedIndex >= NbObjets)
        return FALSE;
    mask = ActorEditorFieldMask(field);
    if (!mask)
        return FALSE;
    if (!ActorEditorCanEditField(field)) {
        Editor_SetStatus("Actor field is read-only for this actor");
        return FALSE;
    }
    if (!ActorEditorFieldValueValid(field, value)) {
        if (field == ACTOR_EDITOR_FIELD_SPRITE)
            Editor_SetStatus("Sprite ID refused: missing SPRITES metadata");
        else
            Editor_SetStatus("Actor value refused: outside scene-file range");
        return FALSE;
    }
    edit = ActorEditorCreateEdit(ActorEditorSelectedIndex);
    if (!edit)
        return FALSE;
    if (field == ACTOR_EDITOR_FIELD_FLAGS
            AND(((U32)value ^ edit->Target.Flags) & ANIM_3DS)) {
        Editor_SetStatus("Actor flags refused: sprite/3DS layout cannot change");
        return FALSE;
    }
    if (field == ACTOR_EDITOR_FIELD_FLAGS
            AND(((U32)value ^ edit->Target.Flags) & SPRITE_3D)
            AND !edit->Added) {
        Editor_SetStatus("Actor flags refused: sprite layout cannot change for existing actors");
        return FALSE;
    }
    if (field >= ACTOR_EDITOR_FIELD_FLAG_FIRST
            AND field <= ACTOR_EDITOR_FIELD_FLAG_LAST) {
        U32 bit = 1U << (field - ACTOR_EDITOR_FIELD_FLAG_FIRST);
        if (bit & ANIM_3DS) {
            Editor_SetStatus("Actor flag is read-only: 3DS layout would change");
            return FALSE;
        }
        if ((bit & SPRITE_3D) AND !edit->Added) {
            Editor_SetStatus("Actor flag is read-only: sprite layout would change");
            return FALSE;
        }
    }
    ActorEditorSetStateField(&edit->Target, field, value);
    if (field == ACTOR_EDITOR_FIELD_FLAG_FIRST + 10
            AND value
            AND edit->Target.Sprite < 0
            AND PtrZvExtraCount > 425)
        edit->Target.Sprite = 425;
    edit->Changed |= mask;
    ActorEditorApplyState(&ListObjet[ActorEditorSelectedIndex], ActorEditorSelectedIndex, &edit->Target, TRUE);
    FirstTime = AFF_ALL_FLIP;
    return TRUE;
}

void ActorEditorNudgeSelected(S32 dx, S32 dy, S32 dz) {
    T_ACTOR_EDITOR_EDIT *edit;
    S32 x;
    S32 y;
    S32 z;

    if (ActorEditorSelectedIndex <= 0 OR ActorEditorSelectedIndex >= NbObjets)
        return;
    edit = ActorEditorCreateEdit(ActorEditorSelectedIndex);
    if (!edit)
        return;
    x = edit->Target.X + dx;
    y = edit->Target.Y + dy;
    z = edit->Target.Z + dz;
    if (x < -32768 OR x > 32767
            OR y < -32768 OR y > 32767
            OR z < -32768 OR z > 32767) {
        Editor_SetStatus("Actor move refused: outside scene-file range");
        return;
    }
    if (dx) {
        edit->Target.X = x;
        edit->Changed |= ACTOR_EDITOR_CHANGED_X;
    }
    if (dy) {
        edit->Target.Y = y;
        edit->Changed |= ACTOR_EDITOR_CHANGED_Y;
    }
    if (dz) {
        edit->Target.Z = z;
        edit->Changed |= ACTOR_EDITOR_CHANGED_Z;
    }
    ActorEditorApplyState(&ListObjet[ActorEditorSelectedIndex], ActorEditorSelectedIndex, &edit->Target, FALSE);
    FirstTime = AFF_ALL_FLIP;
}

void ActorEditorBeginPlace(void) {
    S32 i;

    if (NbObjets >= MAX_OBJETS) {
        Editor_SetStatus("New actor refused: actor limit reached");
        return;
    }
    ActorEditorPlaceTemplate = ActorEditorSelectedIndex;
    if (ActorEditorPlaceTemplate <= 0
            OR ActorEditorPlaceTemplate >= NbObjets
            OR(ListObjet[ActorEditorPlaceTemplate].Flags & SPRITE_3D)) {
        ActorEditorPlaceTemplate = -1;
        for (i = 1; i < NbObjets; i++) {
            if (!(ListObjet[i].Flags & SPRITE_3D)) {
                ActorEditorPlaceTemplate = i;
                break;
            }
        }
    }
    if (ActorEditorPlaceTemplate < 0) {
        Editor_SetStatus("New actor refused: no actor model to clone");
        return;
    }
    ActorEditorPlaceMode = TRUE;
    ActorEditorSelectedIndex = -1;
    Editor_SetStatus("New Actor: click a world position");
}

S32 ActorEditorIsPlacing(void) {
    return ActorEditorPlaceMode;
}

void ActorEditorCancelPlace(void) {
    ActorEditorPlaceMode = FALSE;
}

void ActorEditorPlaceAt(S32 x, S32 y, S32 z) {
    T_ACTOR_EDITOR_EDIT *edit;
    T_ACTOR_EDITOR_STATE *template_state;
    T_OBJET *actor;
    S32 i;

    if (!ActorEditorPlaceMode OR NbObjets >= MAX_OBJETS)
        return;
    if (x < -32768 OR x > 32767
            OR y < -32768 OR y > 32767
            OR z < -32768 OR z > 32767) {
        Editor_SetStatus("New actor refused: position is outside scene-file range");
        return;
    }
    template_state = ActorEditorGetAuthoredState(ActorEditorPlaceTemplate);
    if (!template_state)
        return;
    edit = NULL;
    for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
        if (!ActorEditorEdits[i].Used) {
            edit = &ActorEditorEdits[i];
            break;
        }
    }
    if (!edit) {
        Editor_SetStatus("New actor refused: edit table is full");
        return;
    }

    memset(edit, 0, sizeof(*edit));
    edit->Used = TRUE;
    edit->Scene = ActorEditorScene;
    edit->Index = NbObjets;
    edit->Added = TRUE;
    edit->Changed = ACTOR_EDITOR_CHANGED_ALL;
    edit->Target = *template_state;
    edit->Target.X = x;
    edit->Target.Y = y;
    edit->Target.Z = z;
    edit->Target.Move = NO_MOVE;
    edit->Target.Life = MAX_LIFE_POINTS;
    edit->Target.Flags = 0;

    InitObject((U8)NbObjets);
    actor = &ListObjet[NbObjets];
    actor->PtrTrack = ActorEditorEmptyTrack;
    actor->PtrLife = ActorEditorEmptyLife;
    actor->OffsetTrack = -1;
    actor->OffsetLife = 0;
    ActorEditorApplyState(actor, NbObjets, &edit->Target, TRUE);
    ActorEditorInitial[NbObjets] = edit->Target;
    ActorEditorInitialCount = NbObjets + 1;
    ActorEditorSelectedIndex = NbObjets;
    NbObjets++;
    ActorEditorPlaceMode = FALSE;
    Editor_SetStatus("New actor added and selected");
    FirstTime = AFF_ALL_FLIP;
}

static void ActorEditorWriteField(FILE *f,
                                  const char *name,
                                  S32 value,
                                  S32 *first) {
    if (!*first)
        fprintf(f, ",\n");
    *first = FALSE;
    fprintf(f, "          \"%s\": %d", name, (int)value);
}

static void ActorEditorWriteBoolField(FILE *f,
                                     const char *name,
                                     S32 value,
                                     S32 *first) {
    if (!*first)
        fprintf(f, ",\n");
    *first = FALSE;
    fprintf(f, "          \"%s\": %s", name, value ? "true" : "false");
}

static void ActorEditorWriteChangedFields(FILE *f, T_ACTOR_EDITOR_EDIT *edit, S32 *first) {
    static const char *names[] = {
        "model", "body", "animation", "x", "y", "z", "sprite", "beta", "life", "armour", "move", "flags"
    };
    S32 fields[] = {
        ACTOR_EDITOR_FIELD_MODEL,
        ACTOR_EDITOR_FIELD_BODY,
        ACTOR_EDITOR_FIELD_ANIMATION,
        ACTOR_EDITOR_FIELD_X,
        ACTOR_EDITOR_FIELD_Y,
        ACTOR_EDITOR_FIELD_Z,
        ACTOR_EDITOR_FIELD_SPRITE,
        ACTOR_EDITOR_FIELD_BETA,
        ACTOR_EDITOR_FIELD_LIFE,
        ACTOR_EDITOR_FIELD_ARMOUR,
        ACTOR_EDITOR_FIELD_MOVE,
        ACTOR_EDITOR_FIELD_FLAGS
    };
    S32 i;

    for (i = 0; i < 12; i++) {
        U32 mask = ActorEditorFieldMask(fields[i]);
        char expect_name[48];
        if (!(edit->Changed & mask))
            continue;
        if (!edit->Added) {
            snprintf(expect_name, sizeof(expect_name), "expect_%s", names[i]);
            ActorEditorWriteField(f, expect_name, ActorEditorStateField(&edit->Base, fields[i]), first);
        }
        ActorEditorWriteField(f, names[i], ActorEditorStateField(&edit->Target, fields[i]), first);
    }
}

S32 ActorEditorSaveRuntimeEdits(const char *island_dir, S32 island) {
    char path[ADELINE_MAX_PATH];
    FILE *f;
    S32 scene;
    S32 first_scene = TRUE;
    S32 count = 0;

    if (!island_dir OR !island_dir[0])
        return FALSE;
    snprintf(path, sizeof(path), "%s%sactor_edits.json", island_dir, ADELINE_PATH_SEP);
    path[sizeof(path) - 1] = '\0';
    f = fopen(path, "wb");
    if (!f) {
        Editor_SetStatus("Actor save failed: cannot open actor_edits.json");
        Log_Warn("[editor] cannot write actor edits: %s", path);
        return FALSE;
    }

    fprintf(f, "{\n");
    fprintf(f, "  \"island\": %d,\n", (int)island);
    fprintf(f, "  \"description\": \"Initial actor edits exported by the actor editor.\",\n");
    fprintf(f, "  \"scenes\": [\n");
    for (scene = 0; scene < 256; scene++) {
        S32 i;
        S32 first_actor = TRUE;
        S32 has_scene = FALSE;

        for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
            if (ActorEditorEdits[i].Used
                    AND ActorEditorEdits[i].Scene == scene
                    AND(ActorEditorEdits[i].Added
                        OR ActorEditorEdits[i].Deleted
                        OR ActorEditorEdits[i].Changed)) {
                has_scene = TRUE;
                break;
            }
        }
        if (!has_scene)
            continue;
        if (!first_scene)
            fprintf(f, ",\n");
        first_scene = FALSE;
        fprintf(f, "    {\n");
        fprintf(f, "      \"scene\": %d,\n", (int)scene);
        fprintf(f, "      \"actors\": [\n");
        for (i = 0; i < ACTOR_EDITOR_MAX_EDITS; i++) {
            T_ACTOR_EDITOR_EDIT *edit = &ActorEditorEdits[i];
            S32 first_field = TRUE;

            if (!edit->Used
                    OR edit->Scene != scene
                    OR(!edit->Added AND !edit->Deleted AND !edit->Changed))
                continue;
            if (!first_actor)
                fprintf(f, ",\n");
            first_actor = FALSE;
            count++;
            fprintf(f, "        {\n");
            ActorEditorWriteField(f, "actor", edit->Index, &first_field);
            if (edit->Added)
                ActorEditorWriteBoolField(f, "added", TRUE, &first_field);
            if (edit->Deleted)
                ActorEditorWriteBoolField(f, "deleted", TRUE, &first_field);
            ActorEditorWriteChangedFields(f, edit, &first_field);
            fprintf(f, "\n        }");
        }
        fprintf(f, "\n");
        fprintf(f, "      ]\n");
        fprintf(f, "    }");
    }
    fprintf(f, "\n");
    fprintf(f, "  ]\n");
    fprintf(f, "}\n");
    fclose(f);

    Log_Info("[editor] saved %d actor edit(s) to %s", (int)count, path);
    return TRUE;
}

#endif
