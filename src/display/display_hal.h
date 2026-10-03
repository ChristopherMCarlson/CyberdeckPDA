#pragma once

#include <Arduino.h>
#define LGFX_USE_V1
#include <LovyanGFX.hpp>
#include <lvgl.h>
#include "config.h"

class ES3C28P_GFX : public lgfx::LGFX_Device {
    lgfx::Panel_ILI9341 _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;
    lgfx::Touch_FT5x06  _touch_instance;

public:
    ES3C28P_GFX();
};

class DisplayHAL {
public:
    static DisplayHAL& getInstance() {
        static DisplayHAL instance;
        return instance;
    }

    void init();
    void setBacklight(uint8_t brightness);
    uint8_t getBacklight() const { return _currentBrightness; }
    void update();
    ES3C28P_GFX& getGFX() { return _gfx; }

    static void lvglFlushCallback(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p);
    static void lvglTouchReadCallback(lv_indev_drv_t* indev_drv, lv_indev_data_t* data);

private:
    DisplayHAL();
    ~DisplayHAL() = default;

    ES3C28P_GFX _gfx;
    uint8_t _currentBrightness = 200;

    // LVGL buffers
    static const size_t BUF_SIZE = SCREEN_WIDTH * 30; // 30 lines buffer
    lv_color_t* _buf1 = nullptr;
    lv_color_t* _buf2 = nullptr;
    lv_disp_draw_buf_t _draw_buf;
    lv_disp_drv_t _disp_drv;
    lv_indev_drv_t _indev_drv;
    lv_indev_t* _touch_indev = nullptr;
    lv_obj_t* _touch_cursor = nullptr;
};
