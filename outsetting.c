/*
 * functions.c
 *
 *  Created on: Sep 2, 2025
 *      Author: Phoenix
 */


#include <outsetting.h>
#include "ch.h"
#include "hal.h"
#include <stdlib.h>
#include <stdint.h>
#include "math.h"

//#define PI 3.14159265358979323846
#define PORTAB_LINE_LED1            LINE_LED
#define PORTAB_DAC_TRIG             7
#define SCALE_FACTOR                14
/*
 * DAC streaming callback.
 */
size_t n = 0;
static void end_cb1(DACDriver *dacp) {

  (void)dacp;
  n++;
  if ((n % 1000) == 0) {
    palToggleLine(PORTAB_LINE_LED1);
  }

}


/*
 * DAC error callback.
 */
static void error_cb1(DACDriver *dacp, dacerror_t err) {

  (void)dacp;
  (void)err;

  chSysHalt("DAC failure");
}

static const DACConfig dac1cfg1 = {
  .init         = 2047U,
  .datamode     = DAC_DHRM_12BIT_RIGHT,
  .cr           = 0
};

static const DACConversionGroup dacgrpcfg1 = {
  .num_channels = 1U,
  .end_cb       = end_cb1,
  .error_cb     = error_cb1,
  .trigger      = DAC_TRG(PORTAB_DAC_TRIG)
};


/*
 * GPT6 configuration.
 */
static GPTConfig gpt6cfg1 = {
  .frequency    = DAC_BUFFER_SIZE*1000, //1440000 HZ
  .callback     = NULL,
  .cr2          = TIM_CR2_MMS_1,    /* MMS = 010 = TRGO on Update Event.    */
  .dier         = 0U
};

static dacsample_t _table[DAC_BUFFER_SIZE];

static DACBuffer _dac_buffer = {
  .table = _table,
  .len = DAC_BUFFER_SIZE
};

static OUTConf _out_conf = {
  .amp = 0x800,
  .ns = DAC_BUFFER_SIZE,
  .offset = 0x800,
  .dc = 0.5,
  .shape = RECT
};

static char running = 0;

void initOut(void){
  /* Setting up the output pin as analog as suggested
         by the Reference Manual.*/
    palSetPadMode(GPIOA, 4, PAL_MODE_INPUT_ANALOG);
    palSetPadMode(GPIOA, 5, PAL_MODE_INPUT_ANALOG);
    dacStart(&DACD1, &dac1cfg1);
    gptStart(&GPTD6, &gpt6cfg1);
    gptStartContinuous(&GPTD6, 2U);
    running = 1;
}

void startOut(void){
  if (running)
    dacStartConversion(&DACD1, &dacgrpcfg1, _dac_buffer.table, _dac_buffer.len);
}

void stopOut(void){
  if (running)
    dacStopConversion(&DACD1);
}

/*
DACBuffer* get_buffer(void) {
  return &_dac_buffer;
}*/

/**
 * @brief   Initialize DAC buffer with Sine function.
 *
 * @param[in] amp       peak-to-peak amplitude of the  sine wave
 * @param[in] ns        period/number of samples of the wave
 * @param[in] offset    mean value of the sine wave, must be higher than amp/2
 *
 * @api
 */
static DACBufferUpdateResult initSin(void){
  dacsample_t amp = _out_conf.amp >> 1;
  if (_out_conf.ns > DAC_BUFFER_SIZE || _out_conf.offset+amp>0xFFF || _out_conf.offset-amp<0)
    return VALUE_ERROR;

  stopOut();

  for (uint16_t i = 0; i < _out_conf.ns; i++){
    _dac_buffer.table[i] = (dacsample_t)(sinf( i*M_PI*2/_out_conf.ns )*(amp) + _out_conf.offset);
  }
  _out_conf.shape = SIN;

  startOut();
  return BUFFER_UPDATED;
}

/**
 * @brief   Initialize DAC buffer with rectangular wave function.
 *
 * @param[in] amp       peak-to-peak amplitude of the rectangular wave
 * @param[in] ns        period/number of samples of the wave
 * @param[in] offset    mean value of the square wave, must be higher than amp/2
 * @param[in] dc        duty cycle, must be between 0 and 1
 *
 * @api
 */
static DACBufferUpdateResult initRect(void){
  dacsample_t amp = _out_conf.amp >> 1;
  if (_out_conf.ns > DAC_BUFFER_SIZE || _out_conf.offset+amp>0xFFF || _out_conf.offset-amp<0 || _out_conf.dc < 0 || _out_conf.dc> 1)
      return VALUE_ERROR;

  stopOut();

  for (uint16_t i = 0; i < _out_conf.ns; i++){
    _dac_buffer.table[i] = _out_conf.offset +(i < _out_conf.dc*_out_conf.ns ? amp : -amp);
  }
  _out_conf.shape = RECT;

  startOut();
  return BUFFER_UPDATED;
}

/**
 * @brief   Initialize DAC buffer with triangular wave function.
 *
 * @param[in] amp       peak-to-peak amplitude of the triangular wave
 * @param[in] ns        period/number of samples of the wave
 * @param[in] offset    mean value of the square wave, must be higher than amp/2
 *
 * @api
 */
