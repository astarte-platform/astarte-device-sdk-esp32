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
