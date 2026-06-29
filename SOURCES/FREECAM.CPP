#include "FREECAM.H"

#include "C_EXTERN.H"
#include "DIRECTORIES.H"
#include "FREECAM_MATH.H"
#include "3DEXT/LOADISLE.H"
#include "3DEXT/VAR_EXT.H"

#include <3D/CAMERA.H>
#include <POLYGON/POLY.H>
#include <SYSTEM/INPUT.H>
#include <SYSTEM/KEYBOARD.H>
#include <SYSTEM/KEYBOARD_KEYS.H>
#include <SYSTEM/LOG.H>
#include <SYSTEM/MOUSE.H>
#include <SYSTEM/TIMER.H>
#include <SYSTEM/WINDOW.H>

#include <SDL3/SDL.h>
#include <stdio.h>
#include <string.h>

#define FREECAM_MOVE_PER_MS 8.0f
#define FREECAM_FAST_MULTIPLIER 4.0f
#define FREECAM_MOUSE_SENSITIVITY 3.0f
#define FREECAM_PITCH_LIMIT 1000
#define FREECAM_MAX_DT 100
#define FREECAM_FOG_FAR 400000
#define FREECAM_FOG_RANGE 100

static S32 s_active = FALSE;
static S32 s_last_toggle = FALSE;
static S32 s_last_fog_toggle = FALSE;
static S32 s_last_actor_toggle = FALSE;
static S32 s_last_screenshot = FALSE;
static S32 s_fog_disabled = FALSE;
static S32 s_hide_actors = FALSE;
static S32 s_screenshot_pending = FALSE;
static S32 s_saved_start_fog = 0;
static S32 s_saved_clip_far = 0;
static S32 s_cube = -1;
static U32 s_last_time = 0;
static float s_x = 0.0f;
static float s_y = 0.0f;
static float s_z = 0.0f;
static S32 s_alpha = 0;
static S32 s_beta = 0;
static S32 s_gamma = 0;

static void FreeCam_SetRelativeMode(S32 enabled) {
    if (!Window_SetRelativeMouseMode(enabled != FALSE) AND enabled)
        Log_Warn("Free camera: unable to capture relative mouse: %s", SDL_GetError());
    {
        float unused_x;
        float unused_y;
        Window_GetRelativeMouseDelta(&unused_x, &unused_y);
    }
}

static void FreeCam_Start(void) {
    s_x = (float)CameraX;
    s_y = (float)CameraY;
    s_z = (float)CameraZ;
    s_alpha = FreeCam_SignedPitch(CameraAlpha);
    s_beta = FreeCam_NormalizeAngle(CameraBeta);
    s_gamma = FreeCam_NormalizeAngle(CameraGamma);
    s_saved_start_fog = StartZFog;
    s_saved_clip_far = ClipZFar;
    s_fog_disabled = TRUE;
    s_hide_actors = FALSE;
    s_last_fog_toggle = FALSE;
    s_last_actor_toggle = FALSE;
    s_last_screenshot = FALSE;
    s_active = TRUE;
    s_last_time = TimerRefHR;
    StartZFog = FREECAM_FOG_FAR - FREECAM_FOG_RANGE;
    ClipZFar = FREECAM_FOG_FAR;
    SetFog(StartZFog, ClipZFar);
    FreeCam_SetRelativeMode(TRUE);
}

static void FreeCam_ApplyState(void) {
    T_FREECAM_VECTOR forward;

    forward = FreeCam_MovementVector(s_alpha, s_beta, 1, 0, 0);
    VueOffsetX = (S32)(s_x + forward.x * (float)VueDistance);
    VueOffsetY = (S32)(s_y + forward.y * (float)VueDistance);
    VueOffsetZ = (S32)(s_z + forward.z * (float)VueDistance);
    AlphaCam = FreeCam_NormalizeAngle(s_alpha);
    BetaCam = s_beta;
    GammaCam = s_gamma;

    SetFollowCamera(VueOffsetX, VueOffsetY, VueOffsetZ,
                    AlphaCam, BetaCam, GammaCam, VueDistance);
}

static void FreeCam_Stop(void) {
    FreeCam_ApplyState();
    if (s_fog_disabled) {
        StartZFog = s_saved_start_fog;
        ClipZFar = s_saved_clip_far;
        SetFog(StartZFog, ClipZFar);
        s_fog_disabled = FALSE;
    }
    s_active = FALSE;
    FreeCam_SetRelativeMode(FALSE);
}

