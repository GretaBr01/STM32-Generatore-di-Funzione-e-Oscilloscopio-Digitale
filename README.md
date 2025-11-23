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

## Requisiti hardware
- STM32 Nucleo (board definita: ST_NUCLEO64_G474RE in [Makefile](Makefile)).
- Connessione I2C per SSD1306 (PB8=SCL, PB9=SDA come in [main.c](main.c)).
- DAC/ADC pin configurati in codice ([outsetting.c](outsetting.c), [sampler.c](sampler.c)).
- Encoder rotante con pulsante.
- Pulsante ON/OFF (collegato a PA10 come in [main.c](main.c)).

## Uso
- Premere pulsante ON/OFF (PA10) per attivare/disattivare sistema.
- Quando acceso, i thread vengono avviati automaticamente:
  - initOut(), startOut() => DAC
  - startADCThread() => campionamento ADC
  - startOledThread() => GUI OLED
  - startEncoderThread() => controllo menu
- Ruotare l'encoder e premere il pulsante encoder per navigare il menu su OLED.
