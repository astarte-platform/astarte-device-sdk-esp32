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
