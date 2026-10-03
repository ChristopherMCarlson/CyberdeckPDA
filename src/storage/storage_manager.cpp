#include "storage/storage_manager.h"

bool StorageManager::init() {
    Serial.println("[StorageManager] Initializing storage subsystem...");
    if (mountSD()) {
        return true;
    }
    Serial.println("[StorageManager] SD card not detected or mount failed. Falling back to internal flash (SPIFFS)...");
    return mountSPIFFS();
}

bool StorageManager::mountSD() {
    Serial.println("[StorageManager] Configuring SDMMC 4-bit bus...");
    SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0, SD_PIN_D1, SD_PIN_D2, SD_PIN_D3);

    if (SD_MMC.begin("/sdcard", false, false, SDMMC_FREQ_DEFAULT, 5)) {
        uint8_t cardType = SD_MMC.cardType();
        if (cardType != CARD_NONE) {
            _fs = &SD_MMC;
            _isSD = true;
            _isReady = true;
            uint64_t totalMB = SD_MMC.totalBytes() / (1024 * 1024);
            uint64_t freeMB = (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
            Serial.printf("[StorageManager] MicroSD (4-bit) ready! Total: %llu MB, Free: %llu MB\n", totalMB, freeMB);
            ensureNotesDirectory();
            return true;
        }
    }

    Serial.println("[StorageManager] 4-bit mode failed, trying 1-bit SDMMC...");
    SD_MMC.setPins(SD_PIN_CLK, SD_PIN_CMD, SD_PIN_D0);
    if (SD_MMC.begin("/sdcard", true, false, SDMMC_FREQ_DEFAULT, 5)) {
        uint8_t cardType = SD_MMC.cardType();
        if (cardType != CARD_NONE) {
            _fs = &SD_MMC;
            _isSD = true;
            _isReady = true;
            uint64_t totalMB = SD_MMC.totalBytes() / (1024 * 1024);
            Serial.printf("[StorageManager] MicroSD (1-bit) ready! Total: %llu MB\n", totalMB);
            ensureNotesDirectory();
            return true;
        }
    }

    return false;
}

bool StorageManager::mountSPIFFS() {
    if (SPIFFS.begin(true)) {
        _fs = &SPIFFS;
        _isSD = false;
        _isReady = true;
        Serial.printf("[StorageManager] SPIFFS mounted. Total: %u KB, Used: %u KB\n",
                      SPIFFS.totalBytes() / 1024, SPIFFS.usedBytes() / 1024);
        ensureNotesDirectory();
        return true;
    }
    Serial.println("[StorageManager] Failed to mount SPIFFS.");
    _fs = nullptr;
    _isSD = false;
    _isReady = false;
    return false;
}

void StorageManager::ensureNotesDirectory() {
    if (!_fs || !_isReady) return;

    if (!_fs->exists("/notes")) {
        _fs->mkdir("/notes");
        Serial.println("[StorageManager] Created /notes directory");
    }

    // Create initial welcome note if empty
    std::vector<NoteInfo> notes = listNotes();
    if (notes.empty()) {
        saveNote("welcome.txt",
            "=== CYBERDECK PDA NOTES ===\n\n"
            "Features:\n"
            "- Multi-note storage on SD Card\n"
            "- Physical BBQ20 keyboard input\n"
            "- Trackpad navigation\n"
            "- Auto-save on exit\n\n"
            "Use the header buttons to create\n"
            "new notes or delete existing ones.\n");
        Serial.println("[StorageManager] Created default welcome.txt note");
    }
}

void StorageManager::refreshStorage() {
    if (!_isSD) {
        mountSD();
    }
}

String StorageManager::getStorageStatus() const {
    if (!_isReady || !_fs) {
        return "NO STORAGE READY";
    }
    if (_isSD) {
        uint64_t totalMB = SD_MMC.totalBytes() / (1024 * 1024);
        uint64_t freeMB = (SD_MMC.totalBytes() - SD_MMC.usedBytes()) / (1024 * 1024);
        char buf[64];
        snprintf(buf, sizeof(buf), "SD CARD: %llu MB FREE / %llu MB", freeMB, totalMB);
        return String(buf);
    } else {
        uint32_t freeKB = (SPIFFS.totalBytes() - SPIFFS.usedBytes()) / 1024;
        char buf[64];
        snprintf(buf, sizeof(buf), "INTERNAL: %u KB FREE", freeKB);
        return String(buf);
    }
}

uint64_t StorageManager::getTotalBytes() const {
    if (!_isReady || !_fs) return 0;
    return _isSD ? SD_MMC.totalBytes() : SPIFFS.totalBytes();
}

uint64_t StorageManager::getFreeBytes() const {
    if (!_isReady || !_fs) return 0;
    return _isSD ? (SD_MMC.totalBytes() - SD_MMC.usedBytes()) : (SPIFFS.totalBytes() - SPIFFS.usedBytes());
}

std::vector<NoteInfo> StorageManager::listNotes() {
    std::vector<NoteInfo> list;
    if (!_fs || !_isReady) return list;

    File dir = _fs->open("/notes");
    if (!dir || !dir.isDirectory()) {
        return list;
    }

    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory()) {
            String name = String(f.name());
            int slash = name.lastIndexOf('/');
            if (slash >= 0) {
                name = name.substring(slash + 1);
            }
            if (!name.startsWith(".")) {
                list.push_back({name, f.size()});
            }
        }
        f = dir.openNextFile();
    }
    dir.close();
    return list;
}

String StorageManager::loadNote(const String& filename) {
    if (!_fs || !_isReady) return "";

    String path = "/notes/" + filename;
    File f = _fs->open(path.c_str(), FILE_READ);
    if (!f) {
        Serial.printf("[StorageManager] Failed to open note for read: %s\n", path.c_str());
        return "";
    }

    String content = f.readString();
    f.close();
    return content;
}

bool StorageManager::saveNote(const String& filename, const String& content) {
    if (!_fs || !_isReady) return false;

    String cleanName = filename;
    if (!cleanName.endsWith(".txt") && !cleanName.endsWith(".md") && !cleanName.endsWith(".log")) {
        cleanName += ".txt";
    }

    String path = "/notes/" + cleanName;
    File f = _fs->open(path.c_str(), FILE_WRITE);
    if (!f) {
        Serial.printf("[StorageManager] Failed to open note for write: %s\n", path.c_str());
        return false;
    }

    f.print(content);
    f.close();
    Serial.printf("[StorageManager] Successfully saved note %s (%u bytes)\n", path.c_str(), content.length());
    return true;
}

bool StorageManager::deleteNote(const String& filename) {
    if (!_fs || !_isReady) return false;

    String path = "/notes/" + filename;
    bool ok = _fs->remove(path.c_str());
    Serial.printf("[StorageManager] Delete %s: %s\n", path.c_str(), ok ? "SUCCESS" : "FAILED");
    return ok;
}

bool StorageManager::noteExists(const String& filename) {
    if (!_fs || !_isReady) return false;
    String path = "/notes/" + filename;
    return _fs->exists(path.c_str());
}
