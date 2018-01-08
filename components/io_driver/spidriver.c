#include "../io_driver/include/spidriver.h"

static const char* TAG = "SPI";

spi_device_handle_t m_handle;
spi_host_device_t   m_host = HSPI_HOST;

void SPI_HW_Init(int mosiPin, int misoPin, int clkPin, int csPin) {
	ESP_LOGD(TAG, "init: mosi=%d, miso=%d, clk=%d, cs=%d", mosiPin, misoPin, clkPin, csPin);

	spi_bus_config_t bus_config;
	bus_config.sclk_io_num     = clkPin;  // CLK
	bus_config.mosi_io_num     = mosiPin; // MOSI
	bus_config.miso_io_num     = misoPin; // MISO
	bus_config.quadwp_io_num   = -1;      // Not used
	bus_config.quadhd_io_num   = -1;      // Not used
	bus_config.max_transfer_sz = 60000;       // 0 means use default.

	ESP_LOGI(TAG, "... Initializing bus; host=%d", m_host);

	esp_err_t errRc = spi_bus_initialize(
			m_host,
			&bus_config,
			1 // DMA Channel
	);

	if (errRc != ESP_OK) {
		ESP_LOGE(TAG, "spi_bus_initialize(): rc=%d", errRc);
		abort();
	}

	spi_device_interface_config_t dev_config;
	dev_config.address_bits     = 0;
	dev_config.command_bits     = 0;
	dev_config.dummy_bits       = 0;
	dev_config.mode             = 0;
	dev_config.duty_cycle_pos   = 0;
	dev_config.cs_ena_posttrans = 0;
	dev_config.cs_ena_pretrans  = 0;
	dev_config.clock_speed_hz   = 4000000;
	dev_config.spics_io_num     = csPin;
	dev_config.flags            = 0;
	dev_config.queue_size       = 1;
	dev_config.pre_cb           = NULL;
	dev_config.post_cb          = NULL;
	ESP_LOGI(TAG, "... Adding device bus.");
	errRc = spi_bus_add_device(m_host, &dev_config, &m_handle);
	if (errRc != ESP_OK) {
		ESP_LOGE(TAG, "spi_bus_add_device(): rc=%d", errRc);
		abort();
	}
} // init

uint8_t SPI_TransferByte(uint8_t value) {
	SPI_Transfer(&value, 1);
	return value;
} // transferByte

/**
 * @brief Send and receive data through %SPI.  This is a blocking call.
 *
 * @param [in] data A data buffer used to send and receive.
 * @param [in] dataLen The number of bytes to transmit and receive.
 */
void SPI_Transfer(uint8_t* data, uint8_t dataLen) {
	assert(data != NULL);
	assert(dataLen > 0);
	spi_transaction_t trans_desc;
	//trans_desc.address   = 0;
	//trans_desc.command   = 0;
	trans_desc.flags     = 0;
	trans_desc.length    = dataLen * 8;
	trans_desc.rxlength  = 0;
	trans_desc.tx_buffer = data;
	trans_desc.rx_buffer = data;
	trans_desc.user = (void*)1;
	//ESP_LOGI(tag, "... Transferring");
	esp_err_t rc = spi_device_transmit(m_handle, &trans_desc);
	if (rc != ESP_OK) {
		ESP_LOGE(TAG, "transfer:spi_device_transmit: %d", rc);
	}
} // transmit
