#include <stdio.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp_log.h"

#include "sensor_math.h"

static const char *TAG = "MPU6050_DRIVER";

#define I2C_MASTER_SCL_IO           22
#define I2C_MASTER_SDA_IO           21
#define I2C_MASTER_NUM              I2C_NUM_0
#define I2C_MASTER_FREQ_HZ          100000

#define MPU6050_ADDR                0x68
#define MPU6050_WHO_AM_I_REG        0x75
#define MPU6050_PWR_MGMT            0x6B
#define ACCEL_XOUT_H                0x3B
#define ACCEL_CONFIG_ADDR           0x1C
#define ACCEL_CONFIG_VAL            0 //0 by default
//Since the sensor auto increment after every byte read, only need to know the address of the first one
//List of registers that will be read:
//ACCEL_XOUT_L  ACCEL_YOUT_H    ACCEL_YOUT_L    ACCEL_ZOUT_H    ACCEL_ZOUT_L

#define PWR1_DEVICE_RESET_BIT       7
#define PWR1_SLEEP_BIT              6 //Set this to 0
#define PWR1_CYCLE_BIT              5
#define PWR1_TEMP_DIS_BIT           3 //Temperature sensor, 0 by default
#define PWR1_CLKSEL_BIT             0 //Clock source (?)

void addr_check(i2c_master_dev_handle_t dev_handle) {
    uint8_t reg_addr = MPU6050_WHO_AM_I_REG;
    uint8_t who_am_i_val = 0;

    vTaskDelay(pdMS_TO_TICKS(50));

      esp_err_t ret = i2c_master_transmit_receive(
        dev_handle,
        &reg_addr,
        1, 
        &who_am_i_val,
        1,            
        1000             
      );

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Communication succeeded, checking identity");
        if (who_am_i_val == 0x68) {ESP_LOGI(TAG, "Success! WHO_AM_I register (0x75) returned: 0x%02X", who_am_i_val);}
        else {ESP_LOGI(TAG, "Wrong sensor address!"); exit(1);}
    } else {
        ESP_LOGE(TAG, "I2C communication failed: %s", esp_err_to_name(ret));
        exit(1);
    }
}

void wake_up(i2c_master_dev_handle_t dev_handle) {
    uint8_t pwr_reg_val = (0<<PWR1_SLEEP_BIT) | (0<<PWR1_TEMP_DIS_BIT) | (1<<PWR1_CLKSEL_BIT);
    
    //i2c_master_transmit need a array of 2 bytes in a single write frame
    uint8_t write_buf[2] = {MPU6050_PWR_MGMT, pwr_reg_val};

    esp_err_t ret = i2c_master_transmit(
    dev_handle,
    write_buf,
    sizeof(write_buf),
    1000
    );

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Device woken!");
    } else {
        ESP_LOGE(TAG, "Waking failed: %s", esp_err_to_name(ret));
        exit(1);
    }

}

void config(i2c_master_dev_handle_t dev_handle) {
    uint8_t accel_config_val = ACCEL_CONFIG_VAL << 3;
    uint8_t write_buf[2] = {ACCEL_CONFIG_ADDR, accel_config_val};

    esp_err_t ret = i2c_master_transmit(
    dev_handle,
    write_buf,
    sizeof(write_buf),
    1000
    );

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Device configured");
    } else {
        ESP_LOGE(TAG, "Configuration failed: %s", esp_err_to_name(ret));
    }
}

void app_main(void)
{
    i2c_master_bus_config_t bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_MASTER_NUM,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true, 
  };

    i2c_master_bus_handle_t bus_handle;
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));
    ESP_LOGI(TAG, "I2C bus initialized successfully");

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = MPU6050_ADDR,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
  };

    i2c_master_dev_handle_t dev_handle;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

    addr_check(dev_handle);
    wake_up(dev_handle);
    config(dev_handle);

vTaskDelay(pdMS_TO_TICKS(20)); // Let sensor stabilize after waking

    uint8_t sensor_accel_temp_data[8];
    uint8_t start_reg_addr = ACCEL_XOUT_H;

    while (1) {
        esp_err_t ret = i2c_master_transmit_receive(
            dev_handle,
            &start_reg_addr,
            1,
            sensor_accel_temp_data,
            sizeof(sensor_accel_temp_data),
            1000
        );

        if (ret == ESP_OK) {
            //Need to explicitly tell the program that this is a signed binary
            int16_t accel_x_data = (int16_t)(((uint16_t)sensor_accel_temp_data[0] << 8) | sensor_accel_temp_data[1]);
            int16_t accel_y_data = (int16_t)(((uint16_t)sensor_accel_temp_data[2] << 8) | sensor_accel_temp_data[3]);
            int16_t accel_z_data = (int16_t)(((uint16_t)sensor_accel_temp_data[4] << 8) | sensor_accel_temp_data[5]);

            int16_t temp_data = (int16_t)(((uint16_t)sensor_accel_temp_data[6] << 8) | sensor_accel_temp_data[7]);

            float temperature = temperature_calc(temp_data);
            acceleration accelaration = accel_calc(accel_x_data, accel_y_data, accel_z_data, ACCEL_CONFIG_VAL);

            ESP_LOGI(TAG, "Raw Accel -> X: %d, Y: %d, Z: %d", accel_x_data, accel_y_data, accel_z_data);
        } else {
            ESP_LOGE(TAG, "Failed to read accel data: %s", esp_err_to_name(ret));
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
