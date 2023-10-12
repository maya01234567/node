/* MQTT Mutual Authentication Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "esp_log.h"
#include "app_config.h"
#include "mqtt_app.h"
#include "app_nvs.h"
#include "touch_pad_app.h"
#include "jSON_APP.h"
#include "cJSON.h"
#include "app_ota.h"
#include "output_iot.h"
#include "dht_iot.h"
#include "cJSON.h"

static const char *TAG = "MQTTS_APP_MAIN";
#define KEY "restart_cnt"
#define KEY2 "string"

static TaskHandle_t myTaskHandle1 = NULL;

static float fhumidity;
static float ftemperature;
static float fhumidity22;
static float ftemperature22;
char REQUEST[512];

void create_json_string(float temperature, float humidity, float temperature1, float humidity1)
{
    // Tạo một đối tượng cJSON
    cJSON *data = cJSON_CreateObject();

    // Thêm các trường vào đối tượng JSON
    cJSON_AddItemToObject(data, "temperature", cJSON_CreateNumber(temperature));
    cJSON_AddItemToObject(data, "humidity", cJSON_CreateNumber(humidity));
    cJSON_AddItemToObject(data, "temperature1", cJSON_CreateNumber(temperature1));
    cJSON_AddItemToObject(data, "humidity1", cJSON_CreateNumber(humidity1));
    // cJSON_AddItemToObject(data, "LED", cJSON_CreateNumber(1));

    // Chuyển đối tượng cJSON thành chuỗi JSON
    char *json_string = cJSON_Print(data);
    app_mqtt_publish("/v1.6/devices/demo2", json_string);

    // In chuỗi JSON
    printf("JSON String: %s\n", json_string);

    // Giải phóng bộ nhớ được cấp phát cho đối tượng cJSON và chuỗi JSON
    cJSON_Delete(data);
    free(json_string);
}

void datahandel(void *pvParameters)
{
    for (;;)
    {
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        dht_read_float_data(DHT_TYPE_AM2301, GPIO_NUM_15, &fhumidity22, &ftemperature22);
        dht_read_float_data(DHT_TYPE_DHT11, GPIO_NUM_5, &fhumidity, &ftemperature);
        fhumidity -= 10;
        ftemperature -= 3;
        fhumidity22 -= 10;
        ftemperature22 -= 3;
        // recive_data_dht(ftemperature, fhumidity, ftemperature22, fhumidity22);
        printf("humidity:%f\n", fhumidity);
        printf("temperature:%f\n", ftemperature);
        printf("humidity22:%f\n", fhumidity22);
        printf("temperature22:%f\n\n", ftemperature22);
        create_json_string(ftemperature, fhumidity, ftemperature22, fhumidity22);
        // sprintf(REQUEST, "{"temperature": %3.2f,"humidity": %3.2f,"temperature1": %3.2f,"humidity1": %3.2f}", ftemperature, fhumidity, ftemperature22, fhumidity22);
        // const char *resp_str = (const char *)REQUEST;
    }
}

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == EVENT_DEV_BASE)
    {
        if (event_id == MQTT_DEV_EVENT_CONNECTED)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_CONNECTED");
            app_mqtt_subscribe("/v1.6/devices/demo2/#");
            app_mqtt_subscribe("/v1.6/devices/demo2/led/lv");
            vTaskResume( myTaskHandle1 );
        }
        else if (event_id == MQTT_DEV_EVENT_DISCONNECT)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_DISCONNECT");
        }
        else if (event_id == MQTT_DEV_EVENT_DATA)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_DATA");
        }
        else if (event_id == MQTT_DEV_EVENT_SUBSCRIBED)
        {
            ESP_LOGW(TAG, "Detected MQTT_DEV_EVENT_SUBSCRIBED");
            //app_mqtt_publish("/v1.6/devices/demo2", "hello");
        }
    }
}

void url_data(char* topic,int length, char *url)
{
    printf("topic : %s \n", topic);
    if(strstr(topic,"led")&&strstr(topic,"lv"))
    {
        if(atoi(url) == 0)
        {
            printf("off \n");
            output_io_set_level(GPIO_NUM_2, 0);
        }else if(atoi(url) == 1)
        {
            printf("on \n");
            output_io_set_level(GPIO_NUM_2, 1);
        }
    }
    // char data[100] = "";
    cJSON *root = NULL;
    root = cJSON_Parse(url);
    if (root != NULL)
    {
        cJSON *url_link = cJSON_GetObjectItemCaseSensitive(root, "url");
        if (url != NULL && cJSON_IsString(url_link))
        {
            // printf("url : %s \n", url_link->valuestring);
            ESP_LOGI(TAG, "Detected 'url' field in JSON : %s ", url_link->valuestring);
        }
        else
        {
            ESP_LOGW(TAG, "Invalid or missing 'url' field in JSON");
        }
        cJSON *cmd_update = cJSON_GetObjectItemCaseSensitive(root, "cmd_update");
        if (cmd_update != NULL && cJSON_IsNumber(cmd_update))
        {
            // printf("url : %s \n", url_link->valuestring);
            ESP_LOGI(TAG, "Detected 'cmd_update' field in JSON : %d ", cmd_update->valueint);
            if (cmd_update->valueint == 1)
            {
                app_ota_start((const char *)url_link->valuestring);
            }
        }
        else
        {
            ESP_LOGW(TAG, "Invalid or missing 'cmd_update' field in JSON");
        }
        cJSON *LED = cJSON_GetObjectItemCaseSensitive(root, "led");
        if (LED != NULL && cJSON_IsNumber(LED))
        {
            // printf("url : %s \n", url_link->valuestring);
            ESP_LOGI(TAG, "Detected 'LED' field in JSON : %d ", LED->valueint);
            if (LED->valueint == 1)
            {
                output_io_set_level(GPIO_NUM_2, 1);
            }
            else if (LED->valueint == 0)
            {
                output_io_set_level(GPIO_NUM_2, 0);
            }
        }
        else
        {
            ESP_LOGW(TAG, "Invalid or missing 'LED' field in JSON");
        }
        // if (cmd_update->valueint == 1)
        // {
        // app_ota_start((const char *)url_link->valuestring);
        // }
        // if (LED->valueint == 1)
        // {
        // output_io_set_level(GPIO_NUM_2, 1);
        // }
        // else if (LED->valueint == 0)
        // {
        // output_io_set_level(GPIO_NUM_2, 0);
        // }
        cJSON_Delete(root);
    }
    else
    {
        ESP_LOGW(TAG, "Failed to parse JSON data");
    }
}

void app_main(void)
{
    output_io_creat(GPIO_NUM_2);
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);
    // ota_set_callback(&url_data);
    mqtt_data_set_callback(&url_data);
    ESP_ERROR_CHECK(nvs_flash_init());
    /*test update data nvs*/
    int rs = 1;
    app_nvs_get_value(KEY, &rs);
    rs += 1;
    app_nvs_set_value(KEY, &rs);
    char mang_set[50] = "";
    sprintf(mang_set, "Hello World %d\n", rs);
    char mang[50];
    app_nvs_get_str(KEY2, mang);
    app_nvs_set_str(KEY2, mang_set);

    xTaskCreate(datahandel, "datahandel", 2048, NULL, 4, &myTaskHandle1);
    vTaskSuspend(myTaskHandle1);

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_event_handler_instance_t instance_any_id;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(EVENT_DEV_BASE,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    app_config();
    // app_ota_start();// test ota
    mqtt_app_init();
}
