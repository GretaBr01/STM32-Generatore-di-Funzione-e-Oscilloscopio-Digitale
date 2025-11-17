#ifndef OLED_MANAGER_H
#define OLED_MANAGER_H

#include "ch.h"
#include "hal.h"
#include "ssd1306.h"
#include "chprintf.h"

#define DISPLAY_WIDTH   SSD1306_WIDTH
#define DISPLAY_HEIGHT  SSD1306_HEIGHT

typedef enum {
    OLED_MODE_MENU,
    OLED_MODE_SCOPE
} oled_mode_t;

void startOledThread(void);
void stopOledThread(void);
oled_mode_t oledGetMode(void);
void oledUpdateScope(void);

#endif
