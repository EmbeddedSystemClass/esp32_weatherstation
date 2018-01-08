#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "../components/io_driver/include/spidriver.h"
#include "wifi.h"
#include "Weather.h"
#include "epd.h"
#include "spiffs_vfs.h"

static const char* TAG = "Main";

void app_main()
{
    nvs_flash_init();
    vfs_spiffs_register();
    initialise_wifi();


    startWeatherDisplay();


}
