#include <Arduino.h>
#include <Wire.h>
#include "config.h"
#include "display/display_hal.h"
#include "input/keyboard_hal.h"
#include "network/wifi_manager.h"
#include "ui/ui.h"

void setup() {
    Serial.begin(115200);
    delay(500);

    Serial.println("\n========================================");
    Serial.printf("  %s %s\n", CYBERDECK_NAME, CYBERDECK_VERSION);
    Serial.println("  ESP32-S3 + ES3C28P + BBQ20KBD");
    Serial.println("========================================");

    // 1. Initialize Display and LVGL (brings up shared I2C bus on GPIO 16/15)
    DisplayHAL::getInstance().init();

    // 2. Initialize Solderparty BBQ20KBD (uses shared I2C bus)
    KeyboardHAL::getInstance().init();

    // 3. Initialize WiFi Subsystem (auto-connects if credentials saved)
    WiFiManager::getInstance().init();

    // 4. Initialize CyberDeck UI (Status bar, Home Screen, Settings App)
    CyberUI::getInstance().init();

    Serial.println("[System] Boot complete! Starting main loop.");
}

void loop() {
    // Service LVGL GUI rendering and touch events
    DisplayHAL::getInstance().update();

    // Service WiFi state machine (async scan & connect)
    WiFiManager::getInstance().update();

    // Service UI updates (Clock, Status Bar)
    CyberUI::getInstance().update();

    // Small yield for FreeRTOS scheduler & power management
    delay(5);
}
