#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"
#include "driver/uart.h"

#include "freertos/timers.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "cJSON.h"

#include "sdkconfig.h"

#include "string.h"
#include "linky.h"
#include "main.h"
#include "config.h"
#include "wifi.h"
#include "shell.h"
#include "mqtt.h"
#include "gpio.h"
#include "web.h"
#include "zigbee.h"

Config config;

#define MAX_DATA_INDEX 5
// ------------Global variables stored in RTC memory to keep their values after deep sleep
RTC_DATA_ATTR LinkyData dataArray[MAX_DATA_INDEX];

RTC_DATA_ATTR unsigned int dataIndex = 0;
// RTC_DATA_ATTR uint8_t firstBoot = 1;
// ---------------------------------------------------------------------------------------

TaskHandle_t fetchLinkyDataTaskHandle = NULL;
TaskHandle_t pushButtonTaskHandle = NULL;
TaskHandle_t pairingTaskHandle = NULL;

#define MAIN_TAG "MAIN"
extern "C" void app_main(void)
{
  // setCPUFreq(10);
  ESP_LOGI(MAIN_TAG, "Starting ESP32 Linky...");
  initPins();
  startLedPattern(PATTERN_START);
  xTaskCreate(pairingButtonTask, "pushButtonTask", 8192, NULL, 1, &pushButtonTaskHandle); // start push button task

  config.begin();
  shellInit(); // init shell

  if (config.verify())
  {
    xTaskCreate(noConfigLedTask, "noConfigLedTask", 1024, NULL, 1, NULL); // start no config led task
    ESP_LOGI(MAIN_TAG, "No config found. Waiting for config...");
    while (config.verify())
    {
      vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
  }

  // check if VCondo is too low and go to deep sleep
  // the BOOT_PIN is used to prevent deep sleep when the device is plugged to a computer for debug
  if (getVUSB() < 4.5 && getVCondo() < 4.5 && config.values.enableDeepSleep && gpio_get_level(BOOT_PIN))
  {
    ESP_LOGI(MAIN_TAG, "VCondo is too low, going to deep sleep");
    esp_sleep_enable_timer_wakeup(1 * 1000000);
    esp_deep_sleep_start();
  }

  switch (config.values.mode)
  {
  case MODE_WEB:
    // connect to wifi
    if (connectToWifi())
    {
      getTimestamp();               // get timestamp from ntp server
      getConfigFromServer(&config); // get config from server
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      disconectFromWifi();
    }
    break;
  case MODE_MQTT:
  case MODE_MQTT_HA:
    // connect to wifi
    if (connectToWifi())
    {
      getTimestamp(); // get timestamp from ntp server
      vTaskDelay(1000 / portTICK_PERIOD_MS);
      disconectFromWifi();
    }
    break;
  case MODE_ZIGBEE:
    init_zigbee();
    // zigbee_task(0);
    break;
  default:
    break;
  }
  // start linky fetch task

  xTaskCreate(fetchLinkyDataTask, "fetchLinkyDataTask", 16384, NULL, 1, &fetchLinkyDataTaskHandle); // start linky task
  // ESP_LOGI(MAIN_TAG, "FREE HEAP: %ld", esp_get_free_heap_size());
}

void fetchLinkyDataTask(void *pvParameters)
{
  linky.begin();
  while (1)
  {
    if (!linky.update() ||
        !linky.presence())
    {
      ESP_LOGE(MAIN_TAG, "Linky update failed: \n %s", linky.buffer);
      startLedPattern(PATTERN_LINKY_ERR);
      vTaskDelay((config.values.refreshRate * 1000) / portTICK_PERIOD_MS); // wait for refreshRate seconds before next loop
      continue;
    }
    linky.print();
    switch (config.values.mode)
    {
    case MODE_WEB: // send data to web server
      if (dataIndex >= MAX_DATA_INDEX)
      {
        dataIndex = 0;
      }
      dataArray[dataIndex] = linky.data;
      dataArray[dataIndex++].timestamp = getTimestamp();
      ESP_LOGI(MAIN_TAG, "Data stored: %d - BASE: %lld", dataIndex, dataArray[0].timestamp);
      if (dataIndex > 2)
      {
        char json[1024] = {0};
        preapareJsonData(dataArray, dataIndex, json, sizeof(json));
        ESP_LOGI(MAIN_TAG, "Sending data to server");
        if (connectToWifi())
        {
          ESP_LOGI(MAIN_TAG, "POST: %s", json);
          sendToServer(json);
        }
        disconectFromWifi();
        dataIndex = 0;
      }
      break;
    case MODE_MQTT:
    case MODE_MQTT_HA: // send data to mqtt server
    case MODE_TUYA:
      if (connectToWifi())
      {
        ESP_LOGI(MAIN_TAG, "Sending data to MQTT");
        sendToMqtt(&linky.data);
        disconectFromWifi();
        startLedPattern(PATTERN_SEND_OK);
      }
      else
      {
        startLedPattern(PATTERN_SEND_ERR);
      }
      break;
    default:
      break;
    }
    vTaskDelay((abs(config.values.refreshRate - 5) * 1000) / portTICK_PERIOD_MS); // wait for refreshRate seconds before next loop
  }
}