/**
 * This file is part of Astarte.
 *
 * Copyright 2018-2023 SECO Mind Srl
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later OR Apache-2.0
 *
 **/

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
 * ISR handlers
 ***********************************************/

/**
 * @brief ISR handler for the button GPIO.
 *
 * @param arg ISR handler arguments.
 */
static void IRAM_ATTR button_isr_handler(void *arg)
{
    uint32_t gpio_num = (uint32_t) arg;
    xQueueSendFromISR(button_evt_queue, &gpio_num, NULL);
}

/************************************************
 * Global functions definition
 ***********************************************/

void button_gpio_init(void)
{
    // zero-initialize the config structure.
    gpio_config_t io_conf = {};
    // interrupt of rising edge
    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    // bit mask of the pin
    io_conf.pin_bit_mask = (1ULL << (uint8_t) CONFIG_BUTTON_GPIO);
    // set as input mode
    io_conf.mode = GPIO_MODE_INPUT;
    // enable pull-up mode
    io_conf.pull_up_en = 1;
    gpio_config(&io_conf);

    button_evt_queue = xQueueCreate(10, sizeof(uint32_t));

    // Install gpio isr service
    gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT);
    // Hook ISR handler for specific gpio pin
    gpio_isr_handler_add(CONFIG_BUTTON_GPIO, button_isr_handler, (void *) CONFIG_BUTTON_GPIO);
}

bool poll_button_event(void)
{
    uint32_t io_num = 0U;
    return (
        xQueueReceive(button_evt_queue, &io_num, portMAX_DELAY) && (io_num == CONFIG_BUTTON_GPIO));
}
