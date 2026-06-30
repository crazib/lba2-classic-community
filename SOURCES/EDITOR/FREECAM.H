#ifndef FREECAM_H
#define FREECAM_H

#include <SYSTEM/ADELINE_TYPES.H>

#define FREECAM_EVENT_NONE 0
#define FREECAM_EVENT_ENTERED 1
#define FREECAM_EVENT_EXITED 2
#define FREECAM_EVENT_FOG_DISABLED 3
#define FREECAM_EVENT_FOG_ENABLED 4
#define FREECAM_EVENT_ACTORS_HIDDEN 5
#define FREECAM_EVENT_ACTORS_SHOWN 6
#define FREECAM_EVENT_SCREENSHOT 7

extern S32 FreeCam_UpdateEditor(S32 allow_toggle);
extern void FreeCam_ApplyCamera(void);
extern void FreeCam_ApplyFogOverride(void);
extern void FreeCam_CaptureScreenshot(void);
extern S32 FreeCam_IsActive(void);
extern S32 FreeCam_HideActors(void);

#endif
