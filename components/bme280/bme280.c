#include "bme280.h"

static int debug = 0;

static const char* TAG = "BME280";

uint8_t address;
int32_t _sensor_id;
int32_t _t_fine;
bme280_calib_data _bme280_calib;

void BME280_Init(uint8_t i2cAddress) {
	address = i2cAddress;

	_sensor_id = BME280_ReadChipId();
	if (_sensor_id != 0x60) {
		if (debug) {
			ESP_LOGD(TAG, "Could not read chip id during initialization");
		}

		return ESP_ERR_NOT_FOUND;
	}

	if (debug) {
		ESP_LOGD(TAG, "Sucessfully read chipId");
	}

	vTaskDelay(1000/portTICK_PERIOD_MS);

	BME280_ReadCoefficients();

	BME280_WriteRegister8(BME280_REGISTER_CONTROLHUMID, 0x05); // 16x oversampling
	BME280_WriteRegister8(BME280_REGISTER_CONTROL, 0xB7);      // 16x oversampling, normal mode

	return ESP_OK;
}

uint8_t BME280_ReadChipId() {
    if (debug) {
        ESP_LOGD(TAG, "readChipId()");
    }

    uint8_t chip_id = BME280_ReadRegister8(BME280_REGISTER_CHIPID);

    if (debug) {
		ESP_LOGD(TAG, "readChipId() %d", chip_id);
	}

    return chip_id;
}

void BME280_ReadCoefficients(void) {
    if (debug) {
        ESP_LOGD(TAG, "readCoefficients()");
    }

    _bme280_calib.dig_T1 = (uint16_t)BME280_Read16BitLittleEndianRegister(BME280_REGISTER_DIG_T1);
    _bme280_calib.dig_T2 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_T2);
    _bme280_calib.dig_T3 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_T3);
    if (debug) {
       ESP_LOGD(TAG, "T1: %u, T2: %d, T3: %d", _bme280_calib.dig_T1, _bme280_calib.dig_T2, _bme280_calib.dig_T3);
    }

    _bme280_calib.dig_P1 = (uint16_t)BME280_Read16BitLittleEndianRegister(BME280_REGISTER_DIG_P1);
    _bme280_calib.dig_P2 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P2);
    _bme280_calib.dig_P3 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P3);
    _bme280_calib.dig_P4 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P4);
    _bme280_calib.dig_P5 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P5);
    _bme280_calib.dig_P6 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P6);
    _bme280_calib.dig_P7 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P7);
    _bme280_calib.dig_P8 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P8);
    _bme280_calib.dig_P9 = (int16_t)BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_P9);
    if (debug) {
        ESP_LOGD(TAG, "P1: %u, P2: %d, P3: %d, P4: %d, P5: %d, P6: %d, P7: %d, P8: %d, P9: %d",
            _bme280_calib.dig_P1,
            _bme280_calib.dig_P2,
            _bme280_calib.dig_P3,
            _bme280_calib.dig_P4,
            _bme280_calib.dig_P5,
            _bme280_calib.dig_P6,
            _bme280_calib.dig_P7,
            _bme280_calib.dig_P8,
            _bme280_calib.dig_P9);
    }

    _bme280_calib.dig_H1 = BME280_ReadRegister8(BME280_REGISTER_DIG_H1);
    _bme280_calib.dig_H2 = BME280_Read16BitSignedLittleEndianRegister(BME280_REGISTER_DIG_H2);
    _bme280_calib.dig_H3 = BME280_ReadRegister8(BME280_REGISTER_DIG_H3);
    _bme280_calib.dig_H4 = (BME280_ReadRegister8(BME280_REGISTER_DIG_H4) << 4) |
    (BME280_ReadRegister8(BME280_REGISTER_DIG_H4 + 1) & 0xF);
    _bme280_calib.dig_H5 = (BME280_ReadRegister8(BME280_REGISTER_DIG_H5 + 1) << 4) |
    (BME280_ReadRegister8(BME280_REGISTER_DIG_H5) >> 4);
    _bme280_calib.dig_H6 = (int8_t) BME280_ReadRegister8(BME280_REGISTER_DIG_H6);
}

