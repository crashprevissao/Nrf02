/* ____________________________
   This software is licensed under the MIT License:
   https://github.com/cifertech/nrfbox
   ________________________________________ */

      #include <U8g2lib.h>
#include <Wire.h>
#include <EEPROM.h>
#include <Adafruit_NeoPixel.h>
#include "icon.h"
#include "setting.h"
#include "cc1101.h"  // NOVO: Biblioteca CC1101

#ifdef U8X8_HAVE_HW_SPI
#include <SPI.h>
#endif
#ifdef U8X8_HAVE_HW_I2C
#include <Wire.h>
#endif

#define NUM_BUTTONS 5
#define MAX_ITEM_LENGTH 25

const int BUTTON_PINS[NUM_BUTTONS] = {D6, D3, D7, D1, D0}; // UP, DOWN, LEFT, RIGHT, SELECT

// Alterado de 12 para 13 itens
const int NUM_ITEMS = 13;

// Adicionado bitmap_icon_radio no final
const unsigned char* bitmap_icons[NUM_ITEMS] = {
  bitmap_icon_scanner, bitmap_icon_analyzer, bitmap_icon_jammer, bitmap_icon_kill,
  bitmap_icon_ble_jammer, bitmap_icon_spoofer, bitmap_icon_apple, bitmap_icon_ble,
  bitmap_icon_wifi, bitmap_icon_wifi_jammer, bitmap_icon_about, 
  bitmap_icon_setting,
  bitmap_icon_radio  // NOVO: Ícone do CC1101/Sub-GHz
};

// Adicionado "Sub-GHz" no final
char menu_items[NUM_ITEMS][MAX_ITEM_LENGTH] = {  
  "Scanner", "Analyzer", "WLAN Jammer", "Proto Kill", "BLE Jammer",
  "BLE Spoofer", "Sour Apple", "BLE Scan", "WiFi Scan", 
  "Deauther", "About", "Setting",
  "Sub-GHz"  // NOVO: Nome do menu CC1101
};

// Adicionadas funções do CC1101 no final
void (*menu_functions[NUM_ITEMS])() = {
  Scanner::scannerSetup, Analyzer::analyzerSetup, Jammer::jammerSetup,
  ProtoKill::blackoutSetup, BleJammer::blejammerSetup, Spoofer::spooferSetup,
  SourApple::sourappleSetup, BleScan::blescanSetup, WifiScan::wifiscanSetup, 
  Deauther::deautherSetup, utils, Setting::settingSetup,
  CC1101::cc1101Setup  // NOVO: Função setup do CC1101
};

void (*menu_loop_functions[NUM_ITEMS])() = {
  Scanner::scannerLoop, Analyzer::analyzerLoop, Jammer::jammerLoop,
  ProtoKill::blackoutLoop, BleJammer::blejammerLoop, Spoofer::spooferLoop,
  SourApple::sourappleLoop, BleScan::blescanLoop, WifiScan::wifiscanLoop, 
  Deauther::deautherLoop, nullptr, Setting::settingLoop,
  CC1101::cc1101Loop  // NOVO: Função loop do CC1101
};

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE);

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(1, D8, NEO_GRB + NEO_KHZ800);

int current_selection = 0;
int item_selected = -1;
int current_screen = 0;
int previous_screen = 0;
int next_screen = 0;

int previous_button_state[NUM_BUTTONS];
int current_button_state[NUM_BUTTONS];

void setup(void) {
  pixels.begin();
  pixels.setPixelColor(0, pixels.Color(255, 0, 0));
  pixels.show();

  u8g2.begin();
  u8g2.setBitmapMode(1);
  
  for (int i = 0; i < NUM_BUTTONS; i++) {
    pinMode(BUTTON_PINS[i], INPUT_PULLUP);
    previous_button_state[i] = HIGH;
  }

  if (EEPROM.read(0) == 255) {
    EEPROM.write(0, 0);
    EEPROM.commit();
  }
}

void loop(void) {
  u8g2.clearBuffer();
  drawMenu();
  u8g2.sendBuffer();
  
  checkButtonState();
}

void drawMenu(void) {
  int offset = 0;
  if (current_selection >= 6) {
    offset = current_selection / 6 * 6;
  }
  
  int num_items_to_display = min(6, NUM_ITEMS - offset);
  
  for (int i = 0; i < num_items_to_display; i++) {
    int item_index = i + offset;
    int x = (i % 3) * 43;
    int y = (i / 3) * 32;
    
    if (item_index == current_selection) {
      u8g2.setDrawColor(2);
      u8g2.drawBox(x, y, 42, 32);
      u8g2.setDrawColor(1);
    }
    
    u8g2.drawXBMP(x + 5, y + 2, 32, 32, bitmap_icons[item_index]);
    
    int textWidth = u8g2.getStrWidth(menu_items[item_index]);
    int textX = x + (42 - textWidth) / 2;
    
    u8g2.setFont(u8g2_font_5x7_tf);
    u8g2.drawStr(textX, y + 30, menu_items[item_index]);
  }
}

void checkButtonState(void) {
  for (int i = 0; i < NUM_BUTTONS; i++) {
    current_button_state[i] = digitalRead(BUTTON_PINS[i]);
    
    if (current_button_state[i] == LOW && previous_button_state[i] == HIGH) {
      if (i == 0) { // UP
        if (current_selection > 0) {
          current_selection--;
        }
      } else if (i == 1) { // DOWN
        if (current_selection < NUM_ITEMS - 1) {
          current_selection++;
        }
      } else if (i == 2) { // LEFT
        if (current_selection > 0) {
          current_selection--;
        }
      } else if (i == 3) { // RIGHT
        if (current_selection < NUM_ITEMS - 1) {
          current_selection++;
        }
      } else if (i == 4) { // SELECT
        item_selected = current_selection;
        current_screen = item_selected + 1;
        previous_screen = 0;
        
        if (menu_functions[item_selected] != nullptr) {
          menu_functions[item_selected]();
        }
      }
    }
    previous_button_state[i] = current_button_state[i];
  }
}

void utils(void) {
  // Função vazia para o item "About"
}
