#include "button.h"

#include <driver/gpio.h>
#include <esp_idf_version.h>

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

/************************************************
 * Constants/Defines
 ***********************************************/

#define ESP_INTR_FLAG_DEFAULT 0

/************************************************
 * Static variables declaration
 ***********************************************/

static xQueueHandle button_evt_queue;

/************************************************
 * Static functions declaration
 ***********************************************/

/**
 * @brief ISR handler for the button GPIO.
 *
 * @param arg ISR handler arguments.
 */
static void IRAM_ATTR button_isr_handler(void *arg);

/************************************************
 * Global functions definition
 ***********************************************/

void button_gpio_init(void)
{
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5, 0, 0)
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // bit mask of the pin
    io_conf.pin_bit_mask = (1ULL << CONFIG_BUTTON_GPIO);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);
#else
    // Set the pad as GPIO
    gpio_pad_select_gpio(CONFIG_BUTTON_GPIO);
    // Set button GPIO as input
    gpio_set_direction(CONFIG_BUTTON_GPIO, GPIO_MODE_INPUT);
    // Enable pullup
    gpio_pullup_en(CONFIG_BUTTON_GPIO);
    // Trigger interrupt on negative edge, i.e. on release
    gpio_set_intr_type(CONFIG_BUTTON_GPIO, GPIO_INTR_NEGEDGE);
#endif

    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook ISR handler for specific gpio pin
    gpio_isr_handler_add(CONFIG_BUTTON_GPIO, button_isr_handler, (void *) CONFIG_BUTTON_GPIO);
}

bool poll_button_event(void)
{
    uint32_t io_num;
    return (
        xQueueReceive(button_evt_queue, &io_num, portMAX_DELAY) && (io_num == CONFIG_BUTTON_GPIO));
}

/************************************************
 * Static functions definitions
 ***********************************************/

static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}
