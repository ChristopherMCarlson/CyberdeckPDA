#pragma once

#include <Arduino.h>
#include <lvgl.h>
#include <vector>
#include "config.h"
#include "network/wifi_manager.h"

enum class CurrentScreen {
    HOME,
    SETTINGS,
    GEMINI_PREVIEW,
    NOTES
};

enum class NavDir {
    UP,
    DOWN,
    LEFT,
    RIGHT
};

class CyberUI {
public:
    static CyberUI& getInstance() {
        static CyberUI instance;
        return instance;
    }

    void init();
    void update();

    void showHomeScreen();
    void showSettingsScreen();
    void showGeminiPreview();
    void showNotesScreen();

    // WiFi callbacks to update UI in real-time
    void onWiFiStateChanged(WiFiState state);
    void onWiFiScanResults(const std::vector<WiFiNetworkInfo>& networks);

    // Global keyboard and trackpad shortcuts
    void handleBackKey();
    void handleNavigation(NavDir dir);

    lv_group_t* getHomeGroup() { return _homeGroup; }
    lv_group_t* getSettingsGroup() { return _settingsGroup; }

private:
    CyberUI();
    ~CyberUI() = default;

    void createStyles();
    void createStatusBar();
    void updateStatusBar();

    // Screen builders
    void buildHomeScreen();
    void buildSettingsScreen();
    void buildGeminiScreen();
    void buildNotesScreen();

    // WiFi Dialog
    void openPasswordDialog(const String& ssid);
    void closePasswordDialog();

    // LVGL objects
    lv_obj_t* _screenContainer = nullptr;
    lv_obj_t* _statusBar = nullptr;
    lv_obj_t* _statusTitle = nullptr;
    lv_obj_t* _statusClock = nullptr;
    lv_obj_t* _statusWiFi = nullptr;
    lv_obj_t* _statusRam = nullptr;

    // Screens
    lv_obj_t* _homeObj = nullptr;
    lv_obj_t* _settingsObj = nullptr;
    lv_obj_t* _geminiObj = nullptr;
    lv_obj_t* _notesObj = nullptr;

    // Settings screen elements
    lv_obj_t* _settingsBackBtn = nullptr;
    lv_obj_t* _tabview = nullptr;
    lv_obj_t* _tabBtns[3] = {nullptr, nullptr, nullptr};
    lv_obj_t* _tabLabels[3] = {nullptr, nullptr, nullptr};
    int _activeSettingsTab = 0;
    lv_obj_t* _wifiList = nullptr;
    lv_obj_t* _scanBtn = nullptr;
    lv_obj_t* _scanSpinner = nullptr;
    lv_obj_t* _wifiStatusLabel = nullptr;
    lv_obj_t* _screenBrightSlider = nullptr;
    lv_obj_t* _kbdBrightSlider = nullptr;
    lv_obj_t* _sysInfoLabel = nullptr;

    void selectSettingsTab(int idx);

    // Password Modal Dialog
    lv_obj_t* _pwdModal = nullptr;
    lv_obj_t* _pwdTa = nullptr;
    lv_obj_t* _pwdKb = nullptr;
    String _selectedSSID = "";

    // Styles
    lv_style_t _styleBase;
    lv_style_t _styleCard;
    lv_style_t _styleCardPressed;
    lv_style_t _styleCardFocus;
    lv_style_t _styleBtnPrimary;
    lv_style_t _styleBtnPressed;
    lv_style_t _styleBtnDanger;
    lv_style_t _styleTitle;
    lv_style_t _styleSubtitle;
    lv_style_t _styleBadge;
    lv_style_t _styleStatusBar;

    // State
    CurrentScreen _currentScreen = CurrentScreen::HOME;
    lv_group_t* _homeGroup = nullptr;
    lv_group_t* _settingsGroup = nullptr;
    lv_group_t* _geminiGroup = nullptr;
    lv_group_t* _notesGroup = nullptr;
    lv_group_t* _dialogGroup = nullptr;
    lv_obj_t* _firstHomeCard = nullptr;
    lv_obj_t* _homeCards[4] = {nullptr, nullptr, nullptr, nullptr};
    unsigned long _lastClockUpdate = 0;
    unsigned long _lastScreenChange = 0;
    lv_obj_t* _homeFooterDate = nullptr;
    bool _timeSynced = false;


    // Notes Application Elements
    enum class NotesViewMode { LIST, EDITOR };
    NotesViewMode _notesMode = NotesViewMode::LIST;
    String _currentNoteFilename = "";
    lv_obj_t* _notesListView = nullptr;
    lv_obj_t* _notesEditorView = nullptr;
    lv_obj_t* _notesListScroll = nullptr;
    lv_obj_t* _notesStorageLabel = nullptr;
    lv_obj_t* _notesEditorTitle = nullptr;
    lv_obj_t* _notesEditorTextArea = nullptr;
    lv_obj_t* _newNoteModal = nullptr;
    lv_obj_t* _newNoteTa = nullptr;
    lv_obj_t* _notesBackBtn = nullptr;
    lv_obj_t* _notesNewBtn = nullptr;
    lv_obj_t* _notesRefreshBtn = nullptr;
    lv_obj_t* _editorBackBtn = nullptr;
    lv_obj_t* _editorSaveBtn = nullptr;
    lv_obj_t* _editorDelBtn = nullptr;
    std::vector<String> _listedFilenames;


    void refreshNotesList();
    void openNoteEditor(const String& filename);
    void showNotesListView();
    void saveCurrentNote();
    void deleteCurrentNote();
    void openNewNoteDialog();
    void closeNewNoteDialog();

    // Static event dispatchers
    static void onAppCardClick(lv_event_t* e);
    static void onBackBtnClick(lv_event_t* e);
    static void onSettingsTabClick(lv_event_t* e);
    static void onScanBtnClick(lv_event_t* e);
    static void onNetworkItemClick(lv_event_t* e);
    static void onPwdConnectClick(lv_event_t* e);
    static void onPwdCancelClick(lv_event_t* e);
    static void onPwdEyeClick(lv_event_t* e);
    static void onDisconnectBtnClick(lv_event_t* e);
    static void onScreenBrightChange(lv_event_t* e);
    static void onKbdBrightChange(lv_event_t* e);

    // Notes event callbacks
    static void onNoteItemClick(lv_event_t* e);
    static void onNewNoteBtnClick(lv_event_t* e);
    static void onSaveNoteBtnClick(lv_event_t* e);
    static void onDeleteNoteBtnClick(lv_event_t* e);
    static void onBackToNotesListClick(lv_event_t* e);
    static void onRefreshNotesClick(lv_event_t* e);
    static void onNewNoteConfirmClick(lv_event_t* e);
    static void onNewNoteCancelClick(lv_event_t* e);
};