bme280_reading_data BME280_ReadSensorData() {
    if (debug) {
        ESP_LOGD(TAG, "readSensorData()");
    }
    bme280_reading_data reading_data;

    bme280_adc_data adc_data = BME280_BurstReadMeasurement();

    reading_data.temperature = BME280_ConvertUncompensatedTemperature(adc_data.adc_data.adc_T);
    reading_data.pressure = BME280_ConvertUncompensatedPressure(adc_data.adc_data.adc_P);
    reading_data.humidity = BME280_ConvertUncompensatedHumidity(adc_data.adc_data.adc_H);

    return reading_data;
}

bme280_adc_data BME280_BurstReadMeasurement() {
    uint8_t buffer[8];

    uint8_t reg = BME280_REGISTER_PRESSUREDATA;

    I2C_BeginTransaction();
    I2C_WriteByte(reg, 1);
    I2C_EndTransaction();

    I2C_BeginTransaction();
    I2C_Read(buffer, 7, 1);
    I2C_ReadByte(&buffer[7], 0);
    I2C_EndTransaction();

    bme280_adc_data adc_data;

    adc_data.buffer.pressure.xmsb = 0;
    adc_data.buffer.pressure.msb = buffer[0];
    adc_data.buffer.pressure.lsb = buffer[1];
    adc_data.buffer.pressure.xlsb = buffer[2];

    adc_data.buffer.temperature.xmsb = 0;
    adc_data.buffer.temperature.msb = buffer[3];
    adc_data.buffer.temperature.lsb = buffer[4];
    adc_data.buffer.temperature.xlsb = buffer[5];

    adc_data.buffer.humidity.xmsb = 0;
    adc_data.buffer.humidity.msb = 0;
    adc_data.buffer.humidity.lsb = buffer[6];
    adc_data.buffer.humidity.xlsb = buffer[7];
    adc_data.adc_data.adc_P = (BME280_S32_t)((buffer[0] << 16) | (buffer[1] << 8) | buffer[2]);
    adc_data.adc_data.adc_T = (BME280_S32_t)((buffer[3] << 16) | (buffer[4] << 8) | buffer[5]);
    adc_data.adc_data.adc_H = (BME280_S32_t)((buffer[6] << 8) | buffer[7]);
    return adc_data;
}

// Returns temperature in DegC, resolution is 0.01 DegC. Output value of “5123” equals 51.23 DegC.
// t_fine carries fine temperature as global value
BME280_S32_t BME280_Compensate_T(BME280_S32_t adc_T) {
    BME280_S32_t var1, var2, T;

    var1 = ((((adc_T>>3) - ((BME280_S32_t)_bme280_calib.dig_T1<<1))) * ((BME280_S32_t)_bme280_calib.dig_T2)) >> 11;
    var2 = (((((adc_T>>4) - ((BME280_S32_t)_bme280_calib.dig_T1)) * ((adc_T>>4) - ((BME280_S32_t)_bme280_calib.dig_T1))) >> 12) *
        ((BME280_S32_t)_bme280_calib.dig_T3)) >> 14;

    _t_fine = var1 + var2;

    T = (_t_fine * 5 + 128) >> 8;

    return T;
}

