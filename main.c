/*
    Neapolis Innovation - Copyright (C) 2023 Salvatore Bramante

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.
*/

#include "ch.h"
#include "hal.h"

#include <string.h>
#include <math.h>

#include "sampler.h"
#include "oled_manager.h"
#include "encoder_manager.h"
#include "outsetting.h"

/* External LEDs Red, Green and Blue: Port, Pin, Line */
#define RLED_LINE   PAL_LINE( GPIOB, 10U ) // D6
#define GLED_LINE   PAL_LINE( GPIOA, 8U ) // D4
#define BLED_LINE   PAL_LINE( GPIOA, 9U ) // D5

//#define BTN_ON_OFF_LINE PAL_LINE(GPIOC, 13U)
#define BTN_ON_OFF_LINE PAL_LINE(GPIOA, 10U)

#define ARRAY_LEN(a)            (sizeof(a)/sizeof(a[0]))

volatile uint8_t flag_on_off = PAL_LOW;
volatile uint8_t flag_error = PAL_LOW;
volatile uint8_t funcionamiento = PAL_LOW;
/*
 * Shared Variable: time
 */
static uint32_t time;

static void button_on_off_cb(void *arg) {

  (void)arg;

  static systime_t start, stop;
  chSysLockFromISR();
  if( palReadLine( BTN_ON_OFF_LINE ) == PAL_HIGH ) {
    start = chVTGetSystemTimeX();
  } else {
    sysinterval_t delta;
    stop = chVTGetSystemTimeX();
    delta = chTimeDiffX( start, stop );
    time = TIME_I2MS( delta );
    if (time >= 150) flag_on_off = !flag_on_off;
  }
  chSysUnlockFromISR();
}
/*
 * Working Area and Thread declarations.
 */
static THD_WORKING_AREA( waStatusLED, 128 );
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
      if(funcionamiento==PAL_LOW){
        initOut();
        startOut();
        setShape(SIN);
        startADCThread();
        startOledThread();
        startEncoderThread();
        funcionamiento=PAL_HIGH;
      }
    }
    else if((flag_on_off == PAL_LOW)&&(flag_error == PAL_LOW)){
      palClearLine(GLED_LINE);
      palSetLine(RLED_LINE);
      if(funcionamiento==PAL_HIGH){
        stopEncoderThread();
        stopOledThread();
        stopADCThread();
        stopOut();

        funcionamiento=PAL_LOW;
      }
    }
    else {
      if(palReadLine(GLED_LINE)==PAL_HIGH) palClearLine(GLED_LINE);
      palToggleLine(RLED_LINE);
    }
    chThdSleepMilliseconds( 200 );
  }
}

int main(void) {

  halInit();
  chSysInit();

  /* Configurazione I2C */
  palSetLineMode(PAL_LINE(GPIOB, 8U), PAL_MODE_ALTERNATE(4) |
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);
  palSetLineMode(PAL_LINE(GPIOB, 9U), PAL_MODE_ALTERNATE(4) |
                 PAL_STM32_OTYPE_OPENDRAIN | PAL_STM32_OSPEED_HIGHEST |
                 PAL_STM32_PUPDR_PULLUP);

  /* Enabling events on both edges of the button line.*/
  palEnableLineEvent( BTN_ON_OFF_LINE, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback( BTN_ON_OFF_LINE, button_on_off_cb, NULL);
  chThdCreateStatic(waStatusLED, sizeof(waStatusLED), NORMALPRIO + 1, thdStatusLED, NULL );

  palSetLineMode(LINE_LED,PAL_MODE_OUTPUT_PUSHPULL);

  palSetLineMode(BTN_ON_OFF_LINE, PAL_MODE_INPUT_PULLUP);
  palEnableLineEvent(BTN_ON_OFF_LINE, PAL_EVENT_MODE_BOTH_EDGES);
  palSetLineCallback(BTN_ON_OFF_LINE, button_on_off_cb, NULL);

  while (true ) {
    updateScope();
    chThdSleepMilliseconds(200);
  }
}
