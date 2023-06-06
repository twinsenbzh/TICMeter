#ifndef MAIN_H
#define MAIN_H

#include "linky.h"

#define uS_TO_S_FACTOR 1000000

extern TaskHandle_t fetchLinkyDataTaskHandle;

/**
 * @brief Get the tension of the condo
 *
 * @return float
 */
float getVCondo();

/**
 * @brief Get the VUSB
 *
 * @return 1 if VUSB is present, 0 if not
 */
uint8_t getVUSB();

/**
 * @brief Create a Http Url (http://host/path)
 *
 * @param url the destination url
 * @param host the host
 * @param path the path
 */
void createHttpUrl(char *url, const char *host, const char *path);

/**
 * @brief prepare json data to send to server
 *
 * @param data  the Array of data to send
 * @param dataIndex  the index of the data to send
 * @param json  the json destination
 * @param jsonSize the size of the json destination
 */
void preapareJsonData(LinkyData *data, char dataIndex, char *json, unsigned int jsonSize);

/**
 * @brief set the CPU frequency to 10Mhz and disconnect from wifi
 *
 */

void fetchLinkyDataTask(void *pvParameters);

uint8_t sleep(int time);
void sendDataTask(void *pvParameters);
void linkyRead(void *pvParameters);

#endif