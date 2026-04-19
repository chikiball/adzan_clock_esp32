# Adzan Clock ESP32 🕌

A real-time clock and adzan (Islamic call to prayer) reminder built with ESP32. Displays the current time on a MAX7219 LED dot matrix, syncs prayer times from the MUIS (Islamic Religious Council of Singapore) API, and plays adzan audio through an I2S DAC and amplifier.

## Features

- **Real-time clock** on 32×16 LED dot matrix (two 32×8 MAX7219 modules)
- **5 daily prayer reminders** — Subuh, Zohor, Asar, Maghrib, Isyak
- **Adzan audio playback** via I2S DAC (UDA1334A) and amplifier (MAX98306)
- **Automatic time sync** via NTP (Singapore timezone UTC+8)
- **MUIS API integration** — fetches daily prayer times from [prayertimes.sg](https://api.prayertimes.sg/api/v1/prayer-times/today)
- **Captive portal** with DNS redirect for reliable AP mode phone connectivity
- **Web-based configuration portal** — WiFi setup, volume, brightness control
- **Dual WiFi mode** — AP mode for setup, STA mode for operation
- **Hijri date display** from API response

## Hardware

| Component | Model | Interface |
|---|---|---|
| MCU | ESP32 DevKit | — |
| DAC | Adafruit UDA1334A | I2S |
| Amplifier | Adafruit MAX98306 | Analog |
| Display | MAX7219 32×8 dot matrix × 2 | SPI |
| Speaker | 5W (4Ω or 8Ω) | — |
| Power | USB-C 5V (TP4056 module) | — |
| Button | Tactile push button | GPIO 4 |

## Wiring

### MAX7219 (SPI) — Daisy-chained
| MAX7219 | ESP32 |
|---|---|
| VCC | 5V |
| GND | GND |
| DIN | GPIO 23 |
| CS | GPIO 5 |
| CLK | GPIO 18 |

### I2S Audio (ESP32 → UDA1334A)
| UDA1334A | ESP32 |
|---|---|
| WSEL | GPIO 25 |
| DIN | GPIO 26 |
| BCLK | GPIO 27 |
| VIN | 3.3V |
| GND | GND |

### UDA1334A → MAX98306
| UDA1334A | MAX98306 |
|---|---|
| L out | L+ in |
| GND | L− in |

### Button
| Pin | ESP32 |
|---|---|
| Leg 1 | GPIO 4 |
| Leg 2 | GND |

## Software Setup

### Prerequisites
- [VSCode](https://code.visualstudio.com/) + [PlatformIO](https://platformio.org/)
- USB cable for ESP32

### Build & Upload

```bash
# Clone
git clone https://github.com/YOUR_USERNAME/adzan_clock_esp32.git
cd adzan_clock_esp32

# Build firmware
pio run

# Upload firmware
pio run --target upload

# Upload filesystem (web UI + adzan audio)
pio run --target uploadfs
```

### Prepare Audio Files

Place your adzan MP3 files in the `data/` folder before uploading the filesystem:
- `data/adzan_short.mp3` — Short adzan (recommended, ~200KB)
- `data/adzan_full.mp3` — Full adzan (optional, ~1.5MB)

## Usage

1. **First boot** — ESP32 starts in AP mode
   - Connect to WiFi: `AdzanClock` (password: `12345678`)
   - Open `http://192.168.4.1` in your browser
   - Enter your home WiFi credentials and save

2. **Normal operation** — Connects to WiFi automatically
   - Syncs time via NTP
   - Fetches prayer times from MUIS API
   - Row 1: Current time (HH:MM with blinking colon)
   - Row 2: Next prayer name and time (scrolling)
   - Plays adzan at each prayer time

3. **Mode switch** — Press the button to toggle AP ↔ STA mode

4. **Web settings** — Access `http://<device-ip>` on your local network
   - Adjust volume and display brightness
   - View prayer times and status
   - Test adzan playback
   - Restart device

5. **Serial debug commands** — Open Serial Monitor at 115200 baud
   | Command | Action |
   |---|---|
   | `time` | Toggle printing current time every 5 seconds |
   | `mode` | Toggle WiFi mode (AP ↔ STA) |
   | `status` | Print current device status (mode, IP, time, sync, next prayer) |
   | `help` | Show available commands |

## Project Structure

```
adzan_clock_esp32/
├── platformio.ini          # PlatformIO configuration
├── partitions.csv          # Custom partition table (2MB LittleFS)
├── include/
│   ├── config.h            # Pin definitions & constants
│   ├── wifi_manager.h
│   ├── time_sync.h
│   ├── prayer_time.h
│   ├── display_manager.h
│   ├── audio_player.h
│   └── web_server.h
├── src/
│   ├── main.cpp            # Entry point & main loop
│   ├── wifi_manager.cpp
│   ├── time_sync.cpp
│   ├── prayer_time.cpp
│   ├── display_manager.cpp
│   ├── audio_player.cpp
│   └── web_server.cpp
├── data/                   # LittleFS filesystem
│   ├── index.html          # Web config UI
│   ├── style.css
│   ├── adzan_short.mp3     # (you provide)
│   └── adzan_full.mp3      # (you provide)
└── context.md              # Project context for AI assistants
```

## API Reference

The MUIS prayer times API:
```
GET https://api.prayertimes.sg/api/v1/prayer-times/today
```

Response:
```json
{
  "date": "2026-04-19",
  "hijri_date": "1 Zulkaedah 1447H",
  "times": {
    "subuh": "05:41",
    "syuruk": "07:00",
    "zohor": "13:05",
    "asar": "16:20",
    "maghrib": "19:09",
    "isyak": "20:19"
  }
}
```

## License

MIT
