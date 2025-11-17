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

  initOut();
  startOut();
  setShape(SIN);


  startADCThread();
  startOledThread();
  startEncoderThread();

  while (true ) {
    updateScope();
    chThdSleepMilliseconds(200);
  }
}