static DACBufferUpdateResult initTri(void){
  dacsample_t offset = _out_conf.offset - (_out_conf.amp>> 1);
  if (_out_conf.ns > DAC_BUFFER_SIZE || offset+_out_conf.amp>0xFFF)
      return VALUE_ERROR;

  stopOut();

  for (uint16_t i = 0; i < _out_conf.ns; i++){
    //dac_buffer[i] = offset + (dacsample_t)((amp<<1)*( i/ns - (dacsample_t)(i/ns + 0.5)  )*(i < ns>>1 ? 1 : -1));
    //dac_buffer[i] = abs((i % ns) - amp) + offset;
    _dac_buffer.table[i] = (2*_out_conf.amp/_out_conf.ns) * ( (_out_conf.ns>>1) - abs(i - (_out_conf.ns>>1)) ) + offset;
  }
  _out_conf.shape = TRI;

  startOut();
  return BUFFER_UPDATED;
}

/**
 * @brief   Initialize DAC buffer with sawtooth wave function.
 *
 * @param[in] amp       peak-to-peak amplitude of the triangular wave
 * @param[in] ns        period/number of samples of the wave
 * @param[in] offset    mean value of the sawtooth wave, must be higher than amp/2
 *
 * @api
 */
static DACBufferUpdateResult initSaw(void){
  dacsample_t offset = _out_conf.offset - (_out_conf.amp>> 1);
  if (_out_conf.ns > DAC_BUFFER_SIZE || offset+_out_conf.amp>0xFFF)
      return VALUE_ERROR;

  stopOut();

  for (uint16_t i = 0; i < _out_conf.ns; i++){
    _dac_buffer.table[i] = _out_conf.amp*i/_out_conf.ns + offset;
  }
  _out_conf.shape = SAW;

  startOut();
  return BUFFER_UPDATED;
}


static DACBufferUpdateResult (*_shape_vector[4])(void) = {&initSin, &initRect, &initTri, &initSaw};


DACBufferUpdateResult setShape(OUTShape new_shape) {
  return _shape_vector[new_shape]();
}

DACBufferUpdateResult setAmplitude(dacsample_t new_amp) {
  dacsample_t now_amp = _out_conf.amp;
  _out_conf.amp = new_amp;
  if ( _shape_vector[_out_conf.shape]() == BUFFER_UPDATED )
    return BUFFER_UPDATED;

  _out_conf.amp = now_amp;
  return VALUE_ERROR;
}

DACBufferUpdateResult setPeriod(uint16_t new_ns){
    uint16_t now_ns = _out_conf.ns;
    _out_conf.ns = new_ns;
    _dac_buffer.len = new_ns;
    if ( _shape_vector[_out_conf.shape]() == BUFFER_UPDATED )
      return BUFFER_UPDATED;

    _out_conf.ns = now_ns;
    _dac_buffer.len = now_ns;
    return VALUE_ERROR;
}

DACBufferUpdateResult setOffset(dacsample_t new_offset){
  uint16_t now_offset = _out_conf.offset;
  _out_conf.offset = new_offset;
  if ( _shape_vector[_out_conf.shape]() == BUFFER_UPDATED )
    return BUFFER_UPDATED;

  _out_conf.offset = now_offset;
  return VALUE_ERROR;
}

DACBufferUpdateResult setDutyCycle(float new_dc){
  float now_dc = _out_conf.dc;
  _out_conf.dc = new_dc;
  if ( _shape_vector[_out_conf.shape]() == BUFFER_UPDATED )
    return BUFFER_UPDATED;

  _out_conf.offset = now_dc;
  return VALUE_ERROR;
}

DACBufferUpdateResult setAmplitudeFromPerc(int perc){
  if ( perc<0 || perc>100 )
    return VALUE_ERROR;
  return setAmplitude( (dacsample_t)(0xFFF * perc * 0.01));
}

DACBufferUpdateResult setPeriodFromPerc(int perc){
  if ( perc<0 || perc>100 )
    return VALUE_ERROR;
  if (!perc){
    _dac_buffer.len = 1;
    _out_conf.ns = 1;
    return initSin();
  }
  return setPeriod( (uint16_t)(SCALE_FACTOR* perc));
}

DACBufferUpdateResult setOffsetFromPerc(int perc){
  if ( perc<0 || perc>100 )
    return VALUE_ERROR;
  return setOffset( (dacsample_t)(0xFFF * perc * 0.01));
}

DACBufferUpdateResult setDutyCycleFromPerc(int perc){
  if ( perc<0 || perc>100 )
    return VALUE_ERROR;
  return setOffset( perc * 0.01);
}


int getAmplitudePerc(void) {
  return (int)(_out_conf.amp *100 / 4095);
}

int getPeriodPerc(void) {
  return (int)(_out_conf.ns/SCALE_FACTOR);
}

int getOffsetPerc(void) {
  return (int)(_out_conf.offset *100 / 4095);
}

int getDutyCyclePerc(void){
  return (int)(_out_conf.dc*100);
}





