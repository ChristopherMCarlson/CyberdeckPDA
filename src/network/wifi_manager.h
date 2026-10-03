#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>
#include <vector>
#include <functional>
#include "config.h"

enum class WiFiState {
    DISCONNECTED,
    SCANNING,
    CONNECTING,
    CONNECTED,
    CONNECT_FAILED
};

struct WiFiNetworkInfo {
    String ssid;
    int32_t rssi;
    bool isEncrypted;
    uint8_t channel;
};

class WiFiManager {
public:
    using StateCallback = std::function<void(WiFiState state)>;
    using ScanCallback = std::function<void(const std::vector<WiFiNetworkInfo>& networks)>;

    static WiFiManager& getInstance() {
        static WiFiManager instance;
        return instance;
    }

    void init();
    void update();

    void startScan();
    bool isScanning() const { return _state == WiFiState::SCANNING; }
    const std::vector<WiFiNetworkInfo>& getNetworks() const { return _networks; }

    void connect(const String& ssid, const String& password);
    void disconnect();
    void forget();

    WiFiState getState() const { return _state; }
    String getSavedSSID() const { return _savedSSID; }
    String getCurrentSSID() const;
    String getIPAddress() const;
    int32_t getRSSI() const;

    void setStateCallback(StateCallback cb) { _stateCallback = cb; }
    void setScanCallback(ScanCallback cb) { _scanCallback = cb; }

private:
    WiFiManager();
    ~WiFiManager() = default;

    void setState(WiFiState newState);
    void checkConnectionStatus();
    void loadSavedCredentials();
    void saveCredentials(const String& ssid, const String& password);

    Preferences _prefs;
    WiFiState _state = WiFiState::DISCONNECTED;
    std::vector<WiFiNetworkInfo> _networks;

    String _savedSSID = "";
    String _savedPass = "";
    unsigned long _connectStartTime = 0;
    const unsigned long CONNECT_TIMEOUT_MS = 15000;

    StateCallback _stateCallback = nullptr;
    ScanCallback _scanCallback = nullptr;
};
