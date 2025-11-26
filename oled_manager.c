/*
    oled_manager.c - Version Robusta Anti-Ruido EMI
*/

#include "oled_manager.h"
#include "ssd1306.h"
#include "chprintf.h"
#include "encoder_manager.h"
#include "sampler.h"
#include "outsetting.h"

/* --- Variables Estáticas y Definiciones --- */
static thread_t *oledThd = NULL;
static THD_WORKING_AREA(waOledDisplay, 1024); // Aumentado ligeramente por seguridad
static SSD1306Driver SSD1306D1;
static oled_mode_t currentMode = OLED_MODE_MENU;

/* Variables para el Osciloscopio */
static const uint16_t scope_height = DISPLAY_HEIGHT-15;
static const adcsample_t *scopeSamples = NULL;
static size_t scopeLength = 0;
static size_t scopeStartIndex = 0;
static int16_t scopeMaxValue;

#define BUFF_SIZE   20

/* Estructuras del Menú */
typedef struct {
  char title[BUFF_SIZE];
  char line1[BUFF_SIZE];
  char line2[BUFF_SIZE];
  char line3[BUFF_SIZE];
  char line4[BUFF_SIZE];
} Page;

static Page Menu[3] = {
    {"> Menu", " -> Sinusoidal"," -> Rectangular", " -> Triangular", " -> Oscilloscope"},
    {"< Return", " -> Amplitude pp", " -> Offset", " -> Period", " -> Duty Cycle"},
    {"< OK", " -> FS:", "", " -> value:", ""}
};

/* --- CONFIGURACIÓN I2C ROBUSTA (Standard Mode ~100kHz) --- */
/* Esta configuración es más lenta pero mucho más resistente al ruido que la original.
   Los valores asumen un reloj APB1 estándar. Si usas un reloj muy exótico,
   quizás requiera ajuste fino, pero esto suele funcionar en F3/F4/L4. */
static const I2CConfig i2c_robust_cfg = {
    STM32_TIMINGR_PRESC(15U) |
    STM32_TIMINGR_SCLDEL(4U) | STM32_TIMINGR_SDADEL(2U) |
    STM32_TIMINGR_SCLH(15U)  | STM32_TIMINGR_SCLL(19U),
    0,
    0
};

static const SSD1306Config ssd1306_cfg = {
    &I2CD1,
    &i2c_robust_cfg,
    SSD1306_SAD_0X78
};

/* --- Función Auxiliar para Imprimir Menú --- */
static void printMenu(Page myPage, int SelectLine) {
  //Ciclo per stampare sul display il menu
  ssd1306FillScreen(&SSD1306D1, 0x00); // annerisci schermo
  //TITOLO
  ssd1306GotoXy(&SSD1306D1, 0, 0); //imposta cursore virtuale
  ssd1306Puts(&SSD1306D1, myPage.title, &ssd1306_font_7x10, SelectLine != 0);
  //LINEA 1
  ssd1306GotoXy(&SSD1306D1, 0, 16);
  ssd1306Puts(&SSD1306D1, myPage.line1, &ssd1306_font_7x10, SelectLine != 1);
  //LINEA 2
  ssd1306GotoXy(&SSD1306D1, 0, 27);
  ssd1306Puts(&SSD1306D1, myPage.line2, &ssd1306_font_7x10, SelectLine != 2);
  //LINEA 3
  ssd1306GotoXy(&SSD1306D1, 0, 38);
  ssd1306Puts(&SSD1306D1, myPage.line3, &ssd1306_font_7x10, SelectLine != 3);
  //LINEA 4
  ssd1306GotoXy(&SSD1306D1, 0, 49);
  ssd1306Puts(&SSD1306D1, myPage.line4, &ssd1306_font_7x10, SelectLine != 4);
}

