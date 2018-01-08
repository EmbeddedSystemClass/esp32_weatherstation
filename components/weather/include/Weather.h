/*
 * weather.h
 *
 *  Created on: 16.12.2017
 *      Author: thomas
 */
#ifndef COMPONENTS_WEATHER_INCLUDE_WEATHER_H_
#define COMPONENTS_WEATHER_INCLUDE_WEATHER_H_
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <stdio.h>
#include "OpenWeatherMap.h"
#include "epd.h"
#include "bme280.h"
#include "i2cdriver.h"

void startWeatherDisplay();
void displayBase();
void displayApi();
void displayLocal();

#endif /* COMPONENTS_WEATHER_INCLUDE_WEATHER_H_ */
