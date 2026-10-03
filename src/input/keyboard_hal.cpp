#include "input/keyboard_hal.h"
#include "ui/ui.h"

KeyboardHAL::KeyboardHAL() {
}

bool KeyboardHAL::init() {
    Serial.println("[KeyboardHAL] Initializing BBQ20KBD on shared I2C bus...");

    // Ensure LovyanGFX I2C port 0 is initialized
    if (!lgfx::i2c::isInitialized(0)) {
        lgfx::i2c::init(0, I2C_PIN_SDA, I2C_PIN_SCL);
    }

    // Test communication with BBQ20KBD
    uint8_t version = readRegister(BBQ_REG_VER);
    if (version == 0) {
        delay(50);
        version = readRegister(BBQ_REG_VER);
    }

    if (version == 0) {
        Serial.println("[KeyboardHAL] BBQ20KBD not responding at 0x1F. Operating touch-only.");
        _connected = false;
        return false;
    }

    _connected = true;
    Serial.printf("[KeyboardHAL] BBQ20KBD connected! Firmware version: 0x%02X\n", version);

    // Set configuration: enable CFG_OVERFLOW_ON (so FIFO doesn't lock up when full),
    // CFG_KEY_INT and CFG_USE_MODS
    writeRegister(BBQ_REG_CFG, BBQ_CFG_OVERFLOW_ON | BBQ_CFG_KEY_INT | BBQ_CFG_USE_MODS);

    // Set keyboard backlight
    setBacklight(_backlight);

    // Clear initial trackpad delta registers
    readRegister(BBQ_REG_TOX);
    readRegister(BBQ_REG_TOY);

    // Drain any leftover events from boot in the FIFO
    BBQKeyEvent flushEvt;
    int drained = 0;
    while (readFIFO(flushEvt) && drained < 35) {
        drained++;
    }
    Serial.printf("[KeyboardHAL] BBQ20KBD ready (drained %d stale events)\n", drained);

    // Register LVGL keypad input device
    lv_indev_drv_init(&_indev_drv);
    _indev_drv.type = LV_INDEV_TYPE_KEYPAD;
    _indev_drv.read_cb = lvglKeypadReadCallback;
    _indev_drv.user_data = this;
    _kbd_indev = lv_indev_drv_register(&_indev_drv);

    return true;
}

void KeyboardHAL::setGroup(lv_group_t* group) {
    _group = group;
    if (_kbd_indev && _group) {
        lv_indev_set_group(_kbd_indev, _group);
    }
}

bool KeyboardHAL::writeRegister(uint8_t reg, uint8_t value) {
    uint8_t buf[2] = { (uint8_t)(reg | BBQ_WRITE_MASK), value };
    return lgfx::i2c::transactionWrite(0, BBQ20KBD_I2C_ADDR, buf, 2, 100000).has_value();
}

uint8_t KeyboardHAL::readRegister(uint8_t reg) {
    uint8_t val = 0;
    auto res = lgfx::i2c::transactionWriteRead(0, BBQ20KBD_I2C_ADDR, &reg, 1, &val, 1, 100000);
    return res.has_value() ? val : 0;
}

void KeyboardHAL::setBacklight(uint8_t brightness) {
    _backlight = brightness;
    if (_connected) {
        writeRegister(BBQ_REG_BKL, brightness);
    }
}

bool KeyboardHAL::readFIFO(BBQKeyEvent& event) {
    if (!_connected) return false;

    // Check key count from REG_KEY (0x04)
    uint8_t keyReg = BBQ_REG_KEY;
    uint8_t keyCount = 0;
    auto resKey = lgfx::i2c::transactionWriteRead(0, BBQ20KBD_I2C_ADDR, &keyReg, 1, &keyCount, 1, 100000);
    uint8_t count = resKey.has_value() ? (keyCount & 0x1F) : 0;

    // Read REG_FIF (0x09)
    uint8_t fifReg = BBQ_REG_FIF;
    uint8_t buf[2] = {0, 0};
    auto resFif = lgfx::i2c::transactionWriteRead(0, BBQ20KBD_I2C_ADDR, &fifReg, 1, buf, 2, 100000);

    if (resFif.has_value() && (buf[0] != 0 || buf[1] != 0)) {
        event.state = (BBQKeyState)buf[0];
        event.key = buf[1];
        Serial.printf("[BBQ20 RAW] count=%d state=%d key=0x%02X ('%c')\n",
                      count, buf[0], buf[1], (buf[1] >= 32 && buf[1] < 127) ? buf[1] : '?');
        return true;
    }

    if (count > 0) {
        Serial.printf("[BBQ20 MISMATCH] keyCount=%d, but REG_FIF returned [0x%02X, 0x%02X] (ok=%d)\n",
                      count, buf[0], buf[1], resFif.has_value());
    }

    return false;
}

