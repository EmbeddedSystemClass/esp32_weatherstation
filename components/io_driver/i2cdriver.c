#include "i2cdriver.h"

static int debug = 0;

static const char* TAG = "I2C";

static int driverInstalled = 0;
i2c_cmd_handle_t cmd;
int directionKnown;
uint8_t address;

void I2C_HW_Init(int sdaPin, int clkPin) {
	ESP_LOGD(TAG, "init: sda=%d, clk=%d", sdaPin, clkPin);
	i2c_config_t conf;
	conf.mode = I2C_MODE_MASTER;
	conf.sda_io_num       = sdaPin;
	conf.scl_io_num       = clkPin;
	conf.sda_pullup_en    = GPIO_PULLUP_ENABLE;
	conf.scl_pullup_en    = GPIO_PULLUP_ENABLE;
	conf.master.clk_speed = 100000;
	ESP_ERROR_CHECK(i2c_param_config(I2C_NUM_0, &conf));
	if (!driverInstalled) {
		ESP_ERROR_CHECK(i2c_driver_install(I2C_NUM_0, I2C_MODE_MASTER, 0, 0, 0));
		driverInstalled = 1;
	}

} // init

void I2C_SetAddress(uint8_t i2cAddress) {
    address = i2cAddress;
}


void I2C_Scan() {
	int i;
	esp_err_t espRc;
	printf("     0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f\n");
	printf("00:         ");
	for (i=3; i< 0x78; i++) {
		i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, (i << 1) | I2C_MASTER_WRITE, 1 /* expect ack */);
		i2c_master_stop(cmd);

		espRc = i2c_master_cmd_begin(I2C_NUM_0, cmd, 100/portTICK_PERIOD_MS);
		if (i%16 == 0) {
			printf("\n%.2x:", i);
		}
		if (espRc == 0) {
			printf(" %.2x", i);
		} else {
			printf(" --");
		}
		//ESP_LOGD(tag, "i=%d, rc=%d (0x%x)", i, espRc, espRc);
		i2c_cmd_link_delete(cmd);
	}
	printf("\n");
} // scan

/**
 * @brief Begin a new %I2C transaction.
 *
 * Begin a transaction by adding an %I2C start to the queue.
 * @return N/A.
 */
void I2C_BeginTransaction() {
	if (debug) {
		ESP_LOGD(TAG, "beginTransaction()");
	}
	cmd = i2c_cmd_link_create();
	ESP_ERROR_CHECK(i2c_master_start(cmd));
	directionKnown = 0;
} // beginTransaction


/**
 * @brief End an I2C transaction.
 *
 * This call will execute the %I2C requests that have been queued up since the preceding call to the
 * beginTransaction() function.  An %I2C stop() is also called.
 * @return N/A.
 */
void I2C_EndTransaction() {
	if (debug) {
		ESP_LOGD(TAG, "endTransaction()");
	}
	ESP_ERROR_CHECK(i2c_master_stop(cmd));
	ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000/portTICK_PERIOD_MS));
	i2c_cmd_link_delete(cmd);
	directionKnown = 0;
} // endTransaction

/**
 * @brief Write a sequence of byte to the %I2C slave.
 *
 * @param [in] bytes The sequence of bytes to write to the %I2C slave.
 * @param [in] length The number of bytes to write.
 * @param [in] ack Whether or not an acknowledgment is expected from the slave.
 * @return N/A.
 */
void I2C_Write(uint8_t *bytes, size_t length, bool ack) {
	if (debug) {
		ESP_LOGD(TAG, "write(length=%d, ack=%d)", length, ack);
	}
	if (directionKnown == 0) {
		directionKnown = 1;
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, !ack));
	}
	ESP_ERROR_CHECK(i2c_master_write(cmd, bytes, length, !ack));
} // write


void I2C_WriteByte(uint8_t byte, bool ack) {
	if (debug) {
		ESP_LOGD(TAG, "write(val=0x%.2x, ack=%d)", byte, ack);
	}
	if (directionKnown == 0) {
		directionKnown = 1;
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_WRITE, !ack));
	}
	ESP_ERROR_CHECK(i2c_master_write_byte(cmd, byte, !ack));
} // write

void I2C_Read(uint8_t* bytes, size_t length, bool ack) {
	if (debug) {
		ESP_LOGD(TAG, "read(size=%d, ack=%d)", length, ack);
	}
	if (directionKnown == 0) {
		directionKnown = 1;
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, !ack));
	}
	ESP_ERROR_CHECK(i2c_master_read(cmd, bytes, length, !ack));
} // read

void I2C_ReadByte(uint8_t *byte, bool ack) {
	if (debug) {
		ESP_LOGD(TAG, "read(size=1, ack=%d)", ack);
	}
	if (directionKnown == 0) {
		directionKnown = 1;
		ESP_ERROR_CHECK(i2c_master_write_byte(cmd, (address << 1) | I2C_MASTER_READ, !ack));
	}
	ESP_ERROR_CHECK(i2c_master_read_byte(cmd, byte, !ack));
} // readByte




