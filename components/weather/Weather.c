/*
 * Weather.c
 *
 *  Created on: 03.01.2018
 *      Author: thomas
 */
#include "Weather.h"


static const char* TAG = "WeatherDisplay";

int count = 0;

void refreshTask(void *pvParameter)
{
    while(1) {
    	count++;
    	ESP_LOGI(TAG, "BEFORE REFRESH=%d",esp_get_free_heap_size());
    	EPD_FillScreen(EPD_WHITE);
    	displayBase();
    	displayLocal();
        displayApi();
    	EPD_Update();
    	ESP_LOGI(TAG, "AFTER REFRESH=%d",esp_get_free_heap_size());
        vTaskDelay(60000 / portTICK_RATE_MS);
    }
}

void startWeatherDisplay() {
	EPD_HW_Init();
	SPI_HW_Init(MOSI_Pin, MISO_Pin, SCK_Pin, GPIO_NUM_5);
	I2C_HW_Init(DEFAULT_SDA_PIN, DEFAULT_CLK_PIN);

	BME280_Init(BME280_ADDRESS);

	xTaskCreate(&refreshTask, "refreshTask", 10000, NULL, 5, NULL);
}


void displayBase() {
	EPD_SetFont(6, NULL);
	EPD_DrawText((char *)"Wetter Station", 0, 0);

	char buffer [50];
	sprintf(buffer, "Count: %d",  count);

	EPD_DrawText(buffer, 400, 0);

	EPD_DrawFastHLine(0, 120, EPD_WIDTH, EPD_BLACK);
}

void displayApi() {
	getData();
}

void displayLocal() {
	bme280_reading_data sensor_data = BME280_ReadSensorData();

	EPD_SetFont(2, NULL);

	char buffer [50];
	sprintf(buffer, "Temperatur: %0.2f °C",  (double)sensor_data.temperature);

	ESP_LOGI(TAG, "%s", buffer);

	EPD_DrawText(buffer, 0, 40);

	sprintf(buffer, "Luftfeuchte: %0.2f %%",  (double)sensor_data.humidity);

	EPD_DrawText(buffer, 0, 65);

	sprintf(buffer, "Luftdruck: %0.2f hPa",  (double)sensor_data.pressure/100);

	EPD_DrawText(buffer, 0, 90);
}
