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

## Extension Points

Watchy uses virtual methods for application-specific behavior. CityWeather overrides:
`onMinuteTick()`, `onAppTick()`, `shouldDeepSleep()`, `screenshotRequested()`, `notificationsEnabled()`, `onWifiConfigured()`, `onMenuLoop()`, `onMenuShown()`, `handleAbout()`, `onNotificationsSelected()`.
Never remove or change signatures of these virtual methods.

## CI

`CITYWEATHER_VERSION` from git tags, `CITYWEATHER_REPOSITORY` from remote. Web installer via ESP Web Tools.

## Don't

- ❌ Remove legacy NVS formats without migration
- ❌ Change magic/version without updating loader
- ❌ Use `Serial.print` in production
- ❌ Add `RTC_DATA_ATTR` without need
- ❌ Use `String` — use `char[]` + `strncpy`

## Build Checks (Mandatory)

Every change must pass these checks before committing:

1. **`pio run` succeeds** — the build must compile without errors. This is the primary gate.
2. **Size limits** — check output after `Checking size`:
   - RAM ≤ 320 KB (currently 22.7%)
   - Flash ≤ 4 MB (currently 59.8%)
3. **No `patch_watchy_library.py` references** — all libraries are local copies in `lib/`. No references to the patch script should remain in any source file.
4. **No secrets** — never commit API keys, passwords, or tokens. GitHub Push Protection will block the push.
5. **Clean build** — run `rm -rf .pio && pio run` to clear the cache. Cached artifacts can hide real errors.

## Build Checks (Recommended)

- **No new warnings** — review compiler warnings for new issues
- **`lib/` is tracked** — verify with `git check-ignore lib/` that local libraries are not ignored
- **Deploy to device** — if hardware is available, test the binary on the actual Watchy
