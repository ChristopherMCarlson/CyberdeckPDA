#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "display/display_hal.h"
#include "input/keyboard_hal.h"
#include "network/wifi_manager.h"
#include "storage/storage_manager.h"
#include "ui/ui.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================");
    Serial.printf("  %s %s\n", CYBERDECK_NAME, CYBERDECK_VERSION);
    Serial.println("  ESP32-S3 + ES3C28P + BBQ20KBD");
    Serial.println("========================================");

    // 0. Free shared I2C bus if a slave (BBQ20 or FT6336) is holding SDA low from a warm reset
    pinMode(I2C_PIN_SDA, INPUT_PULLUP);
    pinMode(I2C_PIN_SCL, OUTPUT_OPEN_DRAIN);
    digitalWrite(I2C_PIN_SCL, HIGH);
    delay(5);
    for (int i = 0; i < 16 && digitalRead(I2C_PIN_SDA) == LOW; i++) {
        digitalWrite(I2C_PIN_SCL, LOW);
        delayMicroseconds(10);
        digitalWrite(I2C_PIN_SCL, HIGH);
        delayMicroseconds(10);
    }
    pinMode(I2C_PIN_SDA, OUTPUT_OPEN_DRAIN);
    digitalWrite(I2C_PIN_SDA, LOW);
    delayMicroseconds(10);
    digitalWrite(I2C_PIN_SCL, HIGH);
    delayMicroseconds(10);
    digitalWrite(I2C_PIN_SDA, HIGH);
    delay(5);
    pinMode(I2C_PIN_SDA, INPUT);
    pinMode(I2C_PIN_SCL, INPUT);

    // 1. Initialize Display and LVGL (brings up shared I2C bus on GPIO 16/15)
    DisplayHAL::getInstance().init();

    // 2. Initialize Solderparty BBQ20KBD (uses shared I2C bus)
    KeyboardHAL::getInstance().init();

    // 3. Initialize WiFi Subsystem (auto-connects if credentials saved)
    WiFiManager::getInstance().init();

    // 4. Initialize Storage Subsystem (SDMMC 4-bit with SPIFFS fallback)
    StorageManager::getInstance().init();

    // 5. Initialize CyberDeck UI (Status bar, Home Screen, Settings App, Notes App)
    CyberUI::getInstance().init();


    Serial.println("[System] Boot complete! Starting main loop.");
}

void loop() {
    // Service BBQ20 auto-reconnect if needed
    KeyboardHAL::getInstance().update();

    // Service LVGL GUI rendering and touch events
    DisplayHAL::getInstance().update();

    // Service WiFi state machine (async scan & connect)
    WiFiManager::getInstance().update();

    // Service UI updates (Clock, Status Bar)
    CyberUI::getInstance().update();

    // Small yield for FreeRTOS scheduler & power management
    delay(5);
}
