#ifndef __OUTSETTING_H__
#define __OUTSETTING_H__

#include <stdint.h>
#include "hal.h"

#define DAC_BUFFER_SIZE 720
#define DAC_MAX 4095

typedef enum {
  BUFFER_UPDATED,
  VALUE_ERROR
} DACBufferUpdateResult;

typedef enum {
  SIN,
  RECT,
  TRI,
  SAW
} OUTShape;

struct dac_buffer {
  dacsample_t* table;
  uint16_t len;
};

typedef struct dac_buffer DACBuffer;

struct out_conf {
  dacsample_t amp;
  uint16_t ns;
  dacsample_t offset;
  float dc;
  OUTShape shape;
};

typedef struct out_conf OUTConf;

void initOut(void);
void startOut(void);
void stopOut(void);


DACBufferUpdateResult setAmplitude(dacsample_t);
DACBufferUpdateResult setPeriod(uint16_t);
DACBufferUpdateResult setOffset(dacsample_t);
DACBufferUpdateResult setDutyCycle(float);

DACBufferUpdateResult setAmplitudeFromPerc(int);
DACBufferUpdateResult setPeriodFromPerc(int);
DACBufferUpdateResult setOffsetFromPerc(int);
DACBufferUpdateResult setDutyCycleFromPerc(int);

DACBufferUpdateResult setShape(OUTShape);

int getAmplitudePerc(void);
int getPeriodPerc(void);
int getOffsetPerc(void);
int getDutyCyclePerc(void);


//DACBuffer* get_buffer(void);


#endif // __OUTSETTING_H__
