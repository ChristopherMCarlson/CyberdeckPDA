#include "display/display_hal.h"
#include <esp_heap_caps.h>

ES3C28P_GFX::ES3C28P_GFX() {
    // 1. Configure SPI Bus
    {
        auto cfg = _bus_instance.config();
        cfg.spi_host = TFT_SPI_HOST;
        cfg.spi_mode = 0;
        cfg.freq_write = 40000000; // 40MHz write
        cfg.freq_read  = 16000000;
        cfg.spi_3wire  = false;
        cfg.use_lock   = true;
        cfg.dma_channel = SPI_DMA_CH_AUTO;
        cfg.pin_sclk = TFT_PIN_SCLK;
        cfg.pin_mosi = TFT_PIN_MOSI;
        cfg.pin_miso = TFT_PIN_MISO;
        cfg.pin_dc   = TFT_PIN_DC;
        _bus_instance.config(cfg);
        _panel_instance.setBus(&_bus_instance);
    }

    // 2. Configure ILI9341 Display Panel
    {
        auto cfg = _panel_instance.config();
        cfg.pin_cs           = TFT_PIN_CS;
        cfg.pin_rst          = TFT_PIN_RST;
        cfg.pin_busy         = -1;
        cfg.memory_width     = 240;
        cfg.memory_height    = 320;
        cfg.panel_width      = 240;
        cfg.panel_height     = 320;
        cfg.offset_x         = 0;
        cfg.offset_y         = 0;
        cfg.offset_rotation  = 0;
        cfg.dummy_read_pixel = 8;
        cfg.dummy_read_bits  = 1;
        cfg.readable         = false;
        cfg.invert           = false;
        cfg.rgb_order        = false;
        cfg.dlen_16bit       = false;
        cfg.bus_shared       = false;
        _panel_instance.config(cfg);
    }

    // 3. Configure Backlight (PWM)
    {
        auto cfg = _light_instance.config();
        cfg.pin_bl = TFT_PIN_BL;
        cfg.invert = false;
        cfg.freq   = 44100;
        cfg.pwm_channel = 7;
        _light_instance.config(cfg);
        _panel_instance.setLight(&_light_instance);
    }

    // 4. Configure FT6336G Capacitive Touch
    {
        auto cfg = _touch_instance.config();
        cfg.x_min      = 0;
        cfg.x_max      = 239;
        cfg.y_min      = 0;
        cfg.y_max      = 319;
        cfg.pin_int    = TOUCH_PIN_INT;
        cfg.pin_rst    = TOUCH_PIN_RST;
        cfg.bus_shared = true; // Shares I2C with BBQ20KBD
        cfg.offset_rotation = 0;
        cfg.i2c_port   = 0;
        cfg.i2c_addr   = TOUCH_I2C_ADDR;
        cfg.pin_sda    = I2C_PIN_SDA;
        cfg.pin_scl    = I2C_PIN_SCL;
        cfg.freq       = 400000;
        _touch_instance.config(cfg);
        _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
}

DisplayHAL::DisplayHAL() {
}

void DisplayHAL::init() {
    Serial.println("[DisplayHAL] Initializing GFX display & touch...");
    _gfx.init();
    _gfx.setRotation(3); // 3 = Landscape 180-deg flipped (320x240)
    _gfx.setBrightness(_currentBrightness);
    _gfx.fillScreen(TFT_BLACK);

    // Initialize LVGL core
    lv_init();

    // Allocate draw buffers with DMA capability
    _buf1 = (lv_color_t*)heap_caps_malloc(BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!_buf1) {
        _buf1 = (lv_color_t*)malloc(BUF_SIZE * sizeof(lv_color_t));
    }
    _buf2 = (lv_color_t*)heap_caps_malloc(BUF_SIZE * sizeof(lv_color_t), MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL);
    if (!_buf2) {
        _buf2 = (lv_color_t*)malloc(BUF_SIZE * sizeof(lv_color_t));
    }

    lv_disp_draw_buf_init(&_draw_buf, _buf1, _buf2, BUF_SIZE);

    // Initialize display driver
    lv_disp_drv_init(&_disp_drv);
    _disp_drv.hor_res = SCREEN_WIDTH;
    _disp_drv.ver_res = SCREEN_HEIGHT;
    _disp_drv.flush_cb = lvglFlushCallback;
    _disp_drv.draw_buf = &_draw_buf;
    _disp_drv.user_data = this;
    lv_disp_drv_register(&_disp_drv);

    // Initialize touch input driver
    lv_indev_drv_init(&_indev_drv);
    _indev_drv.type = LV_INDEV_TYPE_POINTER;
    _indev_drv.read_cb = lvglTouchReadCallback;
    _indev_drv.user_data = this;
    _touch_indev = lv_indev_drv_register(&_indev_drv);

    // Optional: Visual touch reticle cursor on top-level system layer
    _touch_cursor = lv_obj_create(lv_layer_sys());
    lv_obj_remove_style_all(_touch_cursor);
    lv_obj_set_size(_touch_cursor, 14, 14);
    lv_obj_set_style_border_color(_touch_cursor, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(_touch_cursor, 2, 0);
    lv_obj_set_style_radius(_touch_cursor, LV_RADIUS_CIRCLE, 0);
    lv_obj_clear_flag(_touch_cursor, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(_touch_cursor, LV_OBJ_FLAG_HIDDEN);
    lv_indev_set_cursor(_touch_indev, _touch_cursor);

    Serial.printf("[DisplayHAL] LVGL initialized. Screen: %dx%d\n", SCREEN_WIDTH, SCREEN_HEIGHT);
}

void DisplayHAL::setBacklight(uint8_t brightness) {
    _currentBrightness = brightness;
    _gfx.setBrightness(brightness);
}

void DisplayHAL::update() {
    lv_timer_handler();
}

void DisplayHAL::lvglFlushCallback(lv_disp_drv_t* disp_drv, const lv_area_t* area, lv_color_t* color_p) {
    DisplayHAL* hal = (DisplayHAL*)disp_drv->user_data;
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    hal->_gfx.startWrite();
    hal->_gfx.setAddrWindow(area->x1, area->y1, w, h);
    hal->_gfx.writePixels((uint16_t*)&color_p->full, w * h, true);
    hal->_gfx.endWrite();

    lv_disp_flush_ready(disp_drv);
}

void DisplayHAL::lvglTouchReadCallback(lv_indev_drv_t* indev_drv, lv_indev_data_t* data) {
    DisplayHAL* hal = (DisplayHAL*)indev_drv->user_data;
    uint16_t rawX = 0, rawY = 0;
    bool touched = hal->_gfx.getTouch(&rawX, &rawY);

    if (touched) {
        data->state = LV_INDEV_STATE_PR;

        // Invert both axes to match physical landscape orientation (320x240)
        int32_t mappedX = (SCREEN_WIDTH - 1) - (int32_t)rawX;
        int32_t mappedY = (SCREEN_HEIGHT - 1) - (int32_t)rawY;

        if (mappedX < 0) mappedX = 0;
        if (mappedX >= SCREEN_WIDTH) mappedX = SCREEN_WIDTH - 1;
        if (mappedY < 0) mappedY = 0;
        if (mappedY >= SCREEN_HEIGHT) mappedY = SCREEN_HEIGHT - 1;

        data->point.x = mappedX;
        data->point.y = mappedY;

        if (hal->_touch_cursor) {
            lv_obj_clear_flag(hal->_touch_cursor, LV_OBJ_FLAG_HIDDEN);
        }

        static unsigned long lastLog = 0;
        if (millis() - lastLog > 200) {
            lastLog = millis();
            Serial.printf("[Touch] Raw: (%d, %d) -> Mapped: (%d, %d)\n", rawX, rawY, mappedX, mappedY);
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
        if (hal->_touch_cursor) {
            lv_obj_add_flag(hal->_touch_cursor, LV_OBJ_FLAG_HIDDEN);
        }
    }
}