/* --- Thread OLED Principal --- */
static THD_FUNCTION(OledDisplay, arg) {
    (void)arg;
    chRegSetThreadName("OledDisplay");

    /* Inicialización del Driver */
    ssd1306ObjectInit(&SSD1306D1);
    ssd1306Start(&SSD1306D1, &ssd1306_cfg);
    ssd1306FillScreen(&SSD1306D1, 0x00);

    static Page myPage;
    int SelectLine = 0;
    int SelectFunc = 0;
    char FSstring[BUFF_SIZE];
    static int depthMenu = 0;

    while (!chThdShouldTerminateX()) {

        /* --- LÓGICA DE DIBUJO EN MEMORIA (NO AFECTADA POR RUIDO) --- */
        switch (currentMode) {
        case OLED_MODE_MENU:
          if (depthMenu == 2) {
            chsnprintf(Menu[depthMenu].line1, BUFF_SIZE, FSstring);
            chsnprintf(Menu[depthMenu].line3, BUFF_SIZE, "->value: %d[ppc]",getEncoderCount());
          } else {
            setEncMaxCount(4);
            SelectLine = getEncoderCount();
          }
          myPage = Menu[depthMenu];
          printMenu(myPage, SelectLine);

          if(isEncoderButtonPressed()){
            ssd1306GotoXy(&SSD1306D1, 0, (SelectLine == 0 ? 0 : SelectLine * 11 + 5));
            switch (depthMenu) {
                  case 0:
                    if (SelectLine==4){
                      currentMode = OLED_MODE_SCOPE;
                    }else if(SelectLine !=0){
                      setShape((OUTShape)(SelectLine-1));
                    }
                    break;
                  case 1:
                    SelectFunc = SelectLine;
                    switch (SelectLine) {
                    case 1: chsnprintf(FSstring, BUFF_SIZE, " FS: 3.3 [v]");        break;
                    case 2: chsnprintf(FSstring, BUFF_SIZE, " FS: 3.3 [v]");        break;
                    case 3: chsnprintf(FSstring, BUFF_SIZE, " FS: %d [ns]", 1);     break;
                    case 4: chsnprintf(FSstring, BUFF_SIZE, " FS: %d [ppc]", 100);  break;
                    }
                    break;
                  case 2:
                    switch (SelectFunc) {
                    case 1: setAmplitudeFromPerc(getEncoderCount());    break;
                    case 2: setOffsetFromPerc(getEncoderCount());       break;
                    case 3: setPeriodFromPerc(getEncoderCount());       break;
                    case 4: setDutyCycleFromPerc(getEncoderCount());    break;
                    }
                    break;
                  }
            if (SelectLine == 0) {
              depthMenu <= 0 ? depthMenu = 0 : depthMenu--;
              ssd1306Puts(&SSD1306D1, "x", &ssd1306_font_7x10, SSD1306_COLOR_BLACK);
            } else {
              depthMenu >= 2 ? depthMenu = 2 : depthMenu++;
              ssd1306Puts(&SSD1306D1, " -x", &ssd1306_font_7x10, SSD1306_COLOR_BLACK);
              if(depthMenu==2){
                setEncMaxCount(100);
                switch (SelectFunc) {
                  case 1: setEncCount(getAmplitudePerc());    break;
                  case 2: setEncCount(getOffsetPerc());       break;
                  case 3: setEncCount(getPeriodPerc());       break;
                  case 4: setEncCount(getDutyCyclePerc());    break;
                }
              }
            }
            SelectLine = 0;
          }
          break;
        case OLED_MODE_SCOPE:
          oledUpdateScope();
          ssd1306FillScreen(&SSD1306D1, 0x00);
          ssd1306GotoXy(&SSD1306D1, 0, 0);
          ssd1306Puts(&SSD1306D1, "< Return", &ssd1306_font_7x10, 0);
          if (scopeSamples != NULL) {
            int l = scopeLength < DISPLAY_WIDTH? scopeLength: DISPLAY_WIDTH;
            size_t idx0 = (scopeStartIndex) % scopeLength;
            int16_t centered0 = scopeSamples[idx0];
            int prev_y = DISPLAY_HEIGHT - ((centered0 * scope_height)  / scopeMaxValue);
            int prev_x = 0;
            for (int x = 1; x < l; x++) {
              size_t idx = (scopeStartIndex + x) % scopeLength;
              uint16_t samp = (uint16_t)scopeSamples[idx];
              int16_t centered = (int16_t)samp;
              int y =  DISPLAY_HEIGHT - ((centered * scope_height) / scopeMaxValue);
              ssd1306DrawLine(&SSD1306D1, prev_x, prev_y, x, y, SSD1306_COLOR_WHITE);
              prev_x = x;
              prev_y = y;
            }
          }
          if(isEncoderButtonPressed()){
            depthMenu <= 0 ? depthMenu = 0 : depthMenu--;
            ssd1306Puts(&SSD1306D1, "x", &ssd1306_font_7x10, SSD1306_COLOR_BLACK);
            currentMode = OLED_MODE_MENU;
          }
          break;
        }

        /* --- TRANSMISIÓN CRÍTICA Y RECUPERACIÓN DE ERRORES --- */

        ssd1306UpdateScreen(&SSD1306D1);

        /* Chequeo de Salud del Bus I2C */
        if (i2cGetErrors(&I2CD1) != I2C_NO_ERROR) {
            /* ¡Detectado Ruido o Fallo en I2C! Iniciando protocolo de recuperación */

            // 1. Detener drivers
            ssd1306Stop(&SSD1306D1);
            i2cStop(&I2CD1);

            // 2. Esperar (Debounce del bus)
            chThdSleepMilliseconds(20);

            // 3. Reiniciar con configuración segura
            ssd1306Start(&SSD1306D1, &ssd1306_cfg);

            // La pantalla se actualizará correctamente en el siguiente ciclo while
        }

        // Aumentado a 100ms para reducir tráfico I2C y bajar probabilidad de error por ruido
        chThdSleepMilliseconds(100);
    }

    // Limpieza final
    ssd1306FillScreen(&SSD1306D1, 0x00);
    ssd1306UpdateScreen(&SSD1306D1);
}

/* --- Funciones Públicas --- */
void startOledThread(void) {
    if (oledThd == NULL) {
        oledThd = chThdCreateStatic(waOledDisplay, sizeof(waOledDisplay),
                                    NORMALPRIO, OledDisplay, NULL);
    }
}

void stopOledThread(void) {
    if (oledThd != NULL) {
        chThdTerminate(oledThd);
        chThdWait(oledThd);
        oledThd = NULL;
    }
}

void oledSetMode(oled_mode_t mode) {
    currentMode = mode;
}

oled_mode_t oledGetMode(void) {
    return currentMode;
}

void oledUpdateScope() {
    currentMode = OLED_MODE_SCOPE;
    ADCBuffer* scope = updateScope();
    scopeSamples     = scope->buffer;
    scopeLength      = scope->len;
    scopeStartIndex  = scope->trigger;
    scopeMaxValue    = scope->max_dac;
}