uint32_t KeyboardHAL::translateKeyToLVGL(uint8_t rawKey) {
    uint8_t clean = (rawKey & 0x80) ? (rawKey & 0x7F) : rawKey;
    switch (clean) {
        case 8:
        case 0x7F:
            return LV_KEY_BACKSPACE;
        case 10:
        case 13:
        case BBQ_KEY_JOY_CENTER: // 0x05 or 0x85: Trackpad click!
            return LV_KEY_ENTER;
        case 9:
            return LV_KEY_NEXT;
        case 27:
        case BBQ_KEY_BTN_CALL:   // 0x06 or 0x86: Call button
        case BBQ_KEY_BTN_BACK:   // 0x07 or 0x87: Back button
        case BBQ_KEY_BTN_MENU:   // 0x11 or 0x91: BlackBerry menu button
        case BBQ_KEY_BTN_RIGHT2: // 0x12 or 0x92: Secondary Back / End Call
            return LV_KEY_ESC;
        case BBQ_KEY_JOY_UP:    // 1
            return LV_KEY_UP;
        case BBQ_KEY_JOY_DOWN:  // 2
            return LV_KEY_DOWN;
        case BBQ_KEY_JOY_LEFT:  // 3
            return LV_KEY_LEFT;
        case BBQ_KEY_JOY_RIGHT: // 4
            return LV_KEY_RIGHT;
        default:
            return rawKey;
    }
}

