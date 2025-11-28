#ifndef __SEMPLER_H__
#define __SEMPLER_H__

#include "ch.h"
#include "hal.h"

/* --- ADC buffer --- */
#define ADC_INPUT1  PAL_LINE(GPIOA, 0u)
#define VOLTAGE_RES            ((float)3.3/4096)
#define ADC_GRP_NUM_CHANNELS   1
#define ADC_GRP_BUF_DEPTH      640
#define ADC_CENTER             2048

extern adcsample_t adc_samples[ADC_GRP_NUM_CHANNELS * ADC_GRP_BUF_DEPTH];

struct scope_buffer {
  adcsample_t* buffer;
  uint16_t len;
  uint16_t max_dac;
  uint16_t trigger;
};

typedef struct scope_buffer ADCBuffer;

extern THD_WORKING_AREA(waADC, 256);
THD_FUNCTION(thdADC, arg);

adcsample_t* getADCSamples(void);
size_t getADCSamplesLength(void);
void startADCThread(void);
void stopADCThread(void);
ADCBuffer* updateScope(void);

#endif
