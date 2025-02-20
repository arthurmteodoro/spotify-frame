#include "wifi_manager.h"
#include "esp_err.h"
#include "esp_wifi_default.h"
#include "wifi_provisioning/manager.h"
#include "wifi_provisioning/scheme_softap.h"
#include "wifi_provisioning/wifi_config.h"

const char* TAG = "wifi_manager";

/*
The event group allow multiples values to be used. In this case, only 2 values will be used:
BIT0 - Wi-Fi successfully connected
BIT1 - Wi-Fi not connected
*/
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAILED_BIT BIT1

/* FreeRTOS event group to signal when we are connected*/
EventGroupHandle_t wifi_event_group;

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    static int retries;
    if (event_base == WIFI_PROV_EVENT) {
        switch (event_id) {
            case WIFI_PROV_START:
                ESP_LOGI(TAG, "Provisioning started");
                break;
            case WIFI_PROV_CRED_RECV: {
                wifi_sta_config_t *wifi_sta_cfg = (wifi_sta_config_t *)event_data;
                ESP_LOGI(TAG, "Received Wi-Fi credentials"
                         "\n\tSSID     : %s\n\tPassword : %s",
                         (const char *) wifi_sta_cfg->ssid,
                         (const char *) wifi_sta_cfg->password);
                break;
            }
            case WIFI_PROV_CRED_FAIL: {
                wifi_prov_sta_fail_reason_t *reason = (wifi_prov_sta_fail_reason_t *)event_data;
                ESP_LOGE(TAG, "Provisioning failed!\n\tReason : %s"
                         "\n\tPlease reset to factory and retry provisioning",
                         (*reason == WIFI_PROV_STA_AUTH_ERROR) ?
                         "Wi-Fi station authentication failed" : "Wi-Fi access-point not found");
                retries++;
                /*if (retries >= 5) {
                    ESP_LOGI(TAG, "Failed to connect with provisioned AP, resetting provisioned credentials");
                    wifi_prov_mgr_reset_provisioning();
                    wifi_prov_mgr_reset_sm_state_on_failure();
                    retries = 0;
                }*/
                if (*reason == WIFI_PROV_STA_AUTH_ERROR) {
					wifi_prov_mgr_reset_sm_state_on_failure();
                    //wifi_manager_connect();
				}
                break;
            }
            case WIFI_PROV_CRED_SUCCESS:
                ESP_LOGI(TAG, "Provisioning successful");
                retries = 0;
                break;
            case WIFI_PROV_END:
                /* De-initialize manager once provisioning is finished */
                wifi_prov_mgr_deinit();
                break;
            default:
                break;
        }
    } else if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                esp_wifi_connect();
                break;
            case WIFI_EVENT_STA_DISCONNECTED:
                ESP_LOGI(TAG, "Disconnected. Connecting to the AP again...");
                esp_wifi_connect();
                break;
            case WIFI_EVENT_AP_STACONNECTED:
                ESP_LOGI(TAG, "SoftAP transport: Connected!");
                break;
            case WIFI_EVENT_AP_STADISCONNECTED:
                ESP_LOGI(TAG, "SoftAP transport: Disconnected!");
                break;
            default:
                break;
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Connected with IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
        /* Signal main application to continue execution */
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

static void get_device_service_name(char *service_name, size_t max)
{
    uint8_t eth_mac[6];
    const char *ssid_prefix = "PROV_";
    esp_wifi_get_mac(WIFI_IF_STA, eth_mac);
    snprintf(service_name, max, "%s%02X%02X%02X",
             ssid_prefix, eth_mac[3], eth_mac[4], eth_mac[5]);
}

esp_err_t wifi_manager_init() {
	
	// initialize the NVS
    esp_err_t nvs_err = nvs_flash_init();
    // If the NVS don't have free pages, or a new version is found, clear the NVS and initialize again
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Erase the NVS flag
        ESP_RETURN_ON_ERROR(nvs_flash_erase(), TAG, "NVS Flash erase failed");

        // Initialize the NVS flag again
        ESP_RETURN_ON_ERROR(nvs_flash_init(), TAG, "NVS Flash initialization failed");
    }
    
    // Initialize the TCP/IP stack
    ESP_ERROR_CHECK(esp_netif_init());

    // Create the default event handler
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_PROV_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));
    
    esp_netif_create_default_wifi_sta();
    esp_netif_create_default_wifi_ap();
    
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    
     wifi_prov_mgr_config_t config = {
		 .scheme = wifi_prov_scheme_softap,
		 .scheme_event_handler = WIFI_PROV_EVENT_HANDLER_NONE
	 };
	 
	 ESP_ERROR_CHECK(wifi_prov_mgr_init(config));
	
	return ESP_OK;
}

esp_err_t wifi_manager_connect() {
	bool provisioned = false;
	
	ESP_ERROR_CHECK(wifi_prov_mgr_is_provisioned(&provisioned));
	
	if (!provisioned) {
		char service_name[12];
        get_device_service_name(service_name, sizeof(service_name));
        
        ESP_ERROR_CHECK(wifi_prov_mgr_start_provisioning(WIFI_PROV_SECURITY_0, NULL, service_name, NULL));
	} else {
		wifi_prov_mgr_deinit();
		ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    	ESP_ERROR_CHECK(esp_wifi_start());
	}
	
	xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, true, true, portMAX_DELAY);
	
	return ESP_OK;
}



