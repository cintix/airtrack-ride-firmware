# AirTrack Ride Firmware - Agent Init

This document is a session bootstrap for coding agents and maintainers.
It captures practical project knowledge so each session can start fast.

## Quick Start
- Project root: `airtrack-ride-firmware`
- Board: `esp32-c3-devkitm-1`
- Framework: `Arduino` (PlatformIO)
- Entrypoint: `src/main.cpp`
- Primary loop order in `loop()`:
  - `gps.update()`
  - `barometric.update()`
  - `input.update()`
  - app/storage/client/screen handling

## Build + Tooling In This Environment
- Preferred command:
  - `~/.platformio/penv/bin/pio run`
- Why:
  - `pio` is often not on PATH in non-interactive Codex shells.
- Env name in `platformio.ini`:
  - `airtrack-ride-firmware-esp32-c3`
- If sandbox blocks PlatformIO writes in `~/.platformio`:
  - rerun build with escalated permissions.

## Dependency Snapshot
- UI:
  - `olikraus/U8g2`
- Sensor:
  - `adafruit/Adafruit BME280 Library`
  - `adafruit/Adafruit Unified Sensor`

## Top-Level Module Map
- `src/gps/*`
  - UBX serial protocol handling + record decoding.
- `src/barometric/*`
  - BME280 I2C readings (temperature/pressure/humidity/altitude).
- `src/application/*`
  - ride state, distance/speed/calorie calculations, track-point sampling.
- `src/storage/*`
  - persisted settings loading + track buffering hooks.
- `src/client/*`
  - WiFi/AP+STA state machine, HTTP server, setup/app API.
- `src/screen/*`
  - display abstraction + OLED/e-paper implementations.
- `src/input/*`
  - debounced toggle button.
- `src/config/Config.h`
  - central compile-time config constants.

## Runtime Data Flow
1. `GpsReader` updates internal `GpsRecord` from UBX packets.
2. `Barometric` updates latest environmental reading on interval.
3. On valid GPS fix, `Application::update(gpsFix, ambientTempC)` computes ride state.
4. Optional `StoredTrackPoint` forwarded to `Storage::update`.
5. `DisplayRecord` updated and pushed through `Screen` refresh policy.
6. `ClientSync` serves API/UI via WiFi when enabled.

## Module Contracts (What To Know Before Editing)

### GPS (`GpsReader`, `UbxReader`)
- Files:
  - `src/gps/GpsReader.h/.cpp`
  - `src/gps/UbxReader.h/.cpp`
- Responsibilities:
  - configure GPS module to UBX NAV messages.
  - parse binary stream using byte-level state machine.
  - expose latest `GpsRecord` with flags like `valid`, `hasUtcTime`.
- Important behavior:
  - `UbxReader::begin()` enables NAV-POSLLH, NAV-VELNED, NAV-SOL, NAV-TIMEUTC.
  - update rate currently set via `setUpdateRate(1000)` in `UbxReader::begin()`.
  - `GpsReader::getRecord()` clears `recordAvailable` on read.

### Barometric (`Barometric`)
- Files:
  - `src/barometric/Barometric.h/.cpp`
- Responsibilities:
  - initialize I2C BME280.
  - periodic sampling and exposing last valid reading.
- Important behavior:
  - probes configured I2C address (default `0x76`) then fallback `0x77`.
  - `update()` throttled by `BAROMETRIC_READ_INTERVAL_MS`.
  - guards against `NaN`/`Inf` readings.

### Application (`Application`)
- Files:
  - `src/application/Application.h/.cpp`
  - `src/application/models/*`
- Responsibilities:
  - tracking enable/disable state.
  - distance, elapsed/moving time, avg speed, calorie estimation.
  - track-point decimation using min distance/time thresholds.
  - provides display data including temperature.
- Important behavior:
  - overloaded update methods:
    - `update(const GpsFix&)`
    - `update(const GpsFix&, float temperatureC)`

### Storage (`Storage`, `TrackWriter`)
- Files:
  - `src/storage/Storage.h/.cpp`
  - `src/storage/TrackWriter.h/.cpp`
