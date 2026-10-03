#pragma once

#include <Arduino.h>

// ==========================================
// System & UI Info
// ==========================================
#define CYBERDECK_NAME        "CyberDeck PDA"
#define CYBERDECK_VERSION     "v0.1.0"
#define SCREEN_WIDTH          320
#define SCREEN_HEIGHT         240

// ==========================================
// ES3C28P Board Display Pinout (ILI9341V SPI)
// ==========================================
#define TFT_SPI_HOST          SPI2_HOST
#define TFT_PIN_MOSI          11
#define TFT_PIN_MISO          13
#define TFT_PIN_SCLK          12
#define TFT_PIN_CS            10
#define TFT_PIN_DC            46
#define TFT_PIN_RST           -1  // Tied to system reset / EN
#define TFT_PIN_BL            45  // Backlight PWM

// ==========================================
// ES3C28P Board Capacitive Touch (FT6336G I2C)
// ==========================================
#define I2C_PIN_SDA           16
#define I2C_PIN_SCL           15
#define TOUCH_PIN_INT         17
#define TOUCH_PIN_RST         18
#define TOUCH_I2C_ADDR        0x38

// ==========================================
// Solderparty BBQ20KBD Keyboard (I2C)
// Connected to the shared 4-pin I2C port or header
// ==========================================
#define BBQ20KBD_I2C_ADDR     0x1F
#define BBQ20KBD_PIN_INT      -1  // Optional interrupt pin, -1 for polled

// ==========================================
// Preferences & Storage Namespaces
// ==========================================
#define PREF_WIFI_NAMESPACE   "cyber_wifi"
#define PREF_KEY_SSID         "ssid"
#define PREF_KEY_PASS         "pass"
#define PREF_KEY_BRIGHTNESS   "bright"
#define PREF_KEY_KBD_BL       "kbd_bl"
