#include "encoder_manager.h"

static thread_t *encThd = NULL;
static THD_WORKING_AREA(Enc_wa, 256);

static int enc_count = 0;
static uint8_t enc_BTN = 0;
static mutex_t token;
static int enc_max_count = 3;

static THD_FUNCTION(Enc_THR, arg) {
    (void)arg;
    chRegSetThreadName("Encoder");

    //modalità input con pullup
    palSetLineMode(Enc_A, PAL_MODE_INPUT_PULLUP);
    palSetLineMode(Enc_B, PAL_MODE_INPUT_PULLUP);
    palSetLineMode(Enc_SW, PAL_MODE_INPUT_PULLUP);

    // stati passati (per comparare)
    uint8_t old_A = 0;
    uint8_t old_SW = 1; // normalmente alto

    //stati correnti
    uint8_t new_A;
    uint8_t new_B;
    uint8_t new_SW;

    while (!chThdShouldTerminateX()) {
      // leggo i pin (immagine di processo)
      new_A = palReadLine(Enc_A);
      new_B = palReadLine(Enc_B);
      new_SW = palReadLine(Enc_SW);

      if (new_A != old_A) {    //il canale A ha subito una variazione di stato
        if (new_B == new_A) {    // stati concordi
          //se Concordi -> fronti A e B gia avvenuti
          // verso orario -> risalgo il menu
          enc_count <= 0 ? enc_count = 0 : enc_count--;  //linea iniziale del menu
        }// end concordi
        else {
          enc_count >= enc_max_count ? enc_count = enc_max_count : enc_count++; //fine menu
        }// end Discordi

      }// end ch A & ch B
      old_A = new_A;// aggiornamento
      enc_count >= enc_max_count ? enc_count = enc_max_count : true; //fine menu

      if ((new_SW != old_SW) && (new_SW == 0)) {//variazione di stato
        //fronte di discesa -> pigiato
        chMtxLock(&token);
        enc_BTN = 1;
        chMtxUnlock(&token);
      }
      old_SW = new_SW;// aggiornamento

      chThdSleepMilliseconds(1);
    }
}

void startEncoderThread(void) {
    if (encThd == NULL) {
        chMtxObjectInit(&token);
        encThd = chThdCreateStatic(Enc_wa, sizeof(Enc_wa),
                                   NORMALPRIO, Enc_THR, NULL);
    }
}

void stopEncoderThread(void) {
    if (encThd != NULL) {
        chThdTerminate(encThd);
        chThdWait(encThd);
        encThd = NULL;
    }
}

int getEncoderCount(void) {
    return enc_count;
}

void resetEncoder(void) {
    enc_count = 0;
}

bool isEncoderButtonPressed(void) {
    bool pressed = false;

    if (enc_BTN) {
      pressed = true;
      chMtxLock(&token);
        enc_BTN = 0;
      chMtxUnlock(&token);
    }

    return pressed;
}

void setEncMaxCount(int val){
  enc_max_count = val;
}

int getEncMaxCount(void){
  return enc_max_count;
}

void setEncCount(int val){
  enc_count= val;
}
