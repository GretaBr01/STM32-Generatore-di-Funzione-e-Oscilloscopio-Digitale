#include "oled_manager.h"
#include "ssd1306.h"
#include "chprintf.h"
#include "encoder_manager.h"
#include "sampler.h"
#include "outsetting.h"

static thread_t *oledThd = NULL;
static THD_WORKING_AREA(waOledDisplay, 512);
static SSD1306Driver SSD1306D1;

static oled_mode_t currentMode = OLED_MODE_MENU;
static const uint16_t scope_height = DISPLAY_HEIGHT-15;

static const adcsample_t *scopeSamples = NULL;
static size_t scopeLength = 0;
static size_t scopeStartIndex = 0;

static int16_t scopeMaxValue;

#define BUFF_SIZE   20

typedef struct {
  char title[BUFF_SIZE];
  char line1[BUFF_SIZE];
  char line2[BUFF_SIZE];
  char line3[BUFF_SIZE];
  char line4[BUFF_SIZE];
} Page;

static Page Menu[3] = { {"> Menu", " -> Sinusoidal"," -> Rectangular",
                        " -> Triangular", " -> Oscilloscope"},
                       {"< Return", " -> Amplitude pp", " -> Offset",
                        " -> Period", " -> Duty Cycle"},
                       {"< OK", " -> FS:", "", " -> value:", ""}};


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
} // end PrintMenu

/* --- Thread OLED --- */
static THD_FUNCTION(OledDisplay, arg) {
    (void)arg;
    chRegSetThreadName("OledDisplay");

    /* Init display */
    ssd1306ObjectInit(&SSD1306D1);
    ssd1306Start(&SSD1306D1, &(SSD1306Config){
        &I2CD1,
        &(I2CConfig){
            STM32_TIMINGR_PRESC(8U) |
            STM32_TIMINGR_SCLDEL(3U) | STM32_TIMINGR_SDADEL(3U) |
            STM32_TIMINGR_SCLH(3U) | STM32_TIMINGR_SCLL(9U),
            0, 0},
        SSD1306_SAD_0X78
    });

    ssd1306FillScreen(&SSD1306D1, 0x00);

    static Page myPage; // pagina corrente del menu
    int SelectLine = 0; // linea della pagina selezzionata
    int SelectFunc = 0; // selezione della funzione da eseguire
    char FSstring[BUFF_SIZE]; // stringa particolare
    static int depthMenu = 0; //pagina corrente del menu

    while (!chThdShouldTerminateX()) {
        switch (currentMode) {
        case OLED_MODE_MENU:
          if (depthMenu == 2) { // ceck profondità menu
            // personalizzo la pagina del menu
            chsnprintf(Menu[depthMenu].line1, BUFF_SIZE, FSstring);
            chsnprintf(Menu[depthMenu].line3, BUFF_SIZE, "->value: %d[ppc]",getEncoderCount());// mostro valore
          }else {
            setEncMaxCount(4);
            // seleziono linee della pagina
            SelectLine = getEncoderCount();// il conteggio encoder è la linea selezionata
          }

          myPage = Menu[depthMenu];
          printMenu(myPage, SelectLine); // stampa pagina corrente

          if(isEncoderButtonPressed()){
            // se premo bottone encoder
            ssd1306GotoXy(&SSD1306D1, 0, (SelectLine == 0 ? 0 : SelectLine * 11 + 5)); //riga display selezionata
            switch (depthMenu) {
                  case 0: //seleziona tipo di onda
                    if (SelectLine==4){
                      currentMode = OLED_MODE_SCOPE;
                    }else if(SelectLine !=0){
                      // SelectLine ==1 -> Sinusoide
                      // SelectLine ==2 -> Triangolare
                      // SelectLine ==3 -> Rettangolare
                      setShape((OUTShape)(SelectLine-1));
                    }
                    break;

                  case 1:// seleziona un tipo di parametro
                    SelectFunc = SelectLine;
                    switch (SelectLine) {
                    case 1: chsnprintf(FSstring, BUFF_SIZE, " FS: 3.3 [v]");        break; // Ampiezza pp
                    case 2: chsnprintf(FSstring, BUFF_SIZE, " FS: 3.3 [v]");        break; // Offset
                    case 3: chsnprintf(FSstring, BUFF_SIZE, " FS: %d [ns]", 1);     break; // Frequenza
                    case 4: chsnprintf(FSstring, BUFF_SIZE, " FS: %d [ppc]", 100);  break; // Duty Cycle
                    }
                    break;

                  case 2://seleziona il vlore percentuale [0-100] da impostare
                    // passa il valore "Enc_count" alla funzione puntata
                    switch (SelectFunc) {
                    case 1: setAmplitudeFromPerc(getEncoderCount());    break; // setting del ampiezza
                    case 2: setOffsetFromPerc(getEncoderCount());       break; // setting del offset
                    case 3: setPeriodFromPerc(getEncoderCount());       break; // setting della frequenza
                    case 4: setDutyCycleFromPerc(getEncoderCount());    break; // setting del dutycycle
                    }
                    break;
                  }        //end switch

            if (SelectLine == 0) {
              //torna alla pagina precedente
              depthMenu <= 0 ? depthMenu = 0 : depthMenu--;
              ssd1306Puts(&SSD1306D1, "x", &ssd1306_font_7x10, SSD1306_COLOR_BLACK);
            }else {
              // approfondisci menu -> pagina avanti
              depthMenu >= 2 ? depthMenu = 2 : depthMenu++;
              ssd1306Puts(&SSD1306D1, " -x", &ssd1306_font_7x10, SSD1306_COLOR_BLACK);

              if(depthMenu==2){
                //nel caso profondità 2 visualizzo valore settato precedentemente
                setEncMaxCount(100); //set max value encoder
                switch (SelectFunc) {
                  case 1: setEncCount(getAmplitudePerc());    break;// Ampiezza pp
                  case 2: setEncCount(getOffsetPerc());       break;// Offset
                  case 3: setEncCount(getPeriodPerc());       break;// Frequenza
                  case 4: setEncCount(getDutyCyclePerc());    break;// Duty Cycle
                }
              }
            }
            SelectLine = 0;
          }
          break;

        case OLED_MODE_SCOPE:
          oledUpdateScope();
          ssd1306FillScreen(&SSD1306D1, 0x00);

          ssd1306GotoXy(&SSD1306D1, 0, 0); //imposta cursore virtuale
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

              int16_t centered = (int16_t)samp;   //samp centrato
              int y =  DISPLAY_HEIGHT - ((centered * scope_height) / scopeMaxValue); //valore samp centrato -> y display

              ssd1306DrawLine(&SSD1306D1, prev_x, prev_y, x, y, SSD1306_COLOR_WHITE);

              prev_x = x;
              prev_y = y;
            }
          }

          if(isEncoderButtonPressed()){
            depthMenu <= 0 ? depthMenu = 0 : depthMenu--; //torna alla pagina precedente
            ssd1306Puts(&SSD1306D1, "x", &ssd1306_font_7x10, SSD1306_COLOR_BLACK);
            currentMode = OLED_MODE_MENU;
          }
          break;
        }

        ssd1306UpdateScreen(&SSD1306D1);
        chThdSleepMilliseconds(50);
    }
}

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
