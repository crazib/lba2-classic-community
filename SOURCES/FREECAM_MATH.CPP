#include "FREECAM_MATH.H"

#include <math.h>

#define FREECAM_ANGLE_TO_RADIANS (6.28318530717958647692 / 4096.0)

S32 FreeCam_NormalizeAngle(S32 angle) {
    angle %= 4096;
    if (angle < 0)
        angle += 4096;
    return angle;
}

S32 FreeCam_SignedPitch(S32 angle) {
    angle = FreeCam_NormalizeAngle(angle);
    if (angle > 2048)
        angle -= 4096;
    return angle;
}

T_FREECAM_VECTOR FreeCam_MovementVector(S32 alpha, S32 beta,
                                        S32 forward, S32 right, S32 vertical) {
    T_FREECAM_VECTOR result;
    double pitch = (double)FreeCam_SignedPitch(alpha) * FREECAM_ANGLE_TO_RADIANS;
    double yaw = (double)FreeCam_NormalizeAngle(beta) * FREECAM_ANGLE_TO_RADIANS;
    double cos_pitch = cos(pitch);
    double sin_pitch = sin(pitch);
    double cos_yaw = cos(yaw);
    double sin_yaw = sin(yaw);

    result.x = (float)(forward * sin_yaw * cos_pitch + right * cos_yaw);
    result.y = (float)(forward * -sin_pitch + vertical);
    result.z = (float)(forward * -cos_yaw * cos_pitch + right * sin_yaw);
    return result;
}
