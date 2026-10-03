#include "ui/ui.h"
#include "display/display_hal.h"
#include "input/keyboard_hal.h"
#include <esp_system.h>
#include <esp_heap_caps.h>

CyberUI::CyberUI() {
}

void CyberUI::init() {
    Serial.println("[CyberUI] Initializing UI System...");

    // Create dedicated input groups for clean screen-scoped navigation
    _homeGroup = lv_group_create();
    _settingsGroup = lv_group_create();
    _geminiGroup = lv_group_create();
    _notesGroup = lv_group_create();
    _dialogGroup = lv_group_create();

    KeyboardHAL::getInstance().setGroup(_homeGroup);

    // Create design styles
    createStyles();

    // Main screen container
    _screenContainer = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(_screenContainer);
    lv_obj_set_size(_screenContainer, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_add_style(_screenContainer, &_styleBase, 0);
    lv_obj_clear_flag(_screenContainer, LV_OBJ_FLAG_SCROLLABLE);

    // Build Status Bar
    createStatusBar();

    // Build Screen Views
    buildHomeScreen();
    buildSettingsScreen();
    buildGeminiScreen();
    buildNotesScreen();

    // Wire WiFi callbacks to UI updates
    WiFiManager::getInstance().setStateCallback([this](WiFiState state) {
        this->onWiFiStateChanged(state);
    });

    WiFiManager::getInstance().setScanCallback([this](const std::vector<WiFiNetworkInfo>& nets) {
        this->onWiFiScanResults(nets);
    });

    // Show initial home screen
    showHomeScreen();

    Serial.println("[CyberUI] UI System Ready.");
}

void CyberUI::createStyles() {
    // 1. Base dark theme background
    lv_style_init(&_styleBase);
    lv_style_set_bg_color(&_styleBase, lv_color_hex(0x0B0D11));
    lv_style_set_bg_opa(&_styleBase, LV_OPA_COVER);
    lv_style_set_text_color(&_styleBase, lv_color_hex(0xE6EDF3));

    // 2. Status Bar style
    lv_style_init(&_styleStatusBar);
    lv_style_set_bg_color(&_styleStatusBar, lv_color_hex(0x13171F));
    lv_style_set_bg_opa(&_styleStatusBar, LV_OPA_COVER);
    lv_style_set_border_color(&_styleStatusBar, lv_color_hex(0x232936));
    lv_style_set_border_width(&_styleStatusBar, 1);
    lv_style_set_border_side(&_styleStatusBar, LV_BORDER_SIDE_BOTTOM);
    lv_style_set_pad_hor(&_styleStatusBar, 6);
    lv_style_set_pad_ver(&_styleStatusBar, 2);

    // 3. Cyber Card style
    lv_style_init(&_styleCard);
    lv_style_set_bg_color(&_styleCard, lv_color_hex(0x151922));
    lv_style_set_bg_opa(&_styleCard, LV_OPA_COVER);
    lv_style_set_border_color(&_styleCard, lv_color_hex(0x252D3D));
    lv_style_set_border_width(&_styleCard, 1);
    lv_style_set_radius(&_styleCard, 6);
    lv_style_set_pad_all(&_styleCard, 8);
    lv_style_set_shadow_width(&_styleCard, 4);
    lv_style_set_shadow_color(&_styleCard, lv_color_hex(0x000000));
    lv_style_set_shadow_opa(&_styleCard, LV_OPA_40);

    // 4. Card Focus style (Electric Cyan glow for keyboard & trackpad navigation)
    lv_style_init(&_styleCardFocus);
    lv_style_set_border_color(&_styleCardFocus, lv_color_hex(0x00E5FF));
    lv_style_set_border_width(&_styleCardFocus, 3);
    lv_style_set_shadow_color(&_styleCardFocus, lv_color_hex(0x00E5FF));
    lv_style_set_shadow_width(&_styleCardFocus, 10);
    lv_style_set_shadow_opa(&_styleCardFocus, LV_OPA_80);
    lv_style_set_bg_color(&_styleCardFocus, lv_color_hex(0x002B36));

    // 3b. Card Pressed style
    lv_style_init(&_styleCardPressed);
    lv_style_set_bg_color(&_styleCardPressed, lv_color_hex(0x004052));
    lv_style_set_border_color(&_styleCardPressed, lv_color_hex(0x00E5FF));
    lv_style_set_border_width(&_styleCardPressed, 2);

    // 5. Primary button style (Electric Cyan)
    lv_style_init(&_styleBtnPrimary);
    lv_style_set_bg_color(&_styleBtnPrimary, lv_color_hex(0x0088A3));
    lv_style_set_bg_grad_color(&_styleBtnPrimary, lv_color_hex(0x005E73));
    lv_style_set_bg_grad_dir(&_styleBtnPrimary, LV_GRAD_DIR_VER);
    lv_style_set_border_color(&_styleBtnPrimary, lv_color_hex(0x00E5FF));
    lv_style_set_border_width(&_styleBtnPrimary, 1);
    lv_style_set_radius(&_styleBtnPrimary, 4);
    lv_style_set_text_color(&_styleBtnPrimary, lv_color_hex(0xFFFFFF));
    lv_style_set_pad_hor(&_styleBtnPrimary, 10);
    lv_style_set_pad_ver(&_styleBtnPrimary, 5);

    // 5b. Primary Button Pressed style
    lv_style_init(&_styleBtnPressed);
    lv_style_set_bg_color(&_styleBtnPressed, lv_color_hex(0x00B4D8));
    lv_style_set_border_color(&_styleBtnPressed, lv_color_hex(0xFFFFFF));
    lv_style_set_border_width(&_styleBtnPressed, 2);

    // 6. Danger button style (Red)
    lv_style_init(&_styleBtnDanger);
    lv_style_set_bg_color(&_styleBtnDanger, lv_color_hex(0x9E2A2B));
    lv_style_set_border_color(&_styleBtnDanger, lv_color_hex(0xFF5252));
    lv_style_set_border_width(&_styleBtnDanger, 1);
    lv_style_set_radius(&_styleBtnDanger, 4);
    lv_style_set_text_color(&_styleBtnDanger, lv_color_hex(0xFFFFFF));

    // 7. Title style
    lv_style_init(&_styleTitle);
    lv_style_set_text_font(&_styleTitle, &lv_font_montserrat_16);
    lv_style_set_text_color(&_styleTitle, lv_color_hex(0x00E5FF));

    // 8. Subtitle style
    lv_style_init(&_styleSubtitle);
    lv_style_set_text_font(&_styleSubtitle, &lv_font_montserrat_12);
    lv_style_set_text_color(&_styleSubtitle, lv_color_hex(0x8B949E));

    // 9. Badge style
    lv_style_init(&_styleBadge);
    lv_style_set_bg_color(&_styleBadge, lv_color_hex(0x1F2430));
    lv_style_set_radius(&_styleBadge, 3);
    lv_style_set_pad_hor(&_styleBadge, 4);
    lv_style_set_pad_ver(&_styleBadge, 2);
    lv_style_set_text_font(&_styleBadge, &lv_font_montserrat_12);
}

void CyberUI::createStatusBar() {
    _statusBar = lv_obj_create(_screenContainer);
    lv_obj_remove_style_all(_statusBar);
    lv_obj_set_size(_statusBar, SCREEN_WIDTH, 22);
    lv_obj_set_pos(_statusBar, 0, 0);
    lv_obj_add_style(_statusBar, &_styleStatusBar, 0);
    lv_obj_clear_flag(_statusBar, LV_OBJ_FLAG_SCROLLABLE);

    // Left: Device / App Title
    _statusTitle = lv_label_create(_statusBar);
    lv_label_set_text(_statusTitle, "CYBERDECK");
    lv_obj_set_style_text_color(_statusTitle, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_text_font(_statusTitle, &lv_font_montserrat_12, 0);
    lv_obj_align(_statusTitle, LV_ALIGN_LEFT_MID, 4, 0);

    // Center: Clock
    _statusClock = lv_label_create(_statusBar);
    lv_label_set_text(_statusClock, "00:00:00");
    lv_obj_set_style_text_color(_statusClock, lv_color_hex(0xE6EDF3), 0);
    lv_obj_set_style_text_font(_statusClock, &lv_font_montserrat_12, 0);
    lv_obj_align(_statusClock, LV_ALIGN_CENTER, 0, 0);

    // Right: WiFi Status & RAM badge
    _statusWiFi = lv_label_create(_statusBar);
    lv_label_set_text(_statusWiFi, LV_SYMBOL_WIFI " OFFLINE");
    lv_obj_set_style_text_color(_statusWiFi, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_font(_statusWiFi, &lv_font_montserrat_12, 0);
    lv_obj_align(_statusWiFi, LV_ALIGN_RIGHT_MID, -40, 0);

    _statusRam = lv_label_create(_statusBar);
    lv_label_set_text(_statusRam, "8M");
    lv_obj_set_style_text_color(_statusRam, lv_color_hex(0x00E676), 0);
    lv_obj_set_style_text_font(_statusRam, &lv_font_montserrat_12, 0);
    lv_obj_align(_statusRam, LV_ALIGN_RIGHT_MID, -4, 0);
}

void CyberUI::updateStatusBar() {
    // Update live clock / uptime
    unsigned long now = millis();
    if (now - _lastClockUpdate >= 1000) {
        _lastClockUpdate = now;
        uint32_t sec = now / 1000;
        uint32_t min = (sec / 60) % 60;
        uint32_t hr = (sec / 3600) % 24;
        sec = sec % 60;

        char timeBuf[16];
        snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u:%02u", hr, min, sec);
        lv_label_set_text(_statusClock, timeBuf);
    }

    // Update WiFi label
    WiFiState state = WiFiManager::getInstance().getState();
    if (state == WiFiState::CONNECTED) {
        lv_label_set_text_fmt(_statusWiFi, LV_SYMBOL_WIFI " %s", WiFiManager::getInstance().getCurrentSSID().c_str());
        lv_obj_set_style_text_color(_statusWiFi, lv_color_hex(0x00E676), 0);
    } else if (state == WiFiState::CONNECTING) {
        lv_label_set_text(_statusWiFi, LV_SYMBOL_LOOP " CONNECT...");
        lv_obj_set_style_text_color(_statusWiFi, lv_color_hex(0xFFB300), 0);
    } else {
        lv_label_set_text(_statusWiFi, LV_SYMBOL_WIFI " OFFLINE");
        lv_obj_set_style_text_color(_statusWiFi, lv_color_hex(0x8B949E), 0);
    }
}

void CyberUI::buildHomeScreen() {
    _homeObj = lv_obj_create(_screenContainer);
    lv_obj_remove_style_all(_homeObj);
    lv_obj_set_size(_homeObj, SCREEN_WIDTH, SCREEN_HEIGHT - 22);
    lv_obj_set_pos(_homeObj, 0, 22);
    lv_obj_clear_flag(_homeObj, LV_OBJ_FLAG_SCROLLABLE);

    // 2x2 Grid parameters for landscape 320x218
    const int cardW = 146;
    const int cardH = 76;
    const int col1X = 10;
    const int col2X = 164;
    const int row1Y = 8;
    const int row2Y = 90;

    // Helper lambda to create an App Card
    auto makeCard = [&](int x, int y, const char* symbol, const char* name, const char* desc, const char* badge, uint32_t badgeColor, int id) -> lv_obj_t* {
        lv_obj_t* card = lv_btn_create(_homeObj);
        lv_obj_remove_style_all(card);
        lv_obj_set_size(card, cardW, cardH);
        lv_obj_set_pos(card, x, y);
        lv_obj_add_style(card, &_styleCard, 0);
        lv_obj_add_style(card, &_styleCardPressed, LV_STATE_PRESSED);
        lv_obj_add_style(card, &_styleCardFocus, LV_STATE_FOCUSED);
        lv_obj_add_style(card, &_styleCardFocus, LV_STATE_FOCUS_KEY);
        lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

        // Icon + Title Row
        lv_obj_t* titleLbl = lv_label_create(card);
        lv_label_set_text_fmt(titleLbl, "%s %s", symbol, name);
        lv_obj_set_style_text_font(titleLbl, &lv_font_montserrat_14, 0);
        lv_obj_set_style_text_color(titleLbl, lv_color_hex(0x00E5FF), 0);
        lv_obj_align(titleLbl, LV_ALIGN_TOP_LEFT, 0, 0);
        lv_obj_clear_flag(titleLbl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(titleLbl, LV_OBJ_FLAG_EVENT_BUBBLE);

        // Subtitle / Description
        lv_obj_t* descLbl = lv_label_create(card);
        lv_label_set_text(descLbl, desc);
        lv_obj_add_style(descLbl, &_styleSubtitle, 0);
        lv_obj_align(descLbl, LV_ALIGN_LEFT_MID, 0, 6);
        lv_obj_clear_flag(descLbl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(descLbl, LV_OBJ_FLAG_EVENT_BUBBLE);

        // Status Badge (Bottom Right)
        lv_obj_t* badgeObj = lv_obj_create(card);
        lv_obj_remove_style_all(badgeObj);
        lv_obj_add_style(badgeObj, &_styleBadge, 0);
        lv_obj_set_style_text_color(badgeObj, lv_color_hex(badgeColor), 0);
        lv_obj_clear_flag(badgeObj, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_clear_flag(badgeObj, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_flag(badgeObj, LV_OBJ_FLAG_EVENT_BUBBLE);

        lv_obj_t* bLbl = lv_label_create(badgeObj);
        lv_label_set_text(bLbl, badge);
        lv_obj_clear_flag(bLbl, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_flag(bLbl, LV_OBJ_FLAG_EVENT_BUBBLE);
        lv_obj_align(badgeObj, LV_ALIGN_BOTTOM_RIGHT, 0, 0);

        lv_obj_add_event_cb(card, onAppCardClick, LV_EVENT_CLICKED, (void*)(intptr_t)id);
        lv_group_add_obj(_homeGroup, card);

        return card;
    };

    // 1. Settings App
    _firstHomeCard = makeCard(col1X, row1Y, LV_SYMBOL_SETTINGS, "SETTINGS", "WiFi & Hardware", "READY", 0x00E676, 1);

    // 2. Gemini AI App
    makeCard(col2X, row1Y, LV_SYMBOL_KEYBOARD, "GEMINI AI", "AI Assistant Chat", "PHASE 2", 0x00E5FF, 2);

    // 3. Game Boy App
    makeCard(col1X, row2Y, LV_SYMBOL_PLAY, "GAME BOY", "Retro GB/GBC", "PHASE 3", 0xFFB300, 3);

    // 4. Notes / Terminal App
    makeCard(col2X, row2Y, LV_SYMBOL_EDIT, "NOTES", "Scratchpad & Log", "ACTIVE", 0x8B949E, 4);

    // Set initial focus to Settings card
    if (_firstHomeCard) {
        lv_group_focus_obj(_firstHomeCard);
    }

    // Bottom Info Footer Banner
    lv_obj_t* footer = lv_obj_create(_homeObj);
    lv_obj_remove_style_all(footer);
    lv_obj_set_size(footer, SCREEN_WIDTH - 20, 30);
    lv_obj_set_pos(footer, 10, 174);
    lv_obj_add_style(footer, &_styleCard, 0);
    lv_obj_set_style_pad_all(footer, 4, 0);

    lv_obj_t* footerLbl = lv_label_create(footer);
    lv_label_set_text(footerLbl, "KBD: BBQ20KBD I2C | TOUCH: FT6336G | S3 N16R8");
    lv_obj_set_style_text_font(footerLbl, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(footerLbl, lv_color_hex(0x8B949E), 0);
    lv_obj_align(footerLbl, LV_ALIGN_CENTER, 0, 0);
}

void CyberUI::buildSettingsScreen() {
    _settingsObj = lv_obj_create(_screenContainer);
    lv_obj_remove_style_all(_settingsObj);
    lv_obj_set_size(_settingsObj, SCREEN_WIDTH, SCREEN_HEIGHT - 22);
    lv_obj_set_pos(_settingsObj, 0, 22);
    lv_obj_clear_flag(_settingsObj, LV_OBJ_FLAG_SCROLLABLE);

    // Top Header Row with Back Button
    lv_obj_t* header = lv_obj_create(_settingsObj);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, SCREEN_WIDTH, 28);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x13171F), 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x232936), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);

    // Back Button
    lv_obj_t* backBtn = lv_btn_create(header);
    lv_obj_remove_style_all(backBtn);
    lv_obj_add_style(backBtn, &_styleBtnPrimary, 0);
    lv_obj_add_style(backBtn, &_styleBtnPressed, LV_STATE_PRESSED);
    lv_obj_add_style(backBtn, &_styleCardFocus, LV_STATE_FOCUSED);
    lv_obj_add_style(backBtn, &_styleCardFocus, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(backBtn, 64, 22);
    lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_t* bLbl = lv_label_create(backBtn);
    lv_label_set_text(bLbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(bLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(bLbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(bLbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(bLbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(backBtn, onBackBtnClick, LV_EVENT_CLICKED, nullptr);
    lv_group_add_obj(_settingsGroup, backBtn);

    // Title
    lv_obj_t* sTitle = lv_label_create(header);
    lv_label_set_text(sTitle, "SETTINGS & HARDWARE");
    lv_obj_set_style_text_font(sTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(sTitle, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(sTitle, LV_ALIGN_CENTER, 0, 0);

    // Tabview for Settings sections
    lv_obj_t* tv = lv_tabview_create(_settingsObj, LV_DIR_TOP, 26);
    lv_obj_set_size(tv, SCREEN_WIDTH, SCREEN_HEIGHT - 50);
    lv_obj_set_pos(tv, 0, 28);
    lv_obj_set_style_bg_color(tv, lv_color_hex(0x0B0D11), 0);

    // Tab button styling
    lv_obj_t* tab_btns = lv_tabview_get_tab_btns(tv);
    lv_obj_set_style_bg_color(tab_btns, lv_color_hex(0x13171F), 0);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x8B949E), 0);
    lv_obj_set_style_text_color(tab_btns, lv_color_hex(0x00E5FF), LV_PART_ITEMS | LV_STATE_CHECKED);
    lv_obj_set_style_text_font(tab_btns, &lv_font_montserrat_12, 0);

    // --- TAB 1: Wi-Fi ---
    lv_obj_t* tabWifi = lv_tabview_add_tab(tv, "Wi-Fi");
    lv_obj_set_style_bg_color(tabWifi, lv_color_hex(0x0B0D11), 0);
    lv_obj_set_style_pad_all(tabWifi, 6, 0);

    // Control bar inside WiFi tab
    lv_obj_t* ctrlBar = lv_obj_create(tabWifi);
    lv_obj_remove_style_all(ctrlBar);
    lv_obj_set_size(ctrlBar, SCREEN_WIDTH - 20, 28);
    lv_obj_align(ctrlBar, LV_ALIGN_TOP_MID, 0, 0);

    // Scan Button
    _scanBtn = lv_btn_create(ctrlBar);
    lv_obj_remove_style_all(_scanBtn);
    lv_obj_add_style(_scanBtn, &_styleBtnPrimary, 0);
    lv_obj_add_style(_scanBtn, &_styleBtnPressed, LV_STATE_PRESSED);
    lv_obj_add_style(_scanBtn, &_styleCardFocus, LV_STATE_FOCUSED);
    lv_obj_add_style(_scanBtn, &_styleCardFocus, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(_scanBtn, 84, 24);
    lv_obj_align(_scanBtn, LV_ALIGN_LEFT_MID, 0, 0);
    lv_obj_t* scLbl = lv_label_create(_scanBtn);
    lv_label_set_text(scLbl, LV_SYMBOL_REFRESH " Scan");
    lv_obj_set_style_text_font(scLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(scLbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(scLbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(scLbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(_scanBtn, onScanBtnClick, LV_EVENT_CLICKED, nullptr);
    lv_group_add_obj(_settingsGroup, _scanBtn);

    // WiFi Status Banner Text
    _wifiStatusLabel = lv_label_create(ctrlBar);
    lv_label_set_text(_wifiStatusLabel, "Status: Disconnected");
    lv_obj_set_style_text_font(_wifiStatusLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_wifiStatusLabel, lv_color_hex(0x8B949E), 0);
    lv_obj_align(_wifiStatusLabel, LV_ALIGN_LEFT_MID, 94, 0);

    // Scrollable WiFi Network List
    _wifiList = lv_list_create(tabWifi);
    lv_obj_set_size(_wifiList, SCREEN_WIDTH - 20, 110);
    lv_obj_align(_wifiList, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_set_style_bg_color(_wifiList, lv_color_hex(0x13171F), 0);
    lv_obj_set_style_border_color(_wifiList, lv_color_hex(0x232936), 0);
    lv_obj_set_style_border_width(_wifiList, 1, 0);
    lv_obj_set_style_radius(_wifiList, 4, 0);
    lv_obj_set_style_pad_all(_wifiList, 4, 0);

    // Initial item in list
    lv_obj_t* initItem = lv_list_add_text(_wifiList, "Click [Scan] to search networks...");
    lv_obj_set_style_text_color(initItem, lv_color_hex(0x8B949E), 0);

    // --- TAB 2: Display & Backlight ---
    lv_obj_t* tabDisp = lv_tabview_add_tab(tv, "Display");
    lv_obj_set_style_bg_color(tabDisp, lv_color_hex(0x0B0D11), 0);
    lv_obj_set_style_pad_all(tabDisp, 10, 0);

    lv_obj_t* blLbl = lv_label_create(tabDisp);
    lv_label_set_text(blLbl, "Screen Brightness:");
    lv_obj_set_style_text_font(blLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(blLbl, LV_ALIGN_TOP_LEFT, 0, 4);

    _screenBrightSlider = lv_slider_create(tabDisp);
    lv_slider_set_range(_screenBrightSlider, 10, 255);
    lv_slider_set_value(_screenBrightSlider, DisplayHAL::getInstance().getBacklight(), LV_ANIM_OFF);
    lv_obj_set_size(_screenBrightSlider, 170, 16);
    lv_obj_align(_screenBrightSlider, LV_ALIGN_TOP_RIGHT, 0, 4);
    lv_obj_set_style_bg_color(_screenBrightSlider, lv_color_hex(0x00E5FF), LV_PART_INDICATOR);
    lv_obj_add_event_cb(_screenBrightSlider, onScreenBrightChange, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_group_add_obj(_settingsGroup, _screenBrightSlider);

    lv_obj_t* kbdLbl = lv_label_create(tabDisp);
    lv_label_set_text(kbdLbl, "BBQ20 Keyboard Backlight:");
    lv_obj_set_style_text_font(kbdLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(kbdLbl, LV_ALIGN_TOP_LEFT, 0, 42);

    _kbdBrightSlider = lv_slider_create(tabDisp);
    lv_slider_set_range(_kbdBrightSlider, 0, 255);
    lv_slider_set_value(_kbdBrightSlider, KeyboardHAL::getInstance().getBacklight(), LV_ANIM_OFF);
    lv_obj_set_size(_kbdBrightSlider, 170, 16);
    lv_obj_align(_kbdBrightSlider, LV_ALIGN_TOP_RIGHT, 0, 42);
    lv_obj_set_style_bg_color(_kbdBrightSlider, lv_color_hex(0xFFB300), LV_PART_INDICATOR);
    lv_obj_add_event_cb(_kbdBrightSlider, onKbdBrightChange, LV_EVENT_VALUE_CHANGED, nullptr);
    lv_group_add_obj(_settingsGroup, _kbdBrightSlider);

    // --- TAB 3: System Specs ---
    lv_obj_t* tabSys = lv_tabview_add_tab(tv, "System");
    lv_obj_set_style_bg_color(tabSys, lv_color_hex(0x0B0D11), 0);
    lv_obj_set_style_pad_all(tabSys, 10, 0);

    _sysInfoLabel = lv_label_create(tabSys);
    uint32_t freePsram = ESP.getFreePsram() / 1024;
    uint32_t totalPsram = ESP.getPsramSize() / (1024 * 1024);
    uint32_t freeHeap = ESP.getFreeHeap() / 1024;
    lv_label_set_text_fmt(_sysInfoLabel,
        "CPU: ESP32-S3 Dual-Core @ 240MHz\n"
        "PSRAM: %u MB Total (%u KB Free)\n"
        "Internal Heap: %u KB Free\n"
        "Display: 2.8\" ILI9341V (320x240)\n"
        "Touch: FT6336G Capacitive | KBD: BBQ20 I2C\n"
        "MAC: %s",
        totalPsram, freePsram, freeHeap, WiFi.macAddress().c_str()
    );
    lv_obj_set_style_text_font(_sysInfoLabel, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(_sysInfoLabel, lv_color_hex(0x8B949E), 0);
    lv_obj_align(_sysInfoLabel, LV_ALIGN_TOP_LEFT, 0, 0);
}

void CyberUI::buildGeminiScreen() {
    _geminiObj = lv_obj_create(_screenContainer);
    lv_obj_remove_style_all(_geminiObj);
    lv_obj_set_size(_geminiObj, SCREEN_WIDTH, SCREEN_HEIGHT - 22);
    lv_obj_set_pos(_geminiObj, 0, 22);
    lv_obj_clear_flag(_geminiObj, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t* header = lv_obj_create(_geminiObj);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, SCREEN_WIDTH, 28);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x13171F), 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x232936), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);

    lv_obj_t* backBtn = lv_btn_create(header);
    lv_obj_remove_style_all(backBtn);
    lv_obj_add_style(backBtn, &_styleBtnPrimary, 0);
    lv_obj_add_style(backBtn, &_styleCardFocus, LV_STATE_FOCUSED);
    lv_obj_add_style(backBtn, &_styleCardFocus, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(backBtn, 64, 22);
    lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_t* bLbl = lv_label_create(backBtn);
    lv_label_set_text(bLbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(bLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(bLbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(backBtn, onBackBtnClick, LV_EVENT_CLICKED, nullptr);
    lv_group_add_obj(_geminiGroup, backBtn);

    lv_obj_t* gTitle = lv_label_create(header);
    lv_label_set_text(gTitle, "GOOGLE GEMINI AI");
    lv_obj_set_style_text_font(gTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(gTitle, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(gTitle, LV_ALIGN_CENTER, 0, 0);

    // Terminal preview box
    lv_obj_t* termBox = lv_obj_create(_geminiObj);
    lv_obj_remove_style_all(termBox);
    lv_obj_set_size(termBox, SCREEN_WIDTH - 20, 160);
    lv_obj_set_pos(termBox, 10, 38);
    lv_obj_add_style(termBox, &_styleCard, 0);

    lv_obj_t* termText = lv_label_create(termBox);
    lv_label_set_text(termText,
        "> GEMINI CLIENT READY\n\n"
        "Connect Wi-Fi in Settings first.\n\n"
        "In the next step, we will wire up the\n"
        "Gemini API chat pipeline with text streaming\n"
        "and BBQ20 physical keyboard input prompt!"
    );
    lv_obj_set_style_text_font(termText, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(termText, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(termText, LV_ALIGN_TOP_LEFT, 4, 4);
}

void CyberUI::buildNotesScreen() {
    _notesObj = lv_obj_create(_screenContainer);
    lv_obj_remove_style_all(_notesObj);
    lv_obj_set_size(_notesObj, SCREEN_WIDTH, SCREEN_HEIGHT - 22);
    lv_obj_set_pos(_notesObj, 0, 22);
    lv_obj_clear_flag(_notesObj, LV_OBJ_FLAG_SCROLLABLE);

    // Header
    lv_obj_t* header = lv_obj_create(_notesObj);
    lv_obj_remove_style_all(header);
    lv_obj_set_size(header, SCREEN_WIDTH, 28);
    lv_obj_set_pos(header, 0, 0);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x13171F), 0);
    lv_obj_set_style_border_color(header, lv_color_hex(0x232936), 0);
    lv_obj_set_style_border_width(header, 1, 0);
    lv_obj_set_style_border_side(header, LV_BORDER_SIDE_BOTTOM, 0);

    lv_obj_t* backBtn = lv_btn_create(header);
    lv_obj_remove_style_all(backBtn);
    lv_obj_add_style(backBtn, &_styleBtnPrimary, 0);
    lv_obj_add_style(backBtn, &_styleCardFocus, LV_STATE_FOCUSED);
    lv_obj_add_style(backBtn, &_styleCardFocus, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(backBtn, 64, 22);
    lv_obj_align(backBtn, LV_ALIGN_LEFT_MID, 6, 0);
    lv_obj_t* bLbl = lv_label_create(backBtn);
    lv_label_set_text(bLbl, LV_SYMBOL_LEFT " Back");
    lv_obj_set_style_text_font(bLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(bLbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(backBtn, onBackBtnClick, LV_EVENT_CLICKED, nullptr);
    lv_group_add_obj(_notesGroup, backBtn);

    lv_obj_t* nTitle = lv_label_create(header);
    lv_label_set_text(nTitle, "QUICK NOTES & LOG");
    lv_obj_set_style_text_font(nTitle, &lv_font_montserrat_14, 0);
    lv_obj_set_style_text_color(nTitle, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(nTitle, LV_ALIGN_CENTER, 0, 0);

    // Text Area for testing BBQ20 keyboard typing directly
    lv_obj_t* ta = lv_textarea_create(_notesObj);
    lv_obj_set_size(ta, SCREEN_WIDTH - 20, 160);
    lv_obj_set_pos(ta, 10, 38);
    lv_obj_set_style_bg_color(ta, lv_color_hex(0x13171F), 0);
    lv_obj_set_style_text_color(ta, lv_color_hex(0xE6EDF3), 0);
    lv_obj_set_style_border_color(ta, lv_color_hex(0x232936), 0);
    lv_textarea_set_placeholder_text(ta, "Type here using the BBQ20 physical keyboard...");
    lv_group_add_obj(_notesGroup, ta);
}

void CyberUI::showHomeScreen() {
    _lastScreenChange = millis();
    _currentScreen = CurrentScreen::HOME;
    lv_obj_clear_flag(_homeObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_settingsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_geminiObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_notesObj, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(_statusTitle, "CYBERDECK");

    KeyboardHAL::getInstance().setGroup(_homeGroup);
    if (_firstHomeCard) {
        lv_group_focus_obj(_firstHomeCard);
    }
}

void CyberUI::showSettingsScreen() {
    _lastScreenChange = millis();
    _currentScreen = CurrentScreen::SETTINGS;
    lv_obj_add_flag(_homeObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_settingsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_geminiObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_notesObj, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(_statusTitle, "SETTINGS");

    KeyboardHAL::getInstance().setGroup(_settingsGroup);
    if (_scanBtn) {
        lv_group_focus_obj(_scanBtn);
    }

    // Automatically trigger a WiFi scan when opening settings if not connected
    if (WiFiManager::getInstance().getState() == WiFiState::DISCONNECTED) {
        WiFiManager::getInstance().startScan();
    }
}

void CyberUI::showGeminiPreview() {
    _lastScreenChange = millis();
    _currentScreen = CurrentScreen::GEMINI_PREVIEW;
    lv_obj_add_flag(_homeObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_settingsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_geminiObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_notesObj, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(_statusTitle, "GEMINI AI");

    KeyboardHAL::getInstance().setGroup(_geminiGroup);
}

void CyberUI::showNotesScreen() {
    _lastScreenChange = millis();
    _currentScreen = CurrentScreen::NOTES;
    lv_obj_add_flag(_homeObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_settingsObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(_geminiObj, LV_OBJ_FLAG_HIDDEN);
    lv_obj_clear_flag(_notesObj, LV_OBJ_FLAG_HIDDEN);
    lv_label_set_text(_statusTitle, "NOTES");

    KeyboardHAL::getInstance().setGroup(_notesGroup);
}

void CyberUI::handleBackKey() {
    if (millis() - _lastScreenChange < 400) {
        Serial.println("[CyberUI] Back key ignored (debounce)");
        return;
    }
    Serial.println("[CyberUI] Back key handled");
    if (_pwdModal) {
        closePasswordDialog();
        return;
    }
    if (_currentScreen != CurrentScreen::HOME) {
        showHomeScreen();
        return;
    } else {
        if (_firstHomeCard && _homeGroup) {
            lv_group_focus_obj(_firstHomeCard);
        }
    }
}

void CyberUI::update() {
    updateStatusBar();
}

void CyberUI::onWiFiStateChanged(WiFiState state) {
    if (!_wifiStatusLabel) return;

    switch (state) {
        case WiFiState::CONNECTED:
            lv_label_set_text_fmt(_wifiStatusLabel, "Connected: %s (%s)",
                WiFiManager::getInstance().getCurrentSSID().c_str(),
                WiFiManager::getInstance().getIPAddress().c_str()
            );
            lv_obj_set_style_text_color(_wifiStatusLabel, lv_color_hex(0x00E676), 0);
            break;
        case WiFiState::CONNECTING:
            lv_label_set_text(_wifiStatusLabel, "Connecting to network...");
            lv_obj_set_style_text_color(_wifiStatusLabel, lv_color_hex(0xFFB300), 0);
            break;
        case WiFiState::SCANNING:
            lv_label_set_text(_wifiStatusLabel, "Scanning networks...");
            lv_obj_set_style_text_color(_wifiStatusLabel, lv_color_hex(0x00E5FF), 0);
            break;
        case WiFiState::CONNECT_FAILED:
            lv_label_set_text(_wifiStatusLabel, "Connection Failed!");
            lv_obj_set_style_text_color(_wifiStatusLabel, lv_color_hex(0xFF5252), 0);
            break;
        case WiFiState::DISCONNECTED:
        default:
            lv_label_set_text(_wifiStatusLabel, "Disconnected");
            lv_obj_set_style_text_color(_wifiStatusLabel, lv_color_hex(0x8B949E), 0);
            break;
    }
}

void CyberUI::onWiFiScanResults(const std::vector<WiFiNetworkInfo>& networks) {
    if (!_wifiList) return;

    // Clear old list items
    lv_obj_clean(_wifiList);

    if (networks.empty()) {
        lv_obj_t* item = lv_list_add_text(_wifiList, "No networks found. Tap Scan again.");
        lv_obj_set_style_text_color(item, lv_color_hex(0x8B949E), 0);
        return;
    }

    for (const auto& net : networks) {
        const char* icon = LV_SYMBOL_WIFI;
        char itemText[64];
        snprintf(itemText, sizeof(itemText), "%s %s (%d dBm)", net.ssid.c_str(), net.isEncrypted ? "[*]" : "[OPEN]", net.rssi);

        lv_obj_t* btn = lv_list_add_btn(_wifiList, icon, itemText);
        lv_obj_set_style_text_font(btn, &lv_font_montserrat_12, 0);
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x181C26), 0);
        lv_obj_set_style_text_color(btn, lv_color_hex(0xE6EDF3), 0);

        // Highlight currently connected SSID with green border
        if (WiFiManager::getInstance().getState() == WiFiState::CONNECTED &&
            WiFiManager::getInstance().getCurrentSSID() == net.ssid) {
            lv_obj_set_style_border_color(btn, lv_color_hex(0x00E676), 0);
            lv_obj_set_style_border_width(btn, 1, 0);
        }

        // Store SSID in user data via allocated string copy or event param
        char* ssidCopy = strdup(net.ssid.c_str());
        lv_obj_add_style(btn, &_styleCardFocus, LV_STATE_FOCUSED);
        lv_obj_add_style(btn, &_styleCardFocus, LV_STATE_FOCUS_KEY);
        lv_obj_add_event_cb(btn, onNetworkItemClick, LV_EVENT_CLICKED, (void*)ssidCopy);
        lv_group_add_obj(_settingsGroup, btn);
    }
}

void CyberUI::openPasswordDialog(const String& ssid) {
    _selectedSSID = ssid;

    // Modal background overlay
    _pwdModal = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(_pwdModal);
    lv_obj_set_size(_pwdModal, SCREEN_WIDTH, SCREEN_HEIGHT);
    lv_obj_set_style_bg_color(_pwdModal, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(_pwdModal, LV_OPA_70, 0);

    // Dialog card
    lv_obj_t* dialog = lv_obj_create(_pwdModal);
    lv_obj_remove_style_all(dialog);
    lv_obj_set_size(dialog, 280, 150);
    lv_obj_align(dialog, LV_ALIGN_CENTER, 0, -20);
    lv_obj_add_style(dialog, &_styleCard, 0);
    lv_obj_set_style_border_color(dialog, lv_color_hex(0x00E5FF), 0);
    lv_obj_set_style_border_width(dialog, 1, 0);

    // Title
    lv_obj_t* title = lv_label_create(dialog);
    lv_label_set_text_fmt(title, "Connect to: %s", ssid.c_str());
    lv_obj_set_style_text_font(title, &lv_font_montserrat_12, 0);
    lv_obj_set_style_text_color(title, lv_color_hex(0x00E5FF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 0, 0);

    // Password Text Area
    _pwdTa = lv_textarea_create(dialog);
    lv_obj_set_size(_pwdTa, 220, 34);
    lv_obj_align(_pwdTa, LV_ALIGN_TOP_LEFT, 0, 26);
    lv_textarea_set_password_mode(_pwdTa, true);
    lv_textarea_set_one_line(_pwdTa, true);
    lv_textarea_set_placeholder_text(_pwdTa, "Enter Wi-Fi Password");
    lv_obj_set_style_bg_color(_pwdTa, lv_color_hex(0x101319), 0);
    lv_obj_set_style_text_color(_pwdTa, lv_color_hex(0xE6EDF3), 0);
    lv_obj_set_style_border_color(_pwdTa, lv_color_hex(0x232936), 0);

    // Eye toggle button (Show/Hide password)
    lv_obj_t* eyeBtn = lv_btn_create(dialog);
    lv_obj_remove_style_all(eyeBtn);
    lv_obj_add_style(eyeBtn, &_styleBtnPrimary, 0);
    lv_obj_set_size(eyeBtn, 34, 34);
    lv_obj_align(eyeBtn, LV_ALIGN_TOP_RIGHT, 0, 26);
    lv_obj_t* eyeLbl = lv_label_create(eyeBtn);
    lv_label_set_text(eyeLbl, LV_SYMBOL_EYE_OPEN);
    lv_obj_align(eyeLbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(eyeBtn, onPwdEyeClick, LV_EVENT_CLICKED, nullptr);

    // Cancel Button
    lv_obj_t* cancelBtn = lv_btn_create(dialog);
    lv_obj_remove_style_all(cancelBtn);
    lv_obj_add_style(cancelBtn, &_styleBtnDanger, 0);
    lv_obj_add_style(cancelBtn, &_styleCardFocus, LV_STATE_FOCUSED);
    lv_obj_add_style(cancelBtn, &_styleCardFocus, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(cancelBtn, 90, 30);
    lv_obj_align(cancelBtn, LV_ALIGN_BOTTOM_LEFT, 0, 0);
    lv_obj_t* cLbl = lv_label_create(cancelBtn);
    lv_label_set_text(cLbl, "Cancel");
    lv_obj_set_style_text_font(cLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(cLbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(cLbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cLbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(cancelBtn, onPwdCancelClick, LV_EVENT_CLICKED, nullptr);

    // Connect Button
    lv_obj_t* connBtn = lv_btn_create(dialog);
    lv_obj_remove_style_all(connBtn);
    lv_obj_add_style(connBtn, &_styleBtnPrimary, 0);
    lv_obj_add_style(connBtn, &_styleCardFocus, LV_STATE_FOCUSED);
    lv_obj_add_style(connBtn, &_styleCardFocus, LV_STATE_FOCUS_KEY);
    lv_obj_set_size(connBtn, 110, 30);
    lv_obj_align(connBtn, LV_ALIGN_BOTTOM_RIGHT, 0, 0);
    lv_obj_t* cnLbl = lv_label_create(connBtn);
    lv_label_set_text(cnLbl, LV_SYMBOL_OK " Connect");
    lv_obj_set_style_text_font(cnLbl, &lv_font_montserrat_12, 0);
    lv_obj_align(cnLbl, LV_ALIGN_CENTER, 0, 0);
    lv_obj_clear_flag(cnLbl, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_flag(cnLbl, LV_OBJ_FLAG_EVENT_BUBBLE);
    lv_obj_add_event_cb(connBtn, onPwdConnectClick, LV_EVENT_CLICKED, nullptr);

    // Set dialog group for modal typing
    lv_group_remove_all_objs(_dialogGroup);
    lv_group_add_obj(_dialogGroup, _pwdTa);
    lv_group_add_obj(_dialogGroup, connBtn);
    lv_group_add_obj(_dialogGroup, cancelBtn);
    KeyboardHAL::getInstance().setGroup(_dialogGroup);
    lv_group_focus_obj(_pwdTa);

    // On-Screen Virtual Keyboard
    _pwdKb = lv_keyboard_create(_pwdModal);
    lv_obj_set_size(_pwdKb, SCREEN_WIDTH, 100);
    lv_obj_align(_pwdKb, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_keyboard_set_textarea(_pwdKb, _pwdTa);
    lv_obj_set_style_bg_color(_pwdKb, lv_color_hex(0x13171F), 0);
}

void CyberUI::closePasswordDialog() {
    if (_pwdModal) {
        lv_obj_del(_pwdModal);
        _pwdModal = nullptr;
        _pwdTa = nullptr;
        _pwdKb = nullptr;
        KeyboardHAL::getInstance().setGroup(_settingsGroup);
        if (_scanBtn) {
            lv_group_focus_obj(_scanBtn);
        }
    }
}

// ==========================================
// Static Event Handlers
// ==========================================
void CyberUI::onAppCardClick(lv_event_t* e) {
    int id = (int)(intptr_t)lv_event_get_user_data(e);
    CyberUI& ui = CyberUI::getInstance();

    switch (id) {
        case 1:
            ui.showSettingsScreen();
            break;
        case 2:
            ui.showGeminiPreview();
            break;
        case 3:
            // Game Boy coming soon
            ui.showNotesScreen();
            break;
        case 4:
            ui.showNotesScreen();
            break;
    }
}

void CyberUI::onBackBtnClick(lv_event_t* e) {
    if (millis() - CyberUI::getInstance()._lastScreenChange < 400) {
        Serial.println("[CyberUI] onBackBtnClick ignored (screen debounce)");
        return;
    }
    CyberUI::getInstance().showHomeScreen();
}

void CyberUI::onScanBtnClick(lv_event_t* e) {
    WiFiManager::getInstance().startScan();
}

void CyberUI::onNetworkItemClick(lv_event_t* e) {
    char* ssid = (char*)lv_event_get_user_data(e);
    if (ssid) {
        CyberUI::getInstance().openPasswordDialog(String(ssid));
    }
}

void CyberUI::onPwdConnectClick(lv_event_t* e) {
    CyberUI& ui = CyberUI::getInstance();
    if (ui._pwdTa) {
        const char* pass = lv_textarea_get_text(ui._pwdTa);
        WiFiManager::getInstance().connect(ui._selectedSSID, String(pass));
    }
    ui.closePasswordDialog();
}

void CyberUI::onPwdCancelClick(lv_event_t* e) {
    CyberUI::getInstance().closePasswordDialog();
}

void CyberUI::onPwdEyeClick(lv_event_t* e) {
    CyberUI& ui = CyberUI::getInstance();
    if (ui._pwdTa) {
        bool current = lv_textarea_get_password_mode(ui._pwdTa);
        lv_textarea_set_password_mode(ui._pwdTa, !current);
    }
}

void CyberUI::onScreenBrightChange(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    DisplayHAL::getInstance().setBacklight((uint8_t)val);
}

void CyberUI::onKbdBrightChange(lv_event_t* e) {
    lv_obj_t* slider = lv_event_get_target(e);
    int val = lv_slider_get_value(slider);
    KeyboardHAL::getInstance().setBacklight((uint8_t)val);
}
