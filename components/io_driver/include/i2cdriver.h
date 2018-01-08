#ifndef COMPONENTS_IO_DRIVER_INCLUDE_I2CDRIVER_H_
#define COMPONENTS_IO_DRIVER_INCLUDE_I2CDRIVER_H_
#include <driver/i2c.h>
#include <driver/gpio.h>
#include "stddef.h"
#include <stdbool.h>
#include "esp_log.h"

#define DEFAULT_SDA_PIN GPIO_NUM_21
#define DEFAULT_CLK_PIN GPIO_NUM_22

void I2C_HW_Init(int sdaPin, int clkPin);
void I2C_Scan();
void I2C_SetAddress(uint8_t address);
void I2C_Read(uint8_t* bytes, size_t length, bool ack);
void I2C_ReadByte(uint8_t *byte, bool ack);
void I2C_Write(uint8_t *bytes, size_t length, bool ack);
void I2C_WriteByte(uint8_t byte, bool ack);
void I2C_BeginTransaction();
void I2C_EndTransaction();

#endif
