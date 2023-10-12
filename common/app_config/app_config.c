
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_wifi_netif.h"
#include "smart_config.h"
#include "app_config.h"
#include "wifi_iot.h"
#include "http_sever_sta.h"
#include "wifi_sofAP.h"

#include "lwip/err.h"
#include "lwip/sys.h"
static wifi_config_t wifi_config1;
static provision_type_t provisition_type = PROVISITION_SMARTCONFIG;
static const char *TAG = "app config";

void set_callback_wifiinfo(char *ssid, int lengssid, char *pass, int lengpass)
{
    printf("=>>ssid:%s\n", ssid);
    printf("=>>ssid:%s\n", pass);
    ESP_ERROR_CHECK(esp_wifi_stop());
    delete_netif_sofAP();
    wifi_connect(ssid, pass, 3, WIFI_MODE_STA, ESP_IF_WIFI_STA);
}
int is_provisioned()
{
    static int provisioned;
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    esp_wifi_get_config(WIFI_IF_STA, &wifi_config1);
    ESP_LOGI(TAG, "infor to ap SSID:%s password:%s",
             (uint8_t *)wifi_config1.sta.ssid, (uint8_t *)wifi_config1.sta.password);
    if (wifi_config1.sta.ssid[0] == 0x00) // hien tai = 0 khong cos gi o flash
    {
        provisioned = 1;
    }
    else
    {
        provisioned = 0;
    }
    printf("%d\n", provisioned);
    return provisioned;
}
void app_config(void)
{
    int x = is_provisioned();
    if (x == 1)
    {
        if (provisition_type == PROVISITION_SMARTCONFIG)
        {
            //printf("===========>smart\n");
            smart_config_connect();
        }
        else if (provisition_type == PROVISITION_ACCESSPOIN)
        {
            wifi_init_softap();
            start_wedsever_connect_sta();
            http_set_callback_infowifi(&set_callback_wifiinfo);
            //printf("===========>\n");
        }
    }
    else
    {
        //printf("===========> direct\n");
        connect_direction(wifi_config1.sta.ssid, wifi_config1.sta.password);
    }
}