void KeyboardHAL::lvglKeypadReadCallback(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    KeyboardHAL* hal = (KeyboardHAL*)indev_drv->user_data;
    if (!hal || !hal->_connected) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    // Periodic diagnostic log every 2 seconds
    static uint32_t lastDiag = 0;
    if (millis() - lastDiag > 2000) {
        lastDiag = millis();
        uint8_t k = hal->readRegister(BBQ_REG_KEY);
        uint8_t intr = hal->readRegister(BBQ_REG_INT);
        uint8_t cfg = hal->readRegister(BBQ_REG_CFG);
        Serial.printf("[BBQ20 DIAG] KEY=0x%02X (count=%d) INT=0x%02X CFG=0x%02X\n", k, k & 0x1F, intr, cfg);
    }

    // 1. Poll trackpad delta registers (0x15 = X, 0x16 = Y)
    int8_t dx = (int8_t)hal->readRegister(BBQ_REG_TOX);
    int8_t dy = (int8_t)hal->readRegister(BBQ_REG_TOY);

    if (dx != 0 || dy != 0) {
        hal->_trackX += dx;
        hal->_trackY += dy;
        hal->_lastTrackMoveTime = millis();
        Serial.printf("[BBQ20 TRACK] dx=%d dy=%d (accum X=%d Y=%d)\n", dx, dy, hal->_trackX, hal->_trackY);
    } else if (millis() - hal->_lastTrackMoveTime > 250) {
        hal->_trackX = 0;
        hal->_trackY = 0;
    }

    // 2. Trackpad swipe navigation:
    const int16_t SWIPE_THRESH = 15;
    uint32_t now = millis();

    if (now - hal->_lastSwipeTime > 150) {
        if (abs(hal->_trackX) >= SWIPE_THRESH && abs(hal->_trackX) >= abs(hal->_trackY)) {
            if (hal->_trackX > 0) {
                hal->_trackX = 0;
                hal->_trackY = 0;
                hal->_lastSwipeTime = now;
                Serial.println("[BBQ20] Trackpad Swipe RIGHT");
                CyberUI::getInstance().handleNavigation(NavDir::RIGHT);
            } else {
                hal->_trackX = 0;
                hal->_trackY = 0;
                hal->_lastSwipeTime = now;
                Serial.println("[BBQ20] Trackpad Swipe LEFT");
                CyberUI::getInstance().handleNavigation(NavDir::LEFT);
            }
        } else if (abs(hal->_trackY) >= SWIPE_THRESH) {
            if (hal->_trackY > 0) {
                hal->_trackX = 0;
                hal->_trackY = 0;
                hal->_lastSwipeTime = now;
                Serial.println("[BBQ20] Trackpad Swipe DOWN");
                CyberUI::getInstance().handleNavigation(NavDir::DOWN);
            } else {
                hal->_trackX = 0;
                hal->_trackY = 0;
                hal->_lastSwipeTime = now;
                Serial.println("[BBQ20] Trackpad Swipe UP");
                CyberUI::getInstance().handleNavigation(NavDir::UP);
            }
        }
    }

    // 3. Read key FIFO for physical keys & trackpad clicks
    BBQKeyEvent evt;
    if (hal->readFIFO(evt)) {
        hal->_lastEvent = evt;
        uint8_t raw = evt.key;
        uint8_t clean = (raw & 0x80) ? (raw & 0x7F) : raw;
        Serial.printf("[BBQ20 KEY] state=%d raw=0x%02X clean=0x%02X ('%c')\n",
                      evt.state, raw, clean, (raw >= 32 && raw < 127) ? raw : '?');

        // Global Back Button handler:
        // 0x07 / 0x87 = Back button (right of trackpad)
        // 0x12 / 0x92 = End Call (secondary Back)
        // 0x06 / 0x86 = Call button
        // 0x11 / 0x91 = BlackBerry menu button
        // 0x3D        = Physical button sending '='
        // 27          = Escape
        if (clean == BBQ_KEY_BTN_BACK || clean == BBQ_KEY_BTN_RIGHT2 ||
            clean == BBQ_KEY_BTN_CALL || clean == BBQ_KEY_BTN_MENU || 
            raw == 0x3D || raw == 27) {
            if (evt.state == KEY_STATE_PRESSED) {
                Serial.printf("[BBQ20] Back button pressed (raw=0x%02X clean=0x%02X) -> handleBackKey\n", raw, clean);
                CyberUI::getInstance().handleBackKey();
            }
            data->state = LV_INDEV_STATE_REL;
            return;
        }

        // Trackpad Center Click or Enter key handler (clean == 0x05, 10, 13)
        if (clean == BBQ_KEY_JOY_CENTER || clean == 10 || clean == 13) {
            static uint32_t lastClickTime = 0;
            uint32_t clickNow = millis();

            if (evt.state == KEY_STATE_PRESSED) {
                if (clickNow - lastClickTime < 350) {
                    Serial.println("[BBQ20] Click ignored (debounce)");
                    return;
                }
                lastClickTime = clickNow;
                Serial.printf("[BBQ20] Click PRESSED (raw=0x%02X clean=0x%02X)\n", raw, clean);
                hal->_centerPressed = true;
                data->state = LV_INDEV_STATE_PR;
                data->key = LV_KEY_ENTER;
            } else if (evt.state == KEY_STATE_RELEASED) {
                Serial.println("[BBQ20] Click RELEASED");
                hal->_centerPressed = false;
                data->state = LV_INDEV_STATE_REL;
                data->key = LV_KEY_ENTER;
            }
            return;
        }

        // Directional Joystick keys
        if (clean == BBQ_KEY_JOY_UP) {
            if (evt.state == KEY_STATE_PRESSED) CyberUI::getInstance().handleNavigation(NavDir::UP);
            data->state = LV_INDEV_STATE_REL;
            return;
        } else if (clean == BBQ_KEY_JOY_DOWN) {
            if (evt.state == KEY_STATE_PRESSED) CyberUI::getInstance().handleNavigation(NavDir::DOWN);
            data->state = LV_INDEV_STATE_REL;
            return;
        } else if (clean == BBQ_KEY_JOY_LEFT) {
            if (evt.state == KEY_STATE_PRESSED) CyberUI::getInstance().handleNavigation(NavDir::LEFT);
            data->state = LV_INDEV_STATE_REL;
            return;
        } else if (clean == BBQ_KEY_JOY_RIGHT) {
            if (evt.state == KEY_STATE_PRESSED) CyberUI::getInstance().handleNavigation(NavDir::RIGHT);
            data->state = LV_INDEV_STATE_REL;
            return;
        }

        // Other standard keyboard keys
        uint32_t lvglKey = hal->translateKeyToLVGL(evt.key);
        hal->_lastLvglKey = lvglKey;

        if (evt.state == KEY_STATE_PRESSED || evt.state == KEY_STATE_HELD) {
            data->state = LV_INDEV_STATE_PR;
            data->key = lvglKey;
        } else {
            data->state = LV_INDEV_STATE_REL;
            data->key = lvglKey;
        }
    } else {
        data->state = hal->_centerPressed ? LV_INDEV_STATE_PR : LV_INDEV_STATE_REL;
        data->key = hal->_centerPressed ? LV_KEY_ENTER : hal->_lastLvglKey;
    }
}

