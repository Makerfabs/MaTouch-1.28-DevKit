#ifndef LOCAL_STORE_H
#define LOCAL_STORE_H

// #include <freertos/FreeRTOS.h>
// #include <freertos/task.h>
#include <Arduino.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <nvs.h>

#define DEBUG_PRINTF Serial.printf

#define NVS_NAMESPACE "MyConfig"
#define NVS_DATA_LENGTH 80

#define SUCCESS 1
#define ERROR 0

// NVS基础读写
void write_nvs(const char *key, const char *value);
int read_nvs(const char *key, char *value);

#endif