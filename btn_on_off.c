#include "btn_on_off.h"

static THD_WORKING_AREA( waStatusLED, 128 );

#define ARRAY_LEN(a)            (sizeof(a)/sizeof(a[0]))

volatile uint8_t flag_on_off = PAL_LOW;
volatile uint8_t flag_error = PAL_LOW;
/*
 * Shared Variable: time
 */
static uint32_t time;

static THD_FUNCTION( thdStatusLED, arg ) {
  (void) arg;

  uint32_t i;
  ioline_t leds[] = { RLED_LINE, GLED_LINE, BLED_LINE };

  /* Setup Leds Outputs */
  for( i = 0; i < ARRAY_LEN(leds); i++ ) {
    palSetLineMode( leds[i], PAL_MODE_OUTPUT_PUSHPULL );
  }

  while( 1 ) {
    if((flag_on_off == PAL_HIGH)&&(flag_error == PAL_LOW)) {
      palClearLine(RLED_LINE);
      palSetLine(GLED_LINE);
    }
    else if((flag_on_off == PAL_LOW)&&(flag_error == PAL_LOW)){
      palClearLine(GLED_LINE);
      palSetLine(RLED_LINE);
    }
    else {
      if(palReadLine(GLED_LINE)==PAL_HIGH) palClearLine(GLED_LINE);
      palToggleLine(RLED_LINE);
    }
    chThdSleepMilliseconds( 200 );
  }
}

void startStatusLEDThread(void) {
  /* Enabling events on both edges of the button line.*/
  palEnableLineEvent( BTN_ON_OFF_LINE, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback( BTN_ON_OFF_LINE, button_on_off_cb, NULL);
  chThdCreateStatic(waStatusLED, sizeof(waStatusLED), NORMALPRIO + 1, thdStatusLED, NULL );
}


