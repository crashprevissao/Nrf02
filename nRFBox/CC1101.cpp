/* ____________________________
   CC1101 Sub-GHz Module Implementation
   ________________________________________ */

#include "cc1101.h"

namespace CC1101 {
  bool initialized = false;
  bool capturing = false;
  bool signalCaptured = false;
  float currentFreq = FREQ_433MHZ;
  int selectedOption = 0;
  int freqIndex = 1; // 433MHz padrão
  
  const char* freqNames[] = {"315 MHz", "433 MHz", "868 MHz", "915 MHz"};
  const float frequencies[] = {FREQ_315MHZ, FREQ_433MHZ, FREQ_868MHZ, FREQ_915MHZ};
  
  CapturedSignal storedSignal;
  unsigned long lastActivity = 0;
  
  // Buffer para recepção
  byte rxBuffer[64];
  byte rxLen = 0;

  void cc1101Setup() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 20, "CC1101 Sub-GHz");
    u8g2.drawStr(10, 35, "Inicializando...");
    u8g2.sendBuffer();
    
    if (!initCC1101()) {
      u8g2.clearBuffer();
      u8g2.drawStr(10, 20, "ERRO CC1101!");
      u8g2.drawStr(10, 35, "Verifique conexoes:");
      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(10, 50, "CS:D13 MOSI:D12");
      u8g2.drawStr(10, 58, "MISO:D35 SCK:D4");
      u8g2.sendBuffer();
      delay(3000);
      return;
    }
    
    initialized = true;
    selectedOption = 0;
    
    // Tenta carregar sinal salvo
    loadSignalFromEEPROM();
    
    drawMenu();
  }

  void cc1101Loop() {
    if (!initialized) return;
    
    // Navegação no menu
    if (digitalRead(BUTTON_UP_PIN) == LOW) {
      selectedOption--;
      if (selectedOption < 0) selectedOption = 3;
      delay(200);
      drawMenu();
    }
    
    if (digitalRead(BUTTON_DOWN_PIN) == LOW) {
      selectedOption++;
      if (selectedOption > 3) selectedOption = 0;
      delay(200);
      drawMenu();
    }
    
    if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
      delay(200);
      switch(selectedOption) {
        case 0: // Capturar
          captureSignal();
          break;
        case 1: // Reproduzir
          replaySignal();
          break;
        case 2: // Mudar Frequência
          changeFrequency();
          break;
        case 3: // Voltar
          // Sai do menu
          break;
      }
      drawMenu();
    }
    
    // Atualiza display se capturando
    if (capturing) {
      drawCaptureScreen();
    }
  }

  bool initCC1101() {
    // Configura pinos SPI
    ELECHOUSE_cc1101.setSpiPin(CC1101_SCK, CC1101_MISO, CC1101_MOSI, CC1101_CS);
    
    // Inicializa
    if (ELECHOUSE_cc1101.getCC1101()) {
      ELECHOUSE_cc1101.Init();
      ELECHOUSE_cc1101.setMHZ(currentFreq);
      ELECHOUSE_cc1101.SetRx();
      return true;
    }
    return false;
  }

  void setCC1101Frequency(float freq) {
    currentFreq = freq;
    ELECHOUSE_cc1101.setMHZ(freq);
    ELECHOUSE_cc1101.SetRx();
  }

  void drawMenu() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    
    // Título
    u8g2.drawStr(25, 10, "SUB-GHz RF");
    u8g2.drawLine(0, 12, 128, 12);
    
    // Ícone de rádio
    drawRadioIcon(100, 2);
    
    // Info da frequência atual
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(5, 22, "Freq: ");
    u8g2.drawStr(35, 22, freqNames[freqIndex]);
    
    // Menu de opções
    const char* options[] = {"1. Capturar Sinal", "2. Reproduzir", 
                            "3. Trocar Freq", "4. Voltar"};
    
    for (int i = 0; i < 4; i++) {
      int y = 35 + (i * 12);
      
      if (i == selectedOption) {
        u8g2.drawBox(0, y - 8, 128, 11);
        u8g2.setDrawColor(0);
      } else {
        u8g2.setDrawColor(1);
      }
      
      u8g2.drawStr(5, y, options[i]);
      u8g2.setDrawColor(1);
    }
    
    // Indicador de sinal salvo
    if (storedSignal.valid) {
      u8g2.drawStr(90, 22, "[SALVO]");
    }
    
    u8g2.sendBuffer();
  }

  void captureSignal() {
    capturing = true;
    signalCaptured = false;
    
    ELECHOUSE_cc1101.SetRx();
    unsigned long startTime = millis();
    bool timeout = false;
    
    while (!signalCaptured && !timeout) {
      // Verifica botão de saída
      if (digitalRead(BTN_PIN_LEFT) == LOW) {
        capturing = false;
        delay(200);
        return;
      }
      
      drawCaptureScreen();
      
      // Verifica se recebeu dados
      if (ELECHOUSE_cc1101.CheckReceiveFlag()) {
        rxLen = 64;
        if (ELECHOUSE_cc1101.CheckRxFifo(rxLen, rxBuffer)) {
          // Capturou!
          storedSignal.length = rxLen;
          memcpy(storedSignal.data, rxBuffer, rxLen);
          storedSignal.frequency = currentFreq;
          storedSignal.rssi = ELECHOUSE_cc1101.getRssi();
          storedSignal.timestamp = millis();
          storedSignal.valid = true;
          signalCaptured = true;
          
          // Salva na EEPROM
          saveSignalToEEPROM();
          
          // Tela de sucesso
          u8g2.clearBuffer();
          u8g2.setFont(u8g2_font_6x10_tf);
          u8g2.drawStr(20, 20, "SINAL CAPTURADO!");
          u8g2.setFont(u8g2_font_5x7_tf);
          u8g2.drawStr(10, 35, "RSSI: ");
          char rssiStr[10];
          sprintf(rssiStr, "%d dBm", storedSignal.rssi);
          u8g2.drawStr(40, 35, rssiStr);
          u8g2.drawStr(10, 45, "Bytes: ");
          char lenStr[10];
          sprintf(lenStr, "%d", storedSignal.length);
          u8g2.drawStr(50, 45, lenStr);
          u8g2.drawStr(10, 55, "Salvo na memoria");
          u8g2.sendBuffer();
          delay(2000);
        }
      }
      
      // Timeout de 30 segundos
      if (millis() - startTime > 30000) {
        timeout = true;
      }
      
      delay(10);
    }
    
    capturing = false;
    
    if (timeout && !signalCaptured) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(15, 30, "Timeout!");
      u8g2.drawStr(10, 45, "Nenhum sinal");
      u8g2.sendBuffer();
      delay(2000);
    }
  }

  void drawCaptureScreen() {
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(10, 10, "MODO CAPTURA");
    u8g2.drawLine(0, 12, 128, 12);
    
    u8g2.setFont(u8g2_font_5x7_tf);
    char freqStr[20];
    sprintf(freqStr, "Freq: %.2f MHz", currentFreq);
    u8g2.drawStr(5, 22, freqStr);
    
    // RSSI em tempo real
    int8_t rssi = ELECHOUSE_cc1101.getRssi();
    u8g2.drawStr(5, 32, "RSSI: ");
    char rssiStr[15];
    sprintf(rssiStr, "%d dBm", rssi);
    u8g2.drawStr(40, 32, rssiStr);
    
    // Barras de sinal
    drawSignalBars(90, 25, rssi);
    
    // Animação de espera
    static int anim = 0;
    anim = (anim + 1) % 4;
    const char* animChars[] = {"|", "/", "-", "\\"};
    u8g2.drawStr(60, 45, "Aguardando");
    u8g2.drawStr(75, 55, animChars[anim]);
    
    u8g2.drawStr(5, 60, "LEFT: Sair");
    u8g2.sendBuffer();
  }

  void replaySignal() {
    if (!storedSignal.valid) {
      u8g2.clearBuffer();
      u8g2.setFont(u8g2_font_6x10_tf);
      u8g2.drawStr(10, 25, "Nenhum sinal");
      u8g2.drawStr(15, 40, "capturado!");
      u8g2.setFont(u8g2_font_5x7_tf);
      u8g2.drawStr(10, 55, "Capture um primeiro");
      u8g2.sendBuffer();
      delay(2000);
      return;
    }
    
    // Tela de confirmação
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(15, 15, "REPRODUZIR?");
    u8g2.setFont(u8g2_font_5x7_tf);
    
    char freqStr[25];
    sprintf(freqStr, "Freq: %.2f MHz", storedSignal.frequency);
    u8g2.drawStr(10, 28, freqStr);
    
    char rssiStr[20];
    sprintf(rssiStr, "RSSI: %d dBm", storedSignal.rssi);
    u8g2.drawStr(10, 38, rssiStr);
    
    char lenStr[15];
    sprintf(lenStr, "Bytes: %d", storedSignal.length);
    u8g2.drawStr(10, 48, lenStr);
    
    u8g2.drawStr(10, 60, "SEL:Transmitir LFT:Sair");
    u8g2.sendBuffer();
    
    // Aguarda confirmação
    while (true) {
      if (digitalRead(BUTTON_SELECT_PIN) == LOW) {
        delay(200);
        
        // Transmite
        u8g2.clearBuffer();
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(20, 30, "TRANSMITINDO...");
        u8g2.sendBuffer();
        
        // Configura frequência do sinal salvo
        ELECHOUSE_cc1101.setMHZ(storedSignal.frequency);
        ELECHOUSE_cc1101.SetTx();
        
        // Envia os dados
        ELECHOUSE_cc1101.SendData(storedSignal.data, storedSignal.length);
        
        delay(500);
        
        u8g2.drawStr(30, 45, "Concluido!");
        u8g2.sendBuffer();
        delay(1000);
        
        // Volta para frequência atual
        ELECHOUSE_cc1101.setMHZ(currentFreq);
        ELECHOUSE_cc1101.SetRx();
        return;
      }
      
      if (digitalRead(BTN_PIN_LEFT) == LOW) {
        delay(200);
        return;
      }
      
      delay(50);
    }
  }

  void changeFrequency() {
    freqIndex++;
    if (freqIndex > 3) freqIndex = 0;
    currentFreq = frequencies[freqIndex];
    setCC1101Frequency(currentFreq);
    
    // Feedback visual
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_6x10_tf);
    u8g2.drawStr(20, 25, "Frequencia:");
    u8g2.drawStr(30, 40, freqNames[freqIndex]);
    u8g2.sendBuffer();
    delay(1000);
  }

  void saveSignalToEEPROM() {
    if (!storedSignal.valid) return;
    
    // Endereço na EEPROM (após as outras configurações)
    int addr = 200; // Área reservada para CC1101
    
    EEPROM.write(addr, storedSignal.valid ? 1 : 0);
    EEPROM.write(addr + 1, storedSignal.length);
    EEPROM.put(addr + 2, storedSignal.frequency);
    EEPROM.put(addr + 6, storedSignal.rssi);
    
    for (int i = 0; i < storedSignal.length && i < 64; i++) {
      EEPROM.write(addr + 10 + i, storedSignal.data[i]);
    }
    
    EEPROM.commit();
  }

  bool loadSignalFromEEPROM() {
    int addr = 200;
    
    uint8_t valid = EEPROM.read(addr);
    if (valid != 1) return false;
    
    storedSignal.valid = true;
    storedSignal.length = EEPROM.read(addr + 1);
    EEPROM.get(addr + 2, storedSignal.frequency);
    EEPROM.get(addr + 6, storedSignal.rssi);
    
    for (int i = 0; i < storedSignal.length && i < 64; i++) {
      storedSignal.data[i] = EEPROM.read(addr + 10 + i);
    }
    
    return true;
  }

  void drawRadioIcon(int x, int y) {
    // Ícone de rádio/transmissor 16x16
    const unsigned char radio_icon[] = {
      0x00, 0x00, 0x00, 0x06, 0x00, 0x07, 0x00, 0x03, 0x00, 0x00, 0x70, 0x0e, 
      0xf8, 0x0f, 0xf8, 0x07, 0xf8, 0x07, 0xf8, 0x07, 0xf8, 0x0f, 0xf8, 0x1f, 
      0xf0, 0x1f, 0xe0, 0x0f, 0x00, 0x00, 0x00, 0x00
    };
    u8g2.drawXBMP(x, y, 16, 16, radio_icon);
  }

  void drawSignalBars(int x, int y, int8_t rssi) {
    // Desenha barras de intensidade do sinal
    int bars = 0;
    if (rssi > -50) bars = 4;
    else if (rssi > -60) bars = 3;
    else if (rssi > -70) bars = 2;
    else if (rssi > -80) bars = 1;
    
    for (int i = 0; i < 4; i++) {
      int barHeight = (i + 1) * 3;
      if (i < bars) {
        u8g2.drawBox(x + (i * 5), y + 12 - barHeight, 4, barHeight);
      } else {
        u8g2.drawFrame(x + (i * 5), y + 12 - barHeight, 4, barHeight);
      }
    }
  }
}
