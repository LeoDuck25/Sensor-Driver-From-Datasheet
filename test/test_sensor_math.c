// test/test_sensor_math.c
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include "../main/sensor_math.h"

// Floats aren't exact, so compare with a tolerance, never ==
static int close(float a, float b) { return fabsf(a - b) < 1e-4f; }

int test_1(void) {
    acceleration a = accel_calc(16384, -16384, 0, 0);
    assert(close(a.accel_x,  1.0f));
    assert(close(a.accel_y, -1.0f));
    assert(close(a.accel_z,  0.0f));

    // Temperature: raw 0 → 36.53 °C (datasheet formula)
    assert(close(temperature_calc(0), 36.53f));

    printf("(1) All tests passed\n");
    return 0;
}

int test_2(void) {
    int16_t test = combine_byte(0xC0, 0x00);
    acceleration a = accel_calc(test, 0x7FFF, 0, 0);
    assert(close(a.accel_x, -1.0f));
    assert(close(a.accel_y, 32767.0f / 16384.0f));
    assert(a.accel_y < 2.0f);  

    assert(close(temperature_calc(-340), 35.53f));

    printf("(2) All tests passed\n");
    return 0;
}

int test_3(void) {
    assert(combine_byte(0xC0, 0x00) == -16384);
    assert(combine_byte(0x80, 0x00) == -32768);
    assert(combine_byte(0x7F, 0xFF) ==  32767);
    assert(combine_byte(0x00, 0x01) ==  1);

    acceleration a = accel_calc(0x8000, 0, 0, 0);
    assert(close(a.accel_x, -2.0f));

    acceleration b = accel_calc(16384, 0, 0, 0);
    acceleration c = accel_calc(16384, 0, 0, 1);
    acceleration d = accel_calc(16384, 0, 0, 2);
    acceleration e = accel_calc(16384, 0, 0, 3);

    assert(close(b.accel_x, 1.0f));
    assert(close(c.accel_x, 2.0f));
    assert(close(d.accel_x, 4.0f));
    assert(close(e.accel_x, 8.0f));

    printf("(3) All tests passed\n");

    return 0;
}

int main(void) {
    test_1();
    test_2();
    test_3();
}