#ifndef BUTTON_H
#define BUTTON_H

#include <stdbool.h>

/**
 * @brief Initialize the GPIO connected to the button.
 */
void button_gpio_init(void);

/**
 * @brief Poll a push of the button.
 */
bool poll_button_event(void);

#endif /* BUTTON_H */
