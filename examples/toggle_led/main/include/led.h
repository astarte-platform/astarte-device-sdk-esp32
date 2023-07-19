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

#ifndef LED_H
#define LED_H

#include <stdint.h>

/**
 * @brief Initializes the LEDs used in the example.
 */
void led_init();

/**
 * @brief Change the LED status (ON/OFF).
 *
 * N.B. the correlation between status (ON/OFF) and level (1/0) depends on the hardware
 * configuration of the used board.
 */
void led_set_level(uint32_t level);

#endif /* LED_H */
