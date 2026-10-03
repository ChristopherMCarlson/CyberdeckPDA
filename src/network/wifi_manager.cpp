#include "network/wifi_manager.h"
#include <algorithm>

WiFiManager::WiFiManager() {
}

void WiFiManager::init() {
    Serial.println("[WiFiManager] Initializing WiFi subsystem...");
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(false);

    loadSavedCredentials();

    if (_savedSSID.length() > 0) {
        Serial.printf("[WiFiManager] Auto-connecting to saved network: %s\n", _savedSSID.c_str());
        connect(_savedSSID, _savedPass);
    } else {
        setState(WiFiState::DISCONNECTED);
    }
}

void WiFiManager::loadSavedCredentials() {
    _prefs.begin(PREF_WIFI_NAMESPACE, true);
    _savedSSID = _prefs.getString(PREF_KEY_SSID, "");
    _savedPass = _prefs.getString(PREF_KEY_PASS, "");
    _prefs.end();
}

void WiFiManager::saveCredentials(const String& ssid, const String& password) {
    _prefs.begin(PREF_WIFI_NAMESPACE, false);
    _prefs.putString(PREF_KEY_SSID, ssid);
    _prefs.putString(PREF_KEY_PASS, password);
    _prefs.end();
    _savedSSID = ssid;
    _savedPass = password;
}

void WiFiManager::forget() {
    disconnect();
    _prefs.begin(PREF_WIFI_NAMESPACE, false);
    _prefs.remove(PREF_KEY_SSID);
    _prefs.remove(PREF_KEY_PASS);
    _prefs.end();
    _savedSSID = "";
    _savedPass = "";
}

void WiFiManager::startScan() {
    if (_state == WiFiState::SCANNING) return;

    Serial.println("[WiFiManager] Starting async WiFi scan...");
    setState(WiFiState::SCANNING);
    _networks.clear();

    // true indicates async scan
    WiFi.scanNetworks(true, false, false, 300);
}

void WiFiManager::connect(const String& ssid, const String& password) {
    if (ssid.length() == 0) return;

    Serial.printf("[WiFiManager] Connecting to: %s\n", ssid.c_str());
    _savedSSID = ssid;
    _savedPass = password;

    WiFi.disconnect(false);
    delay(50);
    WiFi.begin(ssid.c_str(), password.c_str());

    _connectStartTime = millis();
    setState(WiFiState::CONNECTING);
}

void WiFiManager::disconnect() {
    WiFi.disconnect(true);
    setState(WiFiState::DISCONNECTED);
}

void WiFiManager::setState(WiFiState newState) {
    _state = newState;
    if (_stateCallback) {
        _stateCallback(_state);
    }
}

void WiFiManager::update() {
    // 1. Handle background scan completion
    if (_state == WiFiState::SCANNING) {
        int16_t scanStatus = WiFi.scanComplete();
        if (scanStatus >= 0) {
            Serial.printf("[WiFiManager] Scan complete. Found %d networks.\n", scanStatus);
            _networks.clear();

            for (int i = 0; i < scanStatus; ++i) {
                String foundSSID = WiFi.SSID(i);
                if (foundSSID.length() == 0) continue; // Skip hidden SSIDs

                // Check for duplicate SSIDs and keep stronger signal
                auto it = std::find_if(_networks.begin(), _networks.end(), [&](const WiFiNetworkInfo& item) {
                    return item.ssid == foundSSID;
                });

                if (it != _networks.end()) {
                    if (WiFi.RSSI(i) > it->rssi) {
                        it->rssi = WiFi.RSSI(i);
                        it->channel = WiFi.channel(i);
                        it->isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    }
                } else {
                    WiFiNetworkInfo info;
                    info.ssid = foundSSID;
                    info.rssi = WiFi.RSSI(i);
                    info.isEncrypted = (WiFi.encryptionType(i) != WIFI_AUTH_OPEN);
                    info.channel = WiFi.channel(i);
                    _networks.push_back(info);
                }
            }

            // Sort networks by signal strength descending
            std::sort(_networks.begin(), _networks.end(), [](const WiFiNetworkInfo& a, const WiFiNetworkInfo& b) {
                return a.rssi > b.rssi;
            });

            WiFi.scanDelete();
            setState(WiFi.isConnected() ? WiFiState::CONNECTED : WiFiState::DISCONNECTED);

            if (_scanCallback) {
                _scanCallback(_networks);
            }
        }
    }

    // 2. Handle connection timeout / success
    if (_state == WiFiState::CONNECTING) {
        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFiManager] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
            saveCredentials(_savedSSID, _savedPass);
            setState(WiFiState::CONNECTED);

            // Sync SNTP clock
            configTzTime(DEFAULT_TIMEZONE, NTP_SERVER_1, NTP_SERVER_2);
            Serial.println("[WiFiManager] Configured SNTP with NTP servers.");
        } else if (millis() - _connectStartTime > CONNECT_TIMEOUT_MS) {
            Serial.println("[WiFiManager] Connection timed out.");
            WiFi.disconnect();
            setState(WiFiState::CONNECT_FAILED);
        }
    }

    // 3. Monitor unexpected disconnects while connected
    if (_state == WiFiState::CONNECTED) {
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WiFiManager] Connection lost.");
            setState(WiFiState::DISCONNECTED);
        }
    }
}

String WiFiManager::getCurrentSSID() const {
    if (WiFi.isConnected()) {
        return WiFi.SSID();
    }
    return "";
}

String WiFiManager::getIPAddress() const {
    if (WiFi.isConnected()) {
        return WiFi.localIP().toString();
    }
    return "0.0.0.0";
}

int32_t WiFiManager::getRSSI() const {
    if (WiFi.isConnected()) {
        return WiFi.RSSI();
    }
    return 0;
}
