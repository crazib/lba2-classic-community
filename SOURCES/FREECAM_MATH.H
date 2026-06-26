#ifndef FREECAM_MATH_H
#define FREECAM_MATH_H

#include <SYSTEM/ADELINE_TYPES.H>

typedef struct {
    float x;
    float y;
    float z;
} T_FREECAM_VECTOR;

extern S32 FreeCam_NormalizeAngle(S32 angle);
extern S32 FreeCam_SignedPitch(S32 angle);
extern T_FREECAM_VECTOR FreeCam_MovementVector(S32 alpha, S32 beta,
                                               S32 forward, S32 right, S32 vertical);

#endif
