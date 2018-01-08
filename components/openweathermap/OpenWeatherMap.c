/*
 * Weather.c
 *
 *  Created on: 03.01.2018
 *      Author: thomas
 */
#include "OpenWeatherMap.h"
#include "cJSON.h"
#include "epd.h"

static const char* TAG = "OpenWeatherMap";

char *json;
int length = 0;

int download_callback(request_t *req, char *data, int len)
{

    req_list_t *found = req->response->header;
    while(found->next != NULL) {
        found = found->next;
    }
    found = req_list_get_key(req->response->header, "Content-Length");
    if(found) {
        if(json == NULL) {
			 json = malloc(req->response->len);
		}
    }
    memcpy(json + length, data, len);

    length = length + len;

    return 0;
}

void downloadComplete_callback() {
	ESP_LOGI(TAG, "DOWNLOAD COMPLETE");

	cJSON *apiObj = cJSON_Parse(json);

	ESP_LOGI(TAG, "DOWNLOAD COMPLETE JSON %d", apiObj != NULL);
	char buffer [50];

	cJSON *subitem = NULL;
	cJSON *temp = NULL;
	cJSON *icon = NULL;
	struct tm * dt = NULL;
	char timestr[30];
	char path[50];
	char *iconName = NULL;
	time_t *rawtime = NULL;

	int y = 0;
	for(int i = 1; i < cJSON_GetArraySize(cJSON_GetObjectItem(apiObj, "list")); i+=8) {

		subitem =cJSON_GetArrayItem(cJSON_GetObjectItem(apiObj, "list"),i);

		EPD_SetFont(DEJAVU18_FONT, NULL);

		rawtime = (time_t)cJSON_GetObjectItem(subitem,"dt")->valueint;
		dt = localtime(&rawtime);
		strftime(timestr, sizeof(timestr), "%d.%m.%Y", dt);

		sprintf(buffer, "%s", timestr);
		EPD_DrawText(buffer, 20 +(y*120), 140);

		strftime(timestr, sizeof(timestr), "%H:%M", dt);

		sprintf(buffer, "%s", timestr);
		EPD_DrawText(buffer, 40 +(y*120), 160);

		temp = cJSON_GetObjectItem(subitem,"main");

		sprintf(buffer, "%0.2f C", cJSON_GetObjectItem(temp,"temp")->valuedouble);
		EPD_DrawText(buffer, 50 +(y*120), 180);

		icon =cJSON_GetArrayItem(cJSON_GetObjectItem(subitem, "weather"),0);

		iconName = cJSON_GetObjectItem(icon,"icon")->valuestring;
		sprintf(path,"%s/images/%s.jpg", SPIFFS_BASE_PATH, iconName);

		ESP_LOGI(TAG, "Path %s", path);

		EPD_DrawImageJpg(20 +(y*120), 220, 0, path, NULL, 0);
		if(y < 4) {
			EPD_DrawFastVLine(135+(y*120), 140, 200, EPD_BLACK);
		}
		y++;

	}
	cJSON_Delete(apiObj);

	free(json);
	json = NULL;
	free(json);
	length = 0;
}

void getData() {
	request_t *req;

	int status;
	xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
	ESP_LOGI(TAG, "Connected to AP, freemem=%d",esp_get_free_heap_size());
	// vTaskDelay(1000/portTICK_RATE_MS);
	req = req_new("http://api.openweathermap.org/data/2.5/forecast?id=3302146&lang=de_DE&units=metric&appid=ea857f910cf094092c0f5cbb4ee5d2da");
	//or
	//request *req = req_new("https://google.com"); //for SSL
	req_setopt(req, REQ_SET_METHOD, "GET");
	req_setopt(req, REQ_FUNC_DOWNLOAD_CB, download_callback);
	req_setopt(req, REQ_FUNC_DOWNLOADCOMPLETE_CB, downloadComplete_callback);
	status = req_perform(req);
	ESP_LOGI(TAG,"REQCLEAN");
	req_clean(req);
	ESP_LOGI(TAG,"REQCLEAN FINISH");

}