static void FreeCam_Reset(void) {
    if (s_fog_disabled) {
        StartZFog = s_saved_start_fog;
        ClipZFar = s_saved_clip_far;
        SetFog(StartZFog, ClipZFar);
        s_fog_disabled = FALSE;
    }
    if (s_active)
        FreeCam_SetRelativeMode(FALSE);
    s_active = FALSE;
    s_last_toggle = FALSE;
    s_last_fog_toggle = FALSE;
}

S32 FreeCam_IsActive(void) {
    return s_active;
}

S32 FreeCam_HideActors(void) {
    return s_active AND s_hide_actors;
}

void FreeCam_ApplyFogOverride(void) {
    if (!s_active OR !s_fog_disabled)
        return;

    StartZFog = FREECAM_FOG_FAR - FREECAM_FOG_RANGE;
    ClipZFar = FREECAM_FOG_FAR;
    SetFog(StartZFog, ClipZFar);
}

S32 FreeCam_UpdateEditor(S32 allow_toggle) {
    U32 dt;
    S32 shift;
    S32 toggle;
    S32 fog_toggle;
    S32 actor_toggle;
    S32 screenshot;
    S32 event = FREECAM_EVENT_NONE;

    SetExteriorFogOverride(FreeCam_ApplyFogOverride);
    SetExteriorWideViewActive(FreeCam_IsActive);

    if (s_cube != NumCube) {
        FreeCam_Reset();
        s_cube = NumCube;
    }

    shift = (CheckKey(K_SHIFT) OR CheckKey(K_SHIFT_LEFT) OR CheckKey(K_SHIFT_RIGHT)) ? TRUE : FALSE;
    toggle = (allow_toggle AND shift AND CheckKey(K_CARRE)) ? TRUE : FALSE;

    if (toggle AND !s_last_toggle) {
        if (s_active)
            FreeCam_Stop();
        else {
            FreeCam_Start();
        }
        event = s_active ? FREECAM_EVENT_ENTERED : FREECAM_EVENT_EXITED;
    }
    s_last_toggle = toggle;

    if (!s_active) {
        if (event != FREECAM_EVENT_NONE) {
            Input = 0;
            MyKey = 0;
            Key = 0;
            Click = 0;
            memset(TabKeys, 0, sizeof(TabKeys));
            FirstTime = AFF_ALL_FLIP;
        }
        return event;
    }

    fog_toggle = CheckKey(K_F) ? TRUE : FALSE;
    if (fog_toggle AND !s_last_fog_toggle) {
        s_fog_disabled = !s_fog_disabled;
        if (s_fog_disabled) {
            FreeCam_ApplyFogOverride();
            event = FREECAM_EVENT_FOG_DISABLED;
        } else {
            StartZFog = s_saved_start_fog;
            ClipZFar = s_saved_clip_far;
            event = FREECAM_EVENT_FOG_ENABLED;
        }
        SetFog(StartZFog, ClipZFar);
    }
    s_last_fog_toggle = fog_toggle;

    actor_toggle = CheckKey(K_H) ? TRUE : FALSE;
    if (actor_toggle AND !s_last_actor_toggle) {
        s_hide_actors = !s_hide_actors;
        event = s_hide_actors ? FREECAM_EVENT_ACTORS_HIDDEN : FREECAM_EVENT_ACTORS_SHOWN;
    }
    s_last_actor_toggle = actor_toggle;

    screenshot = CheckKey(K_P) ? TRUE : FALSE;
    if (screenshot AND !s_last_screenshot) {
        s_screenshot_pending = TRUE;
        event = FREECAM_EVENT_SCREENSHOT;
    }
    s_last_screenshot = screenshot;

    dt = TimerRefHR - s_last_time;
    s_last_time = TimerRefHR;
    if (dt > FREECAM_MAX_DT)
        dt = FREECAM_MAX_DT;

    {
        float mouse_x = 0.0f;
        float mouse_y = 0.0f;
        Window_GetRelativeMouseDelta(&mouse_x, &mouse_y);
        s_beta = FreeCam_NormalizeAngle(s_beta + (S32)(mouse_x * FREECAM_MOUSE_SENSITIVITY));
        s_alpha += (S32)(mouse_y * FREECAM_MOUSE_SENSITIVITY);
        if (s_alpha < -FREECAM_PITCH_LIMIT)
            s_alpha = -FREECAM_PITCH_LIMIT;
        if (s_alpha > FREECAM_PITCH_LIMIT)
            s_alpha = FREECAM_PITCH_LIMIT;
    }

    {
        S32 forward = (CheckKey(K_W) ? 1 : 0) - (CheckKey(K_S) ? 1 : 0);
        S32 right = (CheckKey(K_D) ? 1 : 0) - (CheckKey(K_A) ? 1 : 0);
        S32 vertical = (CheckKey(K_E) ? 1 : 0) - (CheckKey(K_Q) ? 1 : 0);
        float speed = FREECAM_MOVE_PER_MS * (float)dt;
        T_FREECAM_VECTOR movement;

        if (CheckKey(K_SHIFT))
            speed *= FREECAM_FAST_MULTIPLIER;

        movement = FreeCam_MovementVector(s_alpha, s_beta, forward, right, vertical);
        s_x += movement.x * speed;
        s_y += movement.y * speed;
        s_z += movement.z * speed;
    }

    Input = 0;
    MyKey = 0;
    Key = 0;
    Click = 0;
    memset(TabKeys, 0, sizeof(TabKeys));
    FirstTime = AFF_ALL_FLIP;
    return event;
}

