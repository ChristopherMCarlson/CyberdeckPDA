#pragma once

#include <Arduino.h>
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "config.h"

// BBQ20KBD Register Definitions (from i2c_puppet protocol)
#define BBQ_REG_VER     0x01  // Firmware version (returns 0x11 for v1.1)
#define BBQ_REG_CFG     0x02  // Configuration register
#define BBQ_REG_INT     0x03  // Interrupt status register
#define BBQ_REG_KEY     0x04  // Key status register (bits 0-4 = FIFO count)
#define BBQ_REG_BKL     0x05  // Backlight brightness (0-255)
#define BBQ_REG_DEB     0x06  // Debounce configuration
#define BBQ_REG_FRQ     0x07  // Polling frequency configuration
#define BBQ_REG_RST     0x08  // Reset register
#define BBQ_REG_FIF     0x09  // FIFO read register (returns 2 bytes: state, key)
#define BBQ_REG_BK2     0x0A  // Secondary backlight
#define BBQ_REG_DIR     0x0B  // GPIO direction
#define BBQ_REG_PUE     0x0C  // GPIO pull enable
#define BBQ_REG_PUD     0x0D  // GPIO pull direction
#define BBQ_REG_HLD     0x11  // Key hold threshold (in 10ms units)
#define BBQ_REG_CF2     0x14  // Configuration register 2
#define BBQ_REG_TOX     0x15  // Trackpad delta X (-128 to 127)
#define BBQ_REG_TOY     0x16  // Trackpad delta Y (-128 to 127)

// Key definitions from BBQ20KBD firmware
#define BBQ_KEY_JOY_UP     0x01
#define BBQ_KEY_JOY_DOWN   0x02
#define BBQ_KEY_JOY_LEFT   0x03
#define BBQ_KEY_JOY_RIGHT  0x04
#define BBQ_KEY_JOY_CENTER 0x05  // Trackpad center press / click
#define BBQ_KEY_BTN_CALL   0x06  // Call button (Green Phone)
#define BBQ_KEY_BTN_BACK   0x07  // Back button (Right 1)
#define BBQ_KEY_BTN_MENU   0x11  // Menu / BlackBerry logo button
#define BBQ_KEY_BTN_RIGHT2 0x12  // End Call / Back button (Right 2)

// Configuration bits for BBQ_REG_CFG
#define BBQ_CFG_OVERFLOW_ON  (1 << 0)
#define BBQ_CFG_OVERFLOW_INT (1 << 1)
#define BBQ_CFG_CAPSLOCK_INT (1 << 2)
#define BBQ_CFG_NUMLOCK_INT  (1 << 3)
#define BBQ_CFG_KEY_INT      (1 << 4)
#define BBQ_CFG_PANIC_INT    (1 << 5)
#define BBQ_CFG_REPORT_MODS  (1 << 6)
#define BBQ_CFG_USE_MODS     (1 << 7)

#define BBQ_WRITE_MASK  0x80

// Key state definitions from BBQ20 firmware
enum BBQKeyState {
    KEY_STATE_IDLE     = 0,
    KEY_STATE_PRESSED  = 1,
    KEY_STATE_HELD     = 2,
    KEY_STATE_RELEASED = 3
};

struct BBQKeyEvent {
    BBQKeyState state;
    uint8_t key;
};

class KeyboardHAL {
public:
    static KeyboardHAL& getInstance() {
        static KeyboardHAL instance;
        return instance;
    }

    bool init();
    void update();
    void setBacklight(uint8_t brightness);
    uint8_t getBacklight() const { return _backlight; }
    bool isConnected() const { return _connected; }
    void setGroup(lv_group_t* group);

    static void lvglKeypadReadCallback(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);

private:
    KeyboardHAL();
    ~KeyboardHAL() = default;

    bool writeRegister(uint8_t reg, uint8_t value);
    uint8_t readRegister(uint8_t reg);
    bool readFIFO(BBQKeyEvent& event);
    uint32_t translateKeyToLVGL(uint8_t rawKey);

    bool _connected = false;
    uint8_t _backlight = 128;
    lv_indev_drv_t _indev_drv;
    lv_indev_t* _kbd_indev = nullptr;
    lv_group_t* _group = nullptr;

    BBQKeyEvent _lastEvent = {KEY_STATE_IDLE, 0};
    uint32_t _lastLvglKey = 0;

    int16_t _trackX = 0;
    int16_t _trackY = 0;
    uint32_t _lastTrackMoveTime = 0;
    uint32_t _lastSwipeTime = 0;
    bool _centerPressed = false;
};

