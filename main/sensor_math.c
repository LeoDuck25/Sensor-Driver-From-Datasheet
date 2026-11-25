#include "sensor_math.h"
#include <stdio.h>
#include <stdint.h>

//Receive a signed 16 bit binary and outputs the measured temperature according to datasheet formula
float temperature_calc(int16_t raw_temp) {
    float output_temp = raw_temp/340.0f + 36.53f;
    return output_temp;
}

acceleration accel_calc(int16_t raw_accelx,int16_t raw_accely,int16_t raw_accelz, uint8_t accel_sen) {
    int sensitivity_arr[4] = {16384, 8192, 4096, 2048};
    if (accel_sen > sizeof(sensitivity_arr)) {
        printf("acceleration sensitivity configuration value out of index, defaulting to max value (3)\n");
        accel_sen = 3;
    }
    int sensitivity = sensitivity_arr[accel_sen];
    //Is adding (float) enough to not let C eveporate my floating points?
    float output_x = (float) raw_accelx / sensitivity;
    float output_y = (float) raw_accely / sensitivity;
    float output_z = (float) raw_accelz / sensitivity;

    acceleration output = {output_x, output_y, output_z};
    return output;
}

float calibrate_accel_z(float accel_z) {
    float offset = -0.3570;
    float scale = 1.0176;
    float output = (accel_z - offset)/scale;

    return output;
}

float calibrate_accel_y(float accel_y) {
    float offset = -0.0015;
    float scale = 1.0005;
    float output = (accel_y - offset)/scale;

    return output;
}

float calibrate_accel_x(float accel_x) {
    float offset = 0.0382;
    float scale = 0.9924;
    float output = (accel_x - offset)/scale;

    return output;
}

int16_t combine_byte(uint8_t hi, uint8_t lo) {
    int16_t output = (int16_t)((uint16_t)(hi<<8)|lo);
    return output;
}

//Gyroscope same thing as acceleration