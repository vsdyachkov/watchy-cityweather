# Rules — CityWeather

## Quick Facts

- **What**: Watchy watch face (ESP32 + e-ink 200x200)
- **Language**: C/C++, Arduino framework, PlatformIO
- **Features**: weather, iOS notifications (ANCS/BLE), battery graph, WiFi
- **Repo**: `vsdykov/watchy-cityweather`

## Naming

| Element | File | Class/Type | Var/Function | Constant |
|---------|------|-----------|--------------|----------|
| Style | `snake_case` | `PascalCase` | `snake_case` | `UPPER_SNAKE_CASE` |

## Principles

1. Read files before changing
2. Minimal changes, no refactoring without request
3. Don't break NVS compatibility or patch markers
4. One change = one logical group

## ESP32

- **RTC_DATA_ATTR** — variables survive deep sleep
- **NVS Preferences** — caches (weather, battery, WiFi)
- **FreeRTOS** — tasks for ANCS actions
- **PROGMEM** — icons and fonts

## NVS Caches

Pattern: `magic + version + data + checksum(FNV-1a)`. Load: validate → restore. Save: fill → checksum → putBytes. Legacy formats supported.

## Battery

5 samples → trimmed mean → 12-point curve (3300mV=0%…4200mV=100%) → stabilization (2 drop confirmations within 5%, max unconfirmed drop 15%). History: circular buffer, 30 hourly samples.

## ANCS

Apple Notification Center Service via BLE. Up to 16 notifications, 3s batching, 90ms vibration, action labels, local dismiss (EN/RU), FreeRTOS tasks, semaphore sync. During network: Bluetooth stops, resumes after 1.2s.

## Graphics

E-ink 1-bit, dithering 2px, outlined text, fonts in PROGMEM.

## Patching

`scripts/patch_watchy_library.py` — modifies Watchy/ANCS/GxEPD2 via marker search. Never remove markers.

## CI

`CITYWEATHER_VERSION` from git tags, `CITYWEATHER_REPOSITORY` from remote. Web installer via ESP Web Tools.

## Don't

- ❌ Remove legacy NVS formats without migration
- ❌ Change magic/version without updating loader
- ❌ Remove markers in `patch_watchy_library.py`
- ❌ Use `Serial.print` in production
- ❌ Add `RTC_DATA_ATTR` without need
- ❌ Use `String` — use `char[]` + `strncpy`
