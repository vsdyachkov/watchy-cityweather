# Project Architecture

## Overview

Custom Watchy watch face (ESP32 + e-ink 200x200). Inherits from `Watchy` SDK, services via composition.

## Data Flows

```
Deep Sleep → Wake → WiFi → ipwho.is → Open-Meteo → NVS cache → Draw → Deep Sleep
                                                       ↑
Buttons → Watchy hooks → CityWeather → Services
                                                       ↑
BLE (ANCS) → NotificationService → FreeRTOS tasks
```

## Files

| File | Role |
|------|------|
| `src/AstroWeather.ino.cpp` | Entry point, singleton |
| `src/CityWeather.h/cpp` | Main class: UI, buttons, lifecycle |
| `src/CityWeatherService.h/cpp` | Weather: geolocation, API, NVS cache |
| `src/NotificationService.h/cpp` | ANCS: iOS push via BLE |
| `src/StatusBar.h/cpp` | Status bar + WiFi state persistence |
| `src/BatteryMonitor.h/cpp` | Battery: voltage → percent, stabilization |
| `src/Screenshot.h/cpp` | Screenshots via Serial (debug) |
| `src/settings.h` | Configuration defaults |
| `src/Adafruit_GFX_ext.h` | Graphics utilities |
| `scripts/patch_watchy_library.py` | Dependency patches at build time |

## Design Decisions

### NVS Caches
Pattern: **magic + version + checksum** (FNV-1a) for all NVS storage. Legacy formats supported backwards-compatibly.

### RTC_DATA_ATTR
Variables preserved across deep sleep: weather cache, battery history, WiFi state, screenshot buffer.

### Library Patching
`patch_watchy_library.py` modifies Watchy/ANCS/GxEPD2 source at build time via marker string search. Weak hooks injected into SDK to extend lifecycle.

### Graphics
E-ink 1-bit, dithering (2px step), outlined text/bitmap. Fonts in PROGMEM.
