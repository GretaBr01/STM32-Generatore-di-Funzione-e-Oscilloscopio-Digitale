#ifndef BTN_ON_OFF_H
#define BTN_ON_OFF_H

#include "ch.h"
#include "hal.h"

/* External LEDs Red, Green and Blue: Port, Pin, Line */
#define RLED_LINE   PAL_LINE( GPIOB, 10U ) // D6
#define GLED_LINE   PAL_LINE( GPIOA, 8U ) // D4
#define BLED_LINE   PAL_LINE( GPIOA, 9U ) // D5

#define BTN_ON_OFF_LINE PAL_LINE(GPIOC, 13U)

void startStatusLEDThread(void);
static void button_on_off_cb(void *arg);

#endif