// Returns pressure in Pa as unsigned 32 bit integer in Q24.8 format (24 integer bits and 8 fractional bits).
// Output value of “24674867” represents 24674867/256 = 96386.2 Pa = 963.862 hPa
BME280_U32_t BME280_Compensate_P(BME280_S32_t adc_P) {
    BME280_S64_t var1, var2, p;

    var1 = ((BME280_S64_t)_t_fine) - 128000;
    var2 = var1 * var1 * (BME280_S64_t)_bme280_calib.dig_P6;
    var2 = var2 + ((var1*(BME280_S64_t)_bme280_calib.dig_P5) << 17);
    var2 = var2 + (((BME280_S64_t)_bme280_calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (BME280_S64_t)_bme280_calib.dig_P3) >> 8) + ((var1 * (BME280_S64_t)_bme280_calib.dig_P2) << 12);
    var1 = (((((BME280_S64_t)1) << 47) + var1)) * ((BME280_S64_t)_bme280_calib.dig_P1 ) >> 33;

    if (var1 == 0)
    {
        return 0; // avoid exception caused by division by zero
    }

    p = 1048576 - adc_P;
    p = (((p<<31) - var2) * 3125) / var1;
    var1 = (((BME280_S64_t)_bme280_calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((BME280_S64_t)_bme280_calib.dig_P8) * p) >> 19;
    p = ((p + var1 + var2) >> 8) + (((BME280_S64_t)_bme280_calib.dig_P7) << 4);

    return (BME280_U32_t)p;
}

// Returns humidity in %RH as unsigned 32 bit integer in Q22.10 format (22 integer and 10 fractional bits).
// Output value of “47445” represents 47445/1024 = 46.333 %RH
BME280_U32_t BME280_Compensate_H(BME280_S32_t adc_H) {
    BME280_S32_t v_x1_u32r;

    v_x1_u32r = (_t_fine - ((BME280_S32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((BME280_S32_t)_bme280_calib.dig_H4) << 20) - (((BME280_S32_t)_bme280_calib.dig_H5) * v_x1_u32r)) +
        ((BME280_S32_t)16384)) >> 15) * (((((((v_x1_u32r * ((BME280_S32_t)_bme280_calib.dig_H6)) >> 10) * (((v_x1_u32r *
        ((BME280_S32_t)_bme280_calib.dig_H3)) >> 11) + ((BME280_S32_t)32768))) >> 10) + ((BME280_S32_t)2097152)) *
        ((BME280_S32_t)_bme280_calib.dig_H2) + 8192) >> 14));
    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((BME280_S32_t)_bme280_calib.dig_H1)) >> 4));
    v_x1_u32r = (v_x1_u32r < 0 ? 0 : v_x1_u32r);
    v_x1_u32r = (v_x1_u32r > 419430400 ? 419430400 : v_x1_u32r);

    return (BME280_U32_t)(v_x1_u32r >> 12);
}

float BME280_ConvertUncompensatedTemperature(int32_t adc_T) {
    adc_T >>= 4;
    BME280_S32_t T = BME280_Compensate_T(adc_T);

    return (float) T / 100.0;
}

float BME280_ConvertUncompensatedPressure(BME280_S32_t adc_P) {
    adc_P >>= 4;
    BME280_U32_t p = BME280_Compensate_P(adc_P);

    return (float) p / 256;
}

float BME280_ConvertUncompensatedHumidity(BME280_S32_t adc_H) {
    BME280_U32_t h = BME280_Compensate_H(adc_H);

    return h / 1024.0;
}

uint16_t BME280_Read16BitLittleEndianRegister(uint8_t register_address) {
    uint16_t temp = BME280_Read16BitBigEndianRegister(register_address);

    return (temp >> 8) | (temp << 8);
}

int16_t BME280_Read16BitSignedLittleEndianRegister(uint8_t register_address) {
    return (int16_t) BME280_Read16BitLittleEndianRegister(register_address);
}

uint16_t BME280_Read16BitBigEndianRegister(uint8_t reg) {
    I2C_BeginTransaction();
    I2C_WriteByte(reg, 1);
    I2C_EndTransaction();

    uint8_t msb;
	uint8_t lsb;
    I2C_BeginTransaction();
    I2C_ReadByte(&msb, 1);
    I2C_ReadByte(&lsb, 0);
    I2C_EndTransaction();

    uint16_t ret = (uint16_t)((msb << 8) | lsb);
	return ret;
}

uint32_t BME280_ReadRegister24(uint8_t reg) {
    I2C_BeginTransaction();
    I2C_WriteByte(reg, 1);
    I2C_EndTransaction();

    uint8_t msb;
	uint8_t lsb;
	uint8_t xlsb;
	I2C_BeginTransaction();
	I2C_ReadByte(&msb, 1);
	I2C_ReadByte(&lsb, 1);
	I2C_ReadByte(&xlsb, 0);
    I2C_EndTransaction();

    uint32_t ret = (uint32_t)((msb << 16) | (lsb << 8) | xlsb);
	return ret;
}

uint8_t BME280_ReadRegister8(uint8_t reg) {
    I2C_BeginTransaction();
    I2C_WriteByte(reg, 1);
    I2C_EndTransaction();

	uint8_t value;
	I2C_BeginTransaction();
    I2C_ReadByte(&value, 0);
    I2C_EndTransaction();

	return value;
}

esp_err_t BME280_WriteRegister8(uint8_t register_address, uint8_t data) {
    I2C_BeginTransaction();
    I2C_WriteByte(register_address, 1);
    I2C_WriteByte(data, 1);
    I2C_EndTransaction();

    return ESP_OK;
}