void FreeCam_CaptureScreenshot(void) {
    static S32 number = 0;
    char filename[64];
    char path[ADELINE_MAX_PATH];
    FILE *f;

    if (!s_screenshot_pending)
        return;
    s_screenshot_pending = FALSE;

    do {
        snprintf(filename, sizeof(filename), "FREECAM%05d.LIM", (int)number++);
        GetShootPath(path, sizeof(path), filename);
    } while (FileSize(path));

    f = fopen(path, "wb");
    if (!f) {
        Log_Warn("Free camera screenshot failed: %s", path);
        return;
    }
    fwrite(Log, 1, ModeDesiredX * ModeDesiredY, f);
    fclose(f);
    Log_Info("Free camera screenshot saved: %s", path);

    {
        T_FREECAM_VECTOR forward;
        float distance;
        S32 target_x;
        S32 target_z;
        S32 cube_x;
        S32 cube_z;
        S32 offset_x;
        S32 offset_z;

        forward = FreeCam_MovementVector(s_alpha, s_beta, 1, 0, 0);
        if (forward.y > -0.0001f AND forward.y < 0.0001f) {
            Log_Warn("Free camera holomap view unavailable: camera must look toward ground level");
            return;
        }
        distance = -s_y / forward.y;
        if (distance <= 0.0f) {
            Log_Warn("Free camera holomap view unavailable: ground is behind camera");
            return;
        }

        target_x = (S32)(s_x + forward.x * distance);
        target_z = (S32)(s_z + forward.z * distance);
        cube_x = target_x / SCE;
        cube_z = target_z / SCE;
        offset_x = target_x - cube_x * SCE;
        offset_z = target_z - cube_z * SCE;

        Log_Raw("{");
        Log_Raw("  \"type\": \"add_holomap_island_view\",");
        Log_Raw("  \"file\": \"HOLOMAP.HQR\",");
        Log_Raw("  \"id\": 47,");
        Log_Raw("  \"cube_x\": %d,", (int)cube_x);
        Log_Raw("  \"cube_z\": %d,", (int)cube_z);
        Log_Raw("  \"offset_x\": %d,", (int)offset_x);
        Log_Raw("  \"offset_z\": %d,", (int)offset_z);
        Log_Raw("  \"camera_alpha\": %d,", (int)FreeCam_NormalizeAngle(s_alpha));
        Log_Raw("  \"camera_beta\": %d,", (int)s_beta);
        Log_Raw("  \"camera_distance\": %d,", (int)distance);
        Log_Raw("  \"light_alpha\": %d,", (int)AlphaLight);
        Log_Raw("  \"light_beta\": %d", (int)BetaLight);
        Log_Raw("},");
    }
}

void FreeCam_ApplyCamera(void) {
    if (!FreeCam_IsActive())
        return;

    AlphaCam = FreeCam_NormalizeAngle(s_alpha);
    BetaCam = s_beta;
    GammaCam = s_gamma;
    SetPosCamera((S32)s_x, (S32)s_y, (S32)s_z);
    SetAngleCamera(AlphaCam, BetaCam, GammaCam);
}
