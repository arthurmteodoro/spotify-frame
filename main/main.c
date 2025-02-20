#include <stdio.h>
#include <stdbool.h>
#include <unistd.h>

#include "wifi_manager.h"

void app_main(void)
{
    wifi_manager_init();
    
    wifi_manager_connect();
}
