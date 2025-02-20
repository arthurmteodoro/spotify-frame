#ifndef INCLUDE_WIFI_MANAGER_H_
#define INCLUDE_WIFI_MANAGER_H_


#include <esp_wifi.h> // Wifi
#include <esp_check.h> // Error Handling
#include <nvs_flash.h> // NVS flash
#include <esp_netif.h> // TCP/IP Task
#include <esp_err.h> // Error Helper
#include "freertos/FreeRTOS.h" // Main FreeRTOS
#include "freertos/task.h" // FreeRTOS task
#include "freertos/event_groups.h" // Event Group
#include <wifi_provisioning/manager.h>
#include <wifi_provisioning/scheme_softap.h>

esp_err_t wifi_manager_init();
esp_err_t wifi_manager_connect();

#endif /* INCLUDE_WIFI_MANAGER_H_ */
