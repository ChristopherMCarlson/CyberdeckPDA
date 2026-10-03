# CyberDeck PDA Firmware (ESP32-S3 + ES3C28P + BBQ20KBD)

Minimalist, futuristic PDA firmware designed for the **ESP32-S3 (N16R8)** with a **2.8-inch capacitive touch screen (ILI9341V + FT6336G)** and a **Solderparty BBQ20KBD** I2C physical keyboard.

---

## 🖥️ Hardware Specification & Pin Mapping

| Peripheral | Controller | ESP32-S3 GPIO | Notes |
| :--- | :--- | :--- | :--- |
| **LCD CS** | ILI9341V | **GPIO 10** | Active Low |
| **LCD DC / RS** | ILI9341V | **GPIO 46** | Data / Command |
| **LCD SCK** | ILI9341V | **GPIO 12** | SPI2 Host (40MHz) |
| **LCD MOSI** | ILI9341V | **GPIO 11** | SPI Data In |
| **LCD MISO** | ILI9341V | **GPIO 13** | SPI Data Out |
| **LCD Backlight** | PWM | **GPIO 45** | Brightness controlled |
| **Touch SDA** | FT6336G | **GPIO 16** | Shared I2C Bus |
| **Touch SCL** | FT6336G | **GPIO 15** | Shared I2C Bus |
| **Touch INT** | FT6336G | **GPIO 17** | Interrupt |
| **Touch RST** | FT6336G | **GPIO 18** | Reset |
| **Keyboard SDA** | BBQ20KBD | **GPIO 16** | Shared 4-Pin I2C Port (0x1F) |
| **Keyboard SCL** | BBQ20KBD | **GPIO 15** | Shared 4-Pin I2C Port (0x1F) |

---

## ✨ Features Implemented

1. **Landscape UI Architecture (320x240)**:
   - Built on **LovyanGFX** + **LVGL v8.4.0** with DMA rendering.
   - CyberDeck aesthetic: Deep obsidian background (`#0B0D11`), electric cyan highlights (`#00E5FF`), amber warnings, and emerald indicators.

2. **Persistent Top Status Bar (22px)**:
   - Active Application indicator.
   - Real-time digital clock / system uptime.
   - Wi-Fi status indicator (`CONNECTED: SSID` / `CONNECTING...` / `OFFLINE`).
   - PSRAM / Free Memory badge.

3. **Home Screen Launcher**:
   - 2x2 App Grid with cyan glow focus indicators:
     - ⚙️ **SETTINGS**: Wi-Fi Scanner, Display, and Hardware Specs.
     - 💬 **GEMINI AI**: AI Chat Assistant (Architecture ready for Phase 2).
     - 🎮 **GAME BOY**: Retro GBC player (Planned for Phase 3).
     - 📝 **NOTES**: Quick scratchpad with direct BBQ20 keyboard text entry.
   - System Info Footer.

4. **Settings App**:
   - **Wi-Fi Scanner**: Non-blocking asynchronous Wi-Fi scanning with signal strength (dBm) and encryption indicators.
   - **Password Entry Modal**: Allows typing via both the physical **BBQ20KBD** and an on-screen LVGL keyboard, with show/hide password toggle.
   - **Persistent Storage**: Auto-saves credentials to ESP32 NVS (`Preferences`) and automatically connects on boot.
   - **Backlight Controls**: Independent sliders for Screen Brightness (PWM) and BBQ20 Keyboard Backlight (I2C register).
   - **System Specs**: Real-time readout of CPU clock, total and free 8MB PSRAM, internal heap, flash size, and MAC address.

5. **Solderparty BBQ20KBD Integration**:
   - Registered as an LVGL keypad input device.
   - Translates navigation keys (`Tab`, `Enter`, `Esc`, `Arrows`, `Backspace`) for menu navigation.
   - Types directly into active text inputs (Wi-Fi password, Notes).

---

## 🚀 How to Flash & Run

Ensure your board is connected via USB Type-C:

```bash
# In the project directory (e:\Code\CyberDeckPDA):
pio run --target upload

# To open serial monitor at 115200 baud:
pio run --target monitor
```
