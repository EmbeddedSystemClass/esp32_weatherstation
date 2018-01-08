
#ifndef COMPONENTS_BME280_INCLUDE_BME280_H_
#define COMPONENTS_BME280_INCLUDE_BME280_H_

#ifdef  __cplusplus
extern "C" {
#endif
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "i2cdriver.h"
#include "esp_log.h"
#include <string.h>
#include <errno.h>
#include "epd.h"


/*=========================================================================
I2C ADDRESS/BITS
-----------------------------------------------------------------------*/
#define BME280_ADDRESS (0x77)
/*=========================================================================*/

/*=========================================================================
    REGISTERS
    -----------------------------------------------------------------------*/
enum
{
    BME280_REGISTER_DIG_T1 = 0x88,
    BME280_REGISTER_DIG_T2 = 0x8A,
    BME280_REGISTER_DIG_T3 = 0x8C,

    BME280_REGISTER_DIG_P1 = 0x8E,
    BME280_REGISTER_DIG_P2 = 0x90,
    BME280_REGISTER_DIG_P3 = 0x92,
    BME280_REGISTER_DIG_P4 = 0x94,
    BME280_REGISTER_DIG_P5 = 0x96,
    BME280_REGISTER_DIG_P6 = 0x98,
    BME280_REGISTER_DIG_P7 = 0x9A,
    BME280_REGISTER_DIG_P8 = 0x9C,
    BME280_REGISTER_DIG_P9 = 0x9E,

    BME280_REGISTER_DIG_H1 = 0xA1,
    BME280_REGISTER_DIG_H2 = 0xE1,
    BME280_REGISTER_DIG_H3 = 0xE3,
    BME280_REGISTER_DIG_H4 = 0xE4,
    BME280_REGISTER_DIG_H5 = 0xE5,
    BME280_REGISTER_DIG_H6 = 0xE7,

    BME280_REGISTER_CHIPID = 0xD0,
    BME280_REGISTER_VERSION = 0xD1,
    BME280_REGISTER_SOFTRESET = 0xE0,

    BME280_REGISTER_CAL26 = 0xE1, // R calibration stored in 0xE1-0xF0

    BME280_REGISTER_CONTROLHUMID = 0xF2,
    BME280_REGISTER_STATUS = 0xF3,
    BME280_REGISTER_CONTROL = 0xF4,
    BME280_REGISTER_CONFIG = 0xF5,
    BME280_REGISTER_PRESSUREDATA = 0xF7,
    BME280_REGISTER_TEMPDATA = 0xFA,
    BME280_REGISTER_HUMIDDATA = 0xFD,
};

/*=========================================================================*/

/*=========================================================================
    CALIBRATION DATA
    -----------------------------------------------------------------------*/
typedef struct
{
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;

    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    uint8_t dig_H1;
    int16_t dig_H2;
    uint8_t dig_H3;
    int16_t dig_H4;
    int16_t dig_H5;
    int8_t dig_H6;
} bme280_calib_data;
/*=========================================================================*/

typedef uint32_t BME280_U32_t;
typedef int32_t BME280_S32_t;
typedef int64_t BME280_S64_t;

typedef struct
{
    float temperature;
    float humidity;
    float pressure;
} bme280_reading_data;

typedef struct
{
    struct {
        BME280_S32_t adc_P;
        BME280_S32_t adc_T;
        BME280_S32_t adc_H;
    } adc_data;
    struct {
        struct {
            uint8_t xlsb;
            uint8_t lsb;
            uint8_t msb;
            uint8_t xmsb;
        } pressure;
        struct {
            uint8_t xlsb;
            uint8_t lsb;
            uint8_t msb;
            uint8_t xmsb;
        } temperature;
        struct {
            uint8_t xlsb;
            uint8_t lsb;
            uint8_t msb;
            uint8_t xmsb;
        } humidity;
    } buffer;
} bme280_adc_data;

void BME280_Init(uint8_t i2cAddress);
uint8_t BME280_ReadChipId();
uint8_t BME280_ReadRegister8(uint8_t reg);
esp_err_t BME280_WriteRegister8(uint8_t register_address, uint8_t data);
void BME280_ReadCoefficients(void);
uint16_t BME280_Read16BitLittleEndianRegister(uint8_t register_address);
int16_t BME280_Read16BitSignedLittleEndianRegister(uint8_t register_address);
uint16_t BME280_Read16BitBigEndianRegister(uint8_t reg);
uint32_t BME280_ReadRegister24(uint8_t reg);
bme280_reading_data BME280_ReadSensorData();
bme280_adc_data BME280_BurstReadMeasurement();
float BME280_ConvertUncompensatedHumidity(BME280_S32_t adc_H);
float BME280_ConvertUncompensatedPressure(BME280_S32_t adc_P);
float BME280_ConvertUncompensatedTemperature(int32_t adc_T);
BME280_U32_t BME280_Compensate_H(BME280_S32_t adc_H);
BME280_U32_t BME280_Compensate_P(BME280_S32_t adc_P);
BME280_S32_t BME280_Compensate_T(BME280_S32_t adc_T);

#ifdef  __cplusplus
}
#endif
#endif
