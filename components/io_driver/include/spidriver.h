#ifndef COMPONENTS_IO_DRIVER_INCLUDE_SPIDRIVER_H_
#define COMPONENTS_IO_DRIVER_INCLUDE_SPIDRIVER_H_
#include <driver/spi_master.h>
#include <driver/gpio.h>
#include "esp_log.h"

void SPI_HW_Init(int mosiPin, int misoPin, int clkPin, int csPin);
void SPI_Transfer(uint8_t* data, uint8_t dataLen);
uint8_t SPI_TransferByte(uint8_t value);

#endif
