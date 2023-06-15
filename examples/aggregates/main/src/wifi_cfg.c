#include "wifi_cfg.h"

#include <esp_idf_version.h>
#include <esp_log.h>
#include <esp_wifi.h>

#include "freertos/event_groups.h"
#include "freertos/task.h"

/************************************************
 * Constants/Defines
 ***********************************************/

#define TAG "ASTARTE_EXAMPLE_WIFI_CFG"

static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief Wifi event handler for the ESP32 wifi.
 *
 * @details To be installed in an event loop.
 * @param arg Remaining handler arguments.
 * @param event_base event base.
 * @param event_id event identifier.
 * @param event_data event data.
 * @return The status code, ASTARTE_OK if successful, otherwise an error code is returned.
 */
static void wifi_event_handler(
    void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

/************************************************
 * Global functions definition
 ***********************************************/

void wifi_init(void)
{
    wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
    ESP_LOGI(TAG, "Waiting for wifi");
    xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
}

/************************************************
 * Static functions definitions
 ***********************************************/

static void wifi_event_handler(
    void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
    }
}
