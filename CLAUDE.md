# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Jetson Doorbell is an ESP32-S3-N4 firmware project for a WiFi-connected smart doorbell that plays WAV files from an SD card. It supports two doorbell buttons with independently configurable tunes and volumes, managed via a browser-based UI.

- **MCU**: ESP32-S3-N4 running at 240MHz
- **Toolchain**: ESP-IDF 5.2.1
- **Web server**: Mongoose 7.13
- **Audio amp**: MAX98357A (controlled via SD_MODE_PIN, open-drain)

## Build Commands

Standard ESP-IDF build (requires ESP-IDF environment to be set up):
```sh
idf.py build
idf.py flash
idf.py monitor
```

Docker-based build (Makefile wrapper):
```sh
make build             # build using Docker espressif/idf image
make flash PORT=/dev/ttyUSBx  # build and flash via Docker
```

Alternative flash methods are in the Makefile (`flash2` using `esputil`, `flash3` using `esptool.py`).

To enable hardware profiling via logic analyzer outputs, define `HW_PROFILE` — this activates GPIO outputs on pins 4–7 (LA0–LA3) to time SD reads and I2S writes.

## Architecture

### Startup sequence (`main/main.c` → `app_main`)
1. `setup_gpio()` — configures all GPIOs
2. `sd_setup()` — mounts SD card via SDMMC (4-bit, up to 40MHz); error blinks 1
3. `i2s_setup()` — configures I2S DMA channel (2 descriptors, 4092-byte buffers)
4. `restore_conf()` — reads SD card `/conf/` files into global state; error blinks 2
5. Creates three FreeRTOS tasks: `dma_buffer_fill_task`, `button1_play_task`, `button2_play_task`
6. `wifi_init()` — connects to WiFi using credentials from SD card; error blinks 3
7. Starts Mongoose HTTP server on port 8000; polls forever

### Audio playback flow
- **Button press** → GPIO falling-edge ISR (`isr.c`) → `vTaskNotifyGiveFromISR` to button task + disables that button's interrupt
- **Button task** (`task.c`) → takes `xPlay` semaphore (mutual exclusion, 30s timeout) → calls `open_file()`
- **`open_file()`** (`sd.c`) → parses WAV header (RIFF/WAVE, PCM-only, 16-bit, ≤48kHz, mono or stereo), reconfigures I2S sample rate, preloads 2 DMA buffers, enables MAX98357A, enables I2S channel
- **I2S DMA sent callback** (`isr.c: i2s_write_callback`) → `vTaskNotifyGiveFromISR` to `dma_buffer_fill_task`
- **`dma_buffer_fill_task`** (`task.c`) → reads next chunk from SD, applies volume (linear scalar, converted from dB), writes to I2S; handles EOF by zeroing final buffer, then disables I2S and MAX98357A, re-enables button interrupts, gives `xPlay` semaphore

Mono WAV files are duplicated into both stereo channels in software. Volume is stored in dB on the SD card and converted to a linear multiplier (`pow(10, dB/20)`) applied to each int16 sample.

### Global state (`main/global.c` + `main/global.h`)
Single shared DMA buffer (`buff[]` / `buffer`) protected by the `xPlay` semaphore. Key globals: `tune1`, `tune2`, `vol1`, `vol2`, `volume1`, `volume2`, `stereo`, `remaining_data`, `at_eof`, `last_fill`, `fp`, `tx_chan`.

### HTTP API (`main/net.c`)
All endpoints handled by the Mongoose `cb()` callback:

| Endpoint | Description |
|---|---|
| `POST /upload?offset=&total=` | Upload WAV file to SD `/tunes/` (chunked) |
| `GET /play?name=&vol=` | Play a tune file |
| `GET /list` | List filenames in `/tunes/` (comma-separated) |
| `GET /conf/get/file1` or `/file2` | Get current tune selection |
| `GET /conf/get/volume1` or `/volume2` | Get current volume (dB string) |
| `GET /conf/set?file1=` or `?volume1=` etc. | Set tune/volume, persist to SD |
| `POST /firmware/upload` | OTA firmware update |
| `GET /firmware/info` | Firmware name, date, version, partition |
| All other URIs | Serve static files from SD `/web_root/` |

Config is persisted to SD card in plain-text files under `/conf/`. WiFi credentials and hostname are also read from `/conf/` at boot.

### SD card directory structure
```
/sdcard/
  conf/
    ssid.txt          WiFi SSID
    password.txt      WiFi password (supports spaces, uses fgets)
    hostname.txt      mDNS hostname (default: "jetson")
    filename1.txt     Selected tune for button 1
    filename2.txt     Selected tune for button 2
    volume1.txt       Volume for button 1 (dB, e.g. "-14")
    volume2.txt       Volume for button 2 (dB)
  tunes/              WAV files (PCM, 16-bit, ≤48kHz, mono or stereo)
  web_root/           Static web assets served by Mongoose
```

### GPIO pin assignments (`main/gpio.h`)
- **Buttons**: BUTTON1_PIN=17, BUTTON2_PIN=18 (active-low, pull-up, falling-edge interrupt)
- **Amplifier enable**: SD_MODE_PIN=14 (open-drain, low=off)
- **SD card detect**: CD_PIN=8 (pull-up, low=card present)
- **Error LED**: BLINK_PIN=13
- **I2S**: BCLK=2, WS=38, DOUT=21
- **SDMMC**: CLK=9, CMD=10, DAT0–3=1,3,12,11
- **Logic analyzer** (HW_PROFILE only): LA0–3=4,5,6,7

### Error blink codes
LED on BLINK_PIN blinks N times every 3 seconds (infinite loop, halts normal operation):
- 1 blink: SD card not present or mount failed
- 2 blinks: Config restore failed (missing `/conf/` files)
- 3 blinks: WiFi connection failed after 10 retries

## WAV File Requirements
Files must be: PCM (audio format = 1), 16-bit samples, 1 or 2 channels, sample rate ≤ 48kHz. Convert MP3 to WAV before use (e.g., Audacity, online converters).
