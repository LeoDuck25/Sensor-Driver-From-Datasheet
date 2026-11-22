#ifndef SENSOR_MATH_H
#define SENSOR_MATH_H

#include <stdio.h>
#include <stdint.h>

float temperature_calc(int16_t raw_temp);

typedef struct {
    float accel_x;
    float accel_y;
    float accel_z;
} acceleration;

acceleration accel_calc(int16_t raw_accelx,int16_t raw_accely,int16_t raw_accelz, uint8_t accel_sen);

int16_t combine_byte(uint8_t hi, uint8_t lo);

#endif