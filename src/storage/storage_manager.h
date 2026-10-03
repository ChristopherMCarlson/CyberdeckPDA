#pragma once

#include <Arduino.h>
#include <vector>
#include "FS.h"
#include "SD_MMC.h"
#include "SPIFFS.h"
#include "config.h"

struct NoteInfo {
    String filename;
    size_t size;
};

class StorageManager {
public:
    static StorageManager& getInstance() {
        static StorageManager instance;
        return instance;
    }

    bool init();
    bool isSD() const { return _isSD; }
    bool isReady() const { return _isReady; }
    String getStorageStatus() const;
    uint64_t getTotalBytes() const;
    uint64_t getFreeBytes() const;

    std::vector<NoteInfo> listNotes();
    String loadNote(const String& filename);
    bool saveNote(const String& filename, const String& content);
    bool deleteNote(const String& filename);
    bool noteExists(const String& filename);

    void refreshStorage();

private:
    StorageManager() = default;
    ~StorageManager() = default;

    bool mountSD();
    bool mountSPIFFS();
    void ensureNotesDirectory();

    fs::FS* _fs = nullptr;
    bool _isSD = false;
    bool _isReady = false;
};
