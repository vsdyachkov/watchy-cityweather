# Migration Plan: Remove `patch_watchy_library.py`

## Goal
Replace runtime library patching with local Watchy fork in `lib/`.

## Phase 1: Preparation (5 min)
1. Create `lib/Watchy/` — copy from `.pio/libdeps/`
2. Create `lib/GxEPD2/` — copy from `.pio/libdeps/`
3. Create `lib/ESP32-ANCS-Notifications/` — copy from `../ESP32-ANCS-Notifications`

## Phase 2: Watchy.cpp patches (~30 min)
Transfer all 17 patch functions into `lib/Watchy/src/Watchy.cpp`:
- `patch_minute_tick` → `watchyMinuteTick()` in `loop()`
- `patch_app_tick` → `watchyAppTick()` in `loop()`
- `patch_wifi_configured_hook` → `watchyWifiConfigured()` after WiFi
- `patch_wifi_setup_cancel` → WiFi cancel handling
- `patch_wifi_setup_current_info` → save current WiFi info
- `patch_fast_menu_partial_rows` → partial menu refresh
- `patch_notifications_menu_hook` → notification menu hooks
- `patch_notifications_menu_state_hook` → notification menu state
- `patch_deep_sleep_hook` → `watchyShouldDeepSleep()`
- `patch_quiet_hours_vibration` → quiet hours vibration
- `patch_menu_watchface_partial_refresh` → partial refresh from menu
- `patch_menu_notifications_item` → notifications menu item
- `patch_menu_loop_hook` → `watchyMenuLoop()`
- `patch_menu_shown_hook` → `watchyMenuShown()`
- `patch_about_app_version` → version in about screen
- `patch_about_screen_hook` → `watchyShowAbout()`
- `patch_screenshot_request_hook` → `watchyScreenshotRequested()`

## Phase 3: Display.cpp patches (~10 min)
In `lib/Watchy/src/Display.cpp`:
- Disable initial full refresh
- Partial refresh on menu return

## Phase 4: config.h patches (~2 min)
In `lib/Watchy/src/config.h`:
- Add screenshot buffer constants
- Add Bluetooth stop guard

## Phase 5: GxEPD2 patches (~5 min)
In `lib/GxEPD2/src/GxEPD2_BW.h`:
- Add `cityWeatherScreenshotFrame()` weak declaration
- Insert calls in full-frame and partial-frame rendering

## Phase 6: ANCS patches (~10 min)
In `lib/ESP32-ANCS-Notifications/src/`:
- `ble_notification.h` — add `subtitle` field
- `ancs_ble_client.h/cpp` — subtitle support, advertising fixes
- `esp32notifications.cpp` — stop cleanup, advertising fixes

## Phase 7: Update platformio.ini (~2 min)
```ini
extra_scripts =  # removed
lib_ldf_mode = deep+
lib_extra_dirs = lib
lib_ignore = .pio
```

### Dependencies to add (not pulled by Watchy anymore):
- `arduino-libraries/NTPClient`
- `arduino-libraries/Arduino_JSON`
- `jchristensen/DS3232RTC`
- `adafruit/Adafruit GFX Library`
- `adafruit/Adafruit BusIO`

## Phase 8: Build and verify
```bash
rm -rf .pio
pio run
```
**Status: BUILD SUCCESSFUL**

## Phase 9: Remove script
```bash
rm scripts/patch_watchy_library.py
```

## Phase 10: Replace weak hooks with virtual methods
Added 10 virtual extension methods to `Watchy` class:
`onMinuteTick()`, `onAppTick()`, `shouldDeepSleep()`, `screenshotRequested()`,
`notificationsEnabled()`, `onWifiConfigured()`, `onMenuLoop()`, `onMenuShown()`,
`handleAbout()`, `onNotificationsSelected()`.
CityWeather overrides these via `override`. NotificationService updated to call virtual methods.
All `__attribute__((weak))` functions and `patch_watchy_library.py` comments removed.

## Progress Summary
| Phase | Status |
|-------|--------|
| 1. Copy libraries to lib/ | DONE |
| 2. Watchy.cpp patches | DONE |
| 3. Display.cpp patches | DONE |
| 4. config.h patches | DONE |
| 5. GxEPD2 patches | DONE |
| 6. ANCS patches | DONE |
| 7. Update platformio.ini | DONE |
| 8. Build and verify | DONE (SUCCESS) |
| 9. Remove patch script | DONE |
| 10. Replace weak hooks with virtual methods | DONE |

## Result
Migration complete. Zero dependency on `patch_watchy_library.py`. All libraries local in `lib/`.
Build: 22.7% RAM, 59.8% Flash.
