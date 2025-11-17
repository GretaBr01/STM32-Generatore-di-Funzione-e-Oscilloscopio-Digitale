#include <sampler.h>
#include <outsetting.h>

/* --- ADC conversion group su PA0 --- */
static const ADCConversionGroup adcgrpcfg1 = {
          .circular     = false,
          .num_channels = ADC_GRP_NUM_CHANNELS,
          .end_cb       = NULL,
          .error_cb     = NULL,
          .cfgr         = ADC_CFGR_CONT,
          .cfgr2        = 0U,
          .tr1          = ADC_TR_DISABLED,
          .tr2          = ADC_TR_DISABLED,
          .tr3          = ADC_TR_DISABLED,
          .awd2cr       = 0U,
          .awd3cr       = 0U,
          .smpr         = {
            ADC_SMPR1_SMP_AN1(ADC_SMPR_SMP_247P5),
            0U
          },
          .sqr          = {
            ADC_SQR1_SQ1_N(ADC_CHANNEL_IN1),
            0U,
            0U,
            0U
          }
        };

static adcsample_t  samples[ADC_GRP_NUM_CHANNELS * ADC_GRP_BUF_DEPTH];
static thread_t *adcThd = NULL;

static ADCBuffer _adc_buffer = {
  .buffer = samples,
  .len = ADC_GRP_BUF_DEPTH,
  .max_dac = DAC_MAX,
  .trigger = 0
};

THD_WORKING_AREA(waADC, 256);
THD_FUNCTION(thdADC, arg) {
  (void)arg;
  chRegSetThreadName("ADC");

  palSetLineMode(ADC_INPUT1, PAL_MODE_INPUT_ANALOG);
  adcStart(&ADCD1, NULL);

  while (!chThdShouldTerminateX()) {
    adcConvert(&ADCD1, &adcgrpcfg1, samples, ADC_GRP_BUF_DEPTH);
    chThdSleepMilliseconds(50);
  }

  adcStop(&ADCD1);
}

void startADCThread(void) {
    if (adcThd == NULL) {
        adcThd = chThdCreateStatic(waADC, sizeof(waADC), NORMALPRIO, thdADC, NULL);
    }
}

void stopADCThread(void) {
    if (adcThd != NULL) {
        chThdTerminate(adcThd);
        chThdWait(adcThd);
        adcThd = NULL;
    }
}

adcsample_t* getADCSamples(void) {
    return samples;
}

size_t getADCSamplesLength(void) {
    return sizeof(samples) / sizeof(samples[0]);
}


ADCBuffer* updateScope(void) {
    int trigger_index;
    int16_t threshold;
    uint16_t value;

    trigger_index = -1;

    threshold = DAC_MAX / 2;

    for (int i = 1; i < ADC_GRP_BUF_DEPTH; i++) {
      int16_t prev = samples[i-1];
      int16_t cur  = samples[i];

      if (prev < threshold && cur >= threshold) {
        trigger_index = i;
        break;
      }
    }

    _adc_buffer.trigger = (trigger_index) % ADC_GRP_BUF_DEPTH;

  return &_adc_buffer;
}
