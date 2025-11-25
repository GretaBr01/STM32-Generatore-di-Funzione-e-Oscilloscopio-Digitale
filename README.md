# STM32 Generatore di Funzione e Oscilloscopio Digitale

Progetto per Nucleo STM32 che implementa un generatore di funzioni DAC e un semplice oscilloscopio su display SSD1306.

## Panoramica
- Generatore di forme d'onda (seno, rettangolare, triangolare, saw) utilizzando DAC.
    - Funzioni principali: [`initOut`](outsetting.c), [`startOut`](outsetting.c), [`setShape`](outsetting.h).
- Acquisizione ADC campiona il segnale e alimenta il buffer per lo scope.
  - Funzioni principali: [`startADCThread`](sampler.c), [`updateScope`](sampler.c).
- Visualizzazione su OLED (SSD1306).
  - Driver e API: [ssd1306/ssd1306.h](ssd1306/ssd1306.h), implementazione [ssd1306/ssd1306.c](ssd1306/ssd1306.c). Font: [ssd1306/ssd1306_font.c](ssd1306/ssd1306_font.c), [ssd1306/ssd1306_font_7_10.c](ssd1306/ssd1306_font_7_10.c).
  - Interfaccia alto livello: [`startOledThread`](oled_manager.c), [`oledUpdateScope`](oled_manager.h).
- Gestione encoder rotativo e pulsante.
  - Funzioni: [`startEncoderThread`](encoder_manager.c), [`getEncoderCount`](encoder_manager.c), [`isEncoderButtonPressed`](encoder_manager.c).

## File principali
- [main.c](main.c) — nodo di avvio, gestione pulsante on/off e thread LED.
- [Makefile](Makefile) — build system (ChibiOS + HAL + ssd1306).
- [outsetting.c](outsetting.c), [outsetting.h](outsetting.h) — generatore DAC e configurazione.
- [sampler.c](sampler.c), [sampler.h](sampler.h) — acquisizione ADC e buffer scope.
- [oled_manager.c](oled_manager.c), [oled_manager.h](oled_manager.h) — logica menu e disegno su display.
- [encoder_manager.c](encoder_manager.c), [encoder_manager.h](encoder_manager.h) — lettura encoder e pulsante.
- [ssd1306/ssd1306.c](ssd1306/ssd1306.c), [ssd1306/ssd1306.h](ssd1306/ssd1306.h) — driver SSD1306 e helper.
- [ssd1306/ssd1306_font.c](ssd1306/ssd1306_font.c), [ssd1306/ssd1306_font_7_10.c](ssd1306/ssd1306_font_7_10.c) — dati font.
- [cfg/halconf.h](cfg/halconf.h), [cfg/chconf.h](cfg/chconf.h), [cfg/mcuconf.h](cfg/mcuconf.h) — configurazioni ChibiOS/HAL.

## Hardware
- STM32 Nucleo-64 ST_NUCLEO64_G474RE.
- Display OLED SSD1306 128x64.
- LED RGB.
- Encoder rotante con pulsante.
- Pulsante ON/OFF.

## Mappatura pin
Di seguito sono riportati i pin definiti nel codice (o suggeriti come mapping comune). Verificare sempre i file sorgente (outsetting.c, sampler.c, encoder_manager.c, main.c) e adattare al cablaggio reale.

- I2C (SSD1306)
  - SCL → PB8
  - SDA → PB9

- LED (definiti in main.c)
  - R LED → PB10
  - G LED → PA8
  - B LED → PA9

- Pulsante ON/OFF (definito in main.c)
  - BUTTON ON/OFF → PC7 
  - Nota: input pull-up, evento su entrambi i fronti

- DAC (uscita generatore)
  - DAC OUT1 → PA4  (DAC1_OUT1)
  - Nota: mapping comune, verificare outsetting.c

- ADC (ingresso oscilloscopio)
  - ADC IN (scope input) → PA0 (ADC_IN1)
  - Nota: adattare a sampler.c

- Encoder
  - ENC_CLK →  PB5
  - ENC_DT  →  PB3
  - ENC_SW  →  PB4
  - Nota: i pin effettivi sono definiti in encoder_manager.h;


## Uso
- Premere pulsante ON/OFF per attivare/disattivare sistema.
- Quando acceso, i thread vengono avviati automaticamente:
  - initOut(), startOut() => DAC
  - startADCThread() => campionamento ADC
  - startOledThread() => GUI OLED
  - startEncoderThread() => controllo menu
- Ruotare l'encoder e premere il pulsante encoder per navigare il menu su OLED.