- Responsibilities:
  - mount LittleFS, ensure `/config` exists.
  - load user profile from `/config/profile.txt` key-value file.
  - accept track points only when tracking is enabled.
- Current status:
  - `TrackWriter::flush()` currently logs buffered count (placeholder, not full file export yet).

### Client (`ClientSync`, `WifiManager`, `HttpServerHost`, `ClientApi`, `ProfileService`)
- Files:
  - `src/client/*`
- Responsibilities:
  - WiFi AP/STA lifecycle with retry backoff.
  - serve setup/app static files and REST-like endpoints.
  - persist WiFi and profile configs in LittleFS.
- Important behavior:
  - `ClientSync::setWifiEnabled(false)` stops server + WiFi radio when tracking starts.
  - AP SSID format: `WIFI_AP_SSID_PREFIX-<chip suffix>`.

### Screen (`Screen`, `DisplayRecord`, OLED/EPaper)
- Files:
  - `src/screen/*`
- Responsibilities:
  - throttle rendering by time + significant-change thresholds.
  - hardware-specific draw output.
- Important behavior:
  - `Screen::update()` only redraws on refresh policy conditions.
  - OLED currently shows speed, distance, temperature, pressure+humidity.

### Input (`Input`)
- Files:
  - `src/input/Input.h/.cpp`
- Responsibilities:
  - detect debounced press on `INPUT_TOGGLE_PIN`.
  - produce edge-triggered toggle events via `IsToggled()`.

## Config Hotspots (`src/config/Config.h`)
- GPS:
  - UART pins/baud, UBX payload size, update rate, dynamic platform model.
- Tracking:
  - `TRACK_MIN_DISTANCE_METERS`, `TRACK_MIN_INTERVAL_SECONDS`.
  - moving-stop threshold and grace delay.
- Input:
  - pin + debounce duration.
- Display refresh:
  - min/max refresh + delta thresholds.
- WiFi:
  - AP SSID/pw/channel/IP and STA retry/backoff timings.
- Barometric:
  - enable flag, SDA/SCL pins, I2C address, read interval, sea-level pressure.

## Filesystem Keys + Paths
- Profile file:
  - `/config/profile.txt`
  - keys: `weightKg`, `ageYears`, `isMale`, `restingHeartRateBpm`, `timezoneOffsetMinutes`, `stoppedSpeedThresholdKmh`, `stoppedDelaySeconds`
- WiFi file:
  - `/config/wifi.txt`
  - keys: `ssid`, `password`
- UI static files served from:
  - `/www/setup/*`, `/www/app/*`, `/www/shared/*`

## HTTP API Endpoints
- `GET /api/status`
- `GET /api/wifi/scan`
- `POST /api/setup/wifi`
- `GET /api/profile`
- `POST /api/profile`
- `GET /api/rides` (currently mock/static payload)
- `POST /api/reboot`

## Current Barometric Integration Notes
- `main.cpp` owns global `Barometric barometric;`
- `setup()` calls `barometric.begin()` once.
- `loop()` calls `barometric.update()`.
- Latest barometric reading is copied into `DisplayRecord` fields:
  - `tempatureC`, `pressureHpa`, `humidityPercent`
- Ambient temperature is also fed into `Application::update(...)`.

## Known Caveats / TODOs
- Naming typo is currently intentional in struct field:
  - `DisplayRecord::tempatureC` (legacy spelling across codebase).
- `TrackWriter` is buffering-only right now; persistence/export pipeline is incomplete.
- `ClientApi::handleRides()` still returns hardcoded sample data.

## Recommended Session Routine
1. Read this file.
2. Read `src/main.cpp` and `src/config/Config.h`.
3. Build with `~/.platformio/penv/bin/pio run`.
4. If changes touch runtime behavior, verify serial logs for module init and state transitions.

## Last Validated
- Date: `2026-04-14`
- Build status: `SUCCESS` via `~/.platformio/penv/bin/pio run`
