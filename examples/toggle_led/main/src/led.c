#include "led.h"

#include <driver/gpio.h>
#include <esp_idf_version.h>
#include <esp_log.h>

/************************************************
 * Constants/Defines
 ***********************************************/

#define TAG "ASTARTE_EXAMPLE_LED"

/************************************************
 * Global functions definition
 ***********************************************/

void led_init()
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // disable interrupt
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // set as output mode
    io_conf.mode = GPIO_MODE_OUTPUT;
    // bit mask of the pin to set
    io_conf.pin_bit_mask = (1ULL << (uint8_t) CONFIG_LED_GPIO);
    // disable pull-down mode
    io_conf.pull_down_en = 0;
    // disable pull-up mode
    io_conf.pull_up_en = 0;
    // configure GPIO with the given settings
    gpio_config(&io_conf);
}

void led_set_level(uint32_t level)
{
    ESP_LOGI(TAG, "Changing LED status.");
    gpio_set_level(CONFIG_LED_GPIO, level);
}
