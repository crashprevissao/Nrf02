/* ____________________________
   CC1101 Sub-GHz Module for nRFBox
   Supports: 315/433/868/915 MHz
   Functions: Signal Capture & Replay
   ________________________________________ */

#ifndef CC1101_H
#define CC1101_H

#include "config.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>

// Frequências pré-configuradas
#define FREQ_315MHZ  315.0
#define FREQ_433MHZ  433.92
#define FREQ_868MHZ  868.35
#define FREQ_915MHZ  915.0

// Estrutura para armazenar sinal capturado
struct CapturedSignal {
  float frequency;
  uint8_t data[64];
  uint8_t length;
  int8_t rssi;
  bool valid;
  uint32_t timestamp;
};

namespace CC1101 {
  // Variáveis internas
  extern bool initialized;
  extern bool capturing;
  extern bool signalCaptured;
  extern float currentFreq;
  extern int selectedOption;
  extern int freqIndex;
  extern const char* freqNames[];
  extern const float frequencies[];
  extern CapturedSignal storedSignal;
  extern unsigned long lastActivity;
  
  // Funções principais
  void cc1101Setup();
  void cc1101Loop();
  
  // Funções internas
  void drawMenu();
  void drawCaptureScreen();
  void drawReplayScreen();
  void drawStatusBar();
  void captureSignal();
  void replaySignal();
  void changeFrequency();
  bool initCC1101();
  void setCC1101Frequency(float freq);
  void saveSignalToEEPROM();
  bool loadSignalFromEEPROM();
  void clearSignal();
  void drawIcon(int x, int y, bool selected);
  void drawRadioIcon(int x, int y);
  void drawSignalBars(int x, int y, int8_t rssi);
}

#endif // CC1101_H
