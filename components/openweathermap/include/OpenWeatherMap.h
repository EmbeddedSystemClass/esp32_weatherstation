/*
 * weather.h
 *
 *  Created on: 16.12.2017
 *      Author: thomas
 */
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "spiffs_vfs.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_request.h"

#ifndef COMPONENTS_OPENWEATHERMAP_INCLUDE_OPENWEATHERMAP_H_
#define COMPONENTS_OPENWEATHERMAP_INCLUDE_OPENWEATHERMAP_H_

extern EventGroupHandle_t wifi_event_group;
extern const int CONNECTED_BIT;

void getData();

#endif /* COMPONENTS_OPENWEATHERMAP_INCLUDE_OPENWEATHERMAP_H_ */
