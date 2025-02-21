#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "freertos/FreeRTOS.h"
#include "wifi_manager.h"

void app_main(void)
{
    wifi_manager_init();
    
    wifi_manager_connect();
    
    vTaskDelay(5000 / portTICK_PERIOD_MS);
    
    wifi_manager_reset_prov();
    wifi_manager_connect();
    
    for (;;) {
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}
