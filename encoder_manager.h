#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include "ch.h"
#include "hal.h"

#define Enc_SW PAL_LINE(GPIOB, 4U)
#define Enc_A  PAL_LINE(GPIOB, 5U)
#define Enc_B  PAL_LINE(GPIOB, 3U)


void startEncoderThread(void);
void stopEncoderThread(void);

int getEncoderCount(void);
void resetEncoder(void);
bool isEncoderButtonPressed(void);
int getEncMaxCount(void);
void setEncMaxCount(int);
void setEncCount(int val);

#endif
