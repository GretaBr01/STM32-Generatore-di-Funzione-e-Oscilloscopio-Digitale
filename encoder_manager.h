#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include "ch.h"
#include "hal.h"

#define Enc_SW PAL_LINE(GPIOB, 4U)  //D5
#define Enc_A  PAL_LINE(GPIOB, 5U)  //D4
#define Enc_B  PAL_LINE(GPIOB, 3U)  //D3


void startEncoderThread(void);
void stopEncoderThread(void);

int getEncoderCount(void);
void resetEncoder(void);
bool isEncoderButtonPressed(void);
int getEncMaxCount(void);
void setEncMaxCount(int);
void setEncCount(int val);

#endif
