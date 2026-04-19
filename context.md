# Adzan Clock ESP32 — Project Context

> This file captures all design decisions and technical context from the initial planning conversation.
> Feed this to an AI assistant when continuing work in a new session.

## Project Summary

An ESP32-based real-time clock and Islamic adzan (call to prayer) reminder for Singapore.
Displays time on a 32×16 LED dot matrix, fetches prayer times from the MUIS API, and plays
adzan audio through an external I2S DAC and amplifier.

## Hardware Components (Confirmed)

| Component | Exact Model | Interface | Power |
|---|---|---|---|
| MCU | ESP32 DevKit | — | 5V via USB |
| DAC | Adafruit UDA1334A | I2S | 3.3V |
| Amplifier | Adafruit MAX98306 | Analog in | 5V |
| Display | MAX7219 32×8 dot matrix × 2 (stacked = 32×16) | SPI daisy-chain | 5V |
| Speaker | 5W (4Ω or 8Ω) | — | — |
| Power | TP4056 USB-C module (charger only, no battery used) | — | 5V USB-C |
| Button | Tactile push button | GPIO 4 (INPUT_PULLUP) | — |

## Power Decision

- **No battery** — permanently connected to USB-C 5V via TP4056 module
- All components powered from USB 5V rail
- No boost converter needed

## Pin Assignments

| Function | GPIO |
|---|---|
| MAX7219 DIN (MOSI) | 23 |
| MAX7219 CLK (SCK) | 18 |
| MAX7219 CS | 5 |
| I2S BCLK | 27 |
| I2S LRC (WSEL) | 25 |
| I2S DOUT | 26 |
| Mode Button | 4 |

## Audio Signal Chain

```
ESP32 I2S → UDA1334A (DAC) → analog out → MAX98306 (Amp) → 5W Speaker
```

- Uses ESP8266Audio library for MP3 decoding
- Short adzan (~200KB MP3) and full adzan (~1.5MB MP3) stored in LittleFS
- Both fit within 2MB LittleFS partition (custom partition table, no OTA)

## Display Configuration

- Two MAX7219 32×8 modules stacked vertically = 32×16 pixel canvas
- 8 MAX7219 chips total (4 per module)
- MD_Parola library with 2 zones:
  - Zone 1 (top, devices 4–7): Static time display HH:MM with blinking colon
  - Zone 0 (bottom, devices 0–3): Scrolling next prayer info
- Hardware type: FC16_HW (most common module type — may need adjustment)

## MUIS Prayer Times API

```
GET https://api.prayertimes.sg/api/v1/prayer-times/today
```

### Sample Response (verified 2026-04-19)

```json
{
  "date": "2026-04-19",
  "day": "Sunday",
  "friendly_date": "19 April 2026",
  "hijri_date": "1 Zulkaedah 1447H",
  "times": {
    "subuh": "05:41",
    "syuruk": "07:00",
    "zohor": "13:05",
    "asar": "16:20",
    "maghrib": "19:09",
    "isyak": "20:19"
  },
  "times_ampm": {
    "subuh": "5:41 AM",
    "syuruk": "7:00 AM",
    "zohor": "1:05 PM",
    "asar": "4:20 PM",
    "maghrib": "7:09 PM",
    "isyak": "8:19 PM"
  }
}
```

- 5 prayer times used: subuh, zohor, asar, maghrib, isyak
- Syuruk (sunrise) is skipped — not a prayer time
- All 5 prayers trigger adzan
- Fetched once on boot + re-fetched at midnight

## WiFi Behaviour

- **STA mode** (default): Connects to saved WiFi → syncs NTP → fetches prayer times
- **AP mode** (fallback/config): Creates hotspot `AdzanClock` / `12345678` at `192.168.4.1`
- Button press toggles between modes
- Web server runs in both modes for configuration
- Credentials saved to `/config.json` in LittleFS

## Partition Table

```
nvs,      data, nvs,     0x9000,  0x5000
otadata,  data, ota,     0xe000,  0x2000
app0,     app,  ota_0,   0x10000, 0x1E0000   (~1.9MB firmware)
spiffs,   data, spiffs,  0x1F0000,0x210000   (~2.1MB LittleFS)
```

## Libraries Used

| Library | Version | Purpose |
|---|---|---|
| MD_Parola | ^3.7.3 | MAX7219 text effects + zones |
| MD_MAX72XX | ^3.5.1 | MAX7219 hardware driver |
| ArduinoJson | ^7.4.1 | JSON parsing (API + config) |
| ESPAsyncWebServer | ^1.2.4 | Non-blocking web server |
| AsyncTCP | ^1.1.1 | TCP for async web server |
| ESP8266Audio | ^1.9.9 | MP3 decoding + I2S output |

## Framework & Tooling

- **Framework**: Arduino (via PlatformIO)
- **IDE**: VSCode + PlatformIO extension
- **Upload**: USB serial
- **Filesystem upload**: `pio run --target uploadfs`

## Web Configuration UI

- Dark theme, mobile-friendly, served from LittleFS
- Features: WiFi setup, brightness slider (0–15), volume slider (0–100), prayer times table, test adzan button, restart button
- Polls `/api/status` every 2 seconds for live updates
- WiFi form uses **native HTML form POST** (not JavaScript fetch) — required for reliable AP mode operation
- Captive portal DNS redirects all queries to 192.168.4.1 in AP mode
- Handles OS connectivity checks (Apple CNA, Android generate_204, Windows fwlink)

## Known Items to Address Later

1. **MAX7219 hardware type** — FC16_HW assumed; may need PAROLA_HW, GENERIC_HW, or ICSTATION_HW depending on exact module
2. **Zone device mapping** — Bottom row = devices 0–3, top row = devices 4–7 assumed; may need swapping depending on wiring order
3. **Audio files** — User must provide `adzan_short.mp3` and/or `adzan_full.mp3` in `data/` folder
4. **TP4056 without battery** — Works as USB pass-through but has no power regulation; ensure downstream components handle 5V directly
5. **HTTPS on ESP32** — The MUIS API uses HTTPS; may need WiFiClientSecure with root CA cert or `setInsecure()` for testing

## Serial Debug Commands

Available via Serial Monitor at 115200 baud (type command + Enter):

| Command | Action |
|---|---|
| `time` | Toggle printing `[TIME] HH:MM:SS | Next: <prayer> HH:MM` every 5 seconds |
| `mode` | Toggle WiFi mode (AP ↔ STA), same as physical button |
| `status` | Print current status: mode, IP, time, sync state, next prayer, debug flags |
| `setnext HH:MM` | Override next upcoming prayer time temporarily (for testing adzan trigger) |
| `help` | Show available commands |

## Upload Notes

- **Firmware**: `pio run --target upload`
- **Filesystem (LittleFS)**: `pio run --target uploadfs`
- If upload fails with "Wrong boot mode detected (0x13)": hold the **BOOT** button on the ESP32 during upload, release when progress starts
