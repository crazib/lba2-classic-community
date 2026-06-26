#include "FREECAM_MATH.H"

#include <assert.h>
#include <math.h>
#include <stdio.h>

static void assert_near(float actual, float expected) {
    assert(fabs(actual - expected) < 0.001f);
}

int main(void) {
    T_FREECAM_VECTOR v;

    assert(FreeCam_NormalizeAngle(-1) == 4095);
    assert(FreeCam_NormalizeAngle(4097) == 1);
    assert(FreeCam_SignedPitch(4095) == -1);

    v = FreeCam_MovementVector(0, 0, 1, 0, 0);
    assert_near(v.x, 0.0f);
    assert_near(v.y, 0.0f);
    assert_near(v.z, -1.0f);

    v = FreeCam_MovementVector(0, 1024, 1, 0, 0);
    assert_near(v.x, 1.0f);
    assert_near(v.y, 0.0f);
    assert_near(v.z, 0.0f);

    v = FreeCam_MovementVector(0, 512, 1, 0, 0);
    assert_near(v.x, 0.707106f);
    assert_near(v.y, 0.0f);
    assert_near(v.z, -0.707106f);

    v = FreeCam_MovementVector(0, 0, 0, 1, 1);
    assert_near(v.x, 1.0f);
    assert_near(v.y, 1.0f);
    assert_near(v.z, 0.0f);

    printf("test_freecam_math: OK\n");
    return 0;
}
