from pathlib import Path

Import("env")


SCRIPT_PATH = "scripts/patch_watchy_library.py"
LEGACY_SCRIPT_PATH = "scripts/" + "disable_watchy_" + "reset_vibration.py"

RESET_VIBRATION_PATCH_MARKER = f"Reset vibration disabled by {SCRIPT_PATH}"
RESET_WATCHFACE_PARTIAL_PATCH_MARKER = f"Reset watchface refresh adjusted by {SCRIPT_PATH}"
DISPLAY_NO_INITIAL_FULL_REFRESH_PATCH_MARKER = (
    f"Display initial full refresh disabled by {SCRIPT_PATH}"
)
DISPLAY_FORCED_INITIAL_PARTIAL_REFRESH_PATCH_MARKER = (
    f"Display forced initial partial refresh by {SCRIPT_PATH}"
)
MINUTE_TICK_HOOK_PATCH_MARKER = f"Minute tick hook added by {SCRIPT_PATH}"
WIFI_CONFIGURED_HOOK_PATCH_MARKER = f"WiFi configured hook added by {SCRIPT_PATH}"
WIFI_SETUP_CANCEL_PATCH_MARKER = f"WiFi setup cancel added by {SCRIPT_PATH}"
NOTIFICATIONS_MENU_HOOK_PATCH_MARKER = (
    f"Notifications menu hook added by {SCRIPT_PATH}"
)
NOTIFICATIONS_MENU_STATE_HOOK_PATCH_MARKER = (
    f"Notifications menu state hook added by {SCRIPT_PATH}"
)
DEEP_SLEEP_HOOK_PATCH_MARKER = f"Deep sleep hook added by {SCRIPT_PATH}"
QUIET_HOURS_VIBRATION_PATCH_MARKER = f"Hourly vibration quiet hours added by {SCRIPT_PATH}"
MENU_WATCHFACE_PARTIAL_REFRESH_PATCH_MARKER = (
    f"Menu returns switched to partial watchface refresh by {SCRIPT_PATH}"
)
MENU_NOTIFICATIONS_ITEM_PATCH_MARKER = f"Notifications menu item added by {SCRIPT_PATH}"


def patch_watchy(*_args, **_kwargs):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    pio_env = env.subst("$PIOENV")

    watchy_cpp = find_lib_file(project_dir, pio_env, "Watchy.cpp")
    if watchy_cpp is None:
        print("Watchy patch: Watchy.cpp not found yet")
    else:
        patch_watchy_cpp(watchy_cpp)

    config_h = find_lib_file(project_dir, pio_env, "config.h")
    if config_h is None:
        print("Watchy patch: config.h not found yet")
    else:
        patch_config_h(config_h)

    display_cpp = find_lib_file(project_dir, pio_env, "Display.cpp")
    if display_cpp is None:
        print("Watchy patch: Display.cpp not found yet")
    else:
        patch_display_cpp(display_cpp)


def find_lib_file(project_dir, pio_env, file_name):
    candidates = (
        project_dir / ".pio" / "libdeps" / pio_env / "Watchy" / "src" / file_name,
        project_dir / ".pio" / "libdeps" / "watchy" / "Watchy" / "src" / file_name,
    )
    return next((path for path in candidates if path.exists()), None)


def patch_watchy_cpp(watchy_cpp):
    text = watchy_cpp.read_text()
    original_text = text

    text = normalize_marker_paths(text)
    text = dedupe_event_hooks(text)
    text = dedupe_wifi_configured_hook(text)
    text = add_event_hooks(text)
    text = patch_notifications_menu(text)
    text = patch_deep_sleep_hook(text)
    text = patch_minute_tick(text)
    text = patch_quiet_hours_vibration(text)
    text = patch_wifi_configured_hook(text)
    text = patch_wifi_setup_cancel(text)
    text = patch_menu_partial_refresh(text)
    text = patch_white_menu_theme(text)
    text = patch_menu_font(text)
    text = patch_reset_vibration(text)
    text = patch_reset_watchface_refresh(text)

    if text != original_text:
        watchy_cpp.write_text(text)


def add_event_hooks(text):
    if (
        MINUTE_TICK_HOOK_PATCH_MARKER in text
        and WIFI_CONFIGURED_HOOK_PATCH_MARKER in text
        and NOTIFICATIONS_MENU_HOOK_PATCH_MARKER in text
        and NOTIFICATIONS_MENU_STATE_HOOK_PATCH_MARKER in text
        and DEEP_SLEEP_HOOK_PATCH_MARKER in text
    ):
        print("Watchy event hook patch: already applied")
        return text

    notifications_hook = (
        f"\n// {NOTIFICATIONS_MENU_HOOK_PATCH_MARKER}\n"
        "void watchyNotificationsSelected(Watchy *watchy) __attribute__((weak));\n"
        "void watchyNotificationsSelected(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "}\n"
    )
    deep_sleep_hook = (
        f"\n// {DEEP_SLEEP_HOOK_PATCH_MARKER}\n"
        "bool watchyShouldDeepSleep(Watchy *watchy) __attribute__((weak));\n"
        "bool watchyShouldDeepSleep(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "  return true;\n"
        "}\n"
    )
    notifications_state_hook = (
        f"\n// {NOTIFICATIONS_MENU_STATE_HOOK_PATCH_MARKER}\n"
        "bool watchyNotificationsEnabled(Watchy *watchy) __attribute__((weak));\n"
        "bool watchyNotificationsEnabled(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "  return false;\n"
        "}\n"
    )

    if (
        MINUTE_TICK_HOOK_PATCH_MARKER in text
        and WIFI_CONFIGURED_HOOK_PATCH_MARKER in text
        and (
            NOTIFICATIONS_MENU_HOOK_PATCH_MARKER not in text
            or NOTIFICATIONS_MENU_STATE_HOOK_PATCH_MARKER not in text
            or DEEP_SLEEP_HOOK_PATCH_MARKER not in text
        )
    ):
        anchor = (
            f"\n// {WIFI_CONFIGURED_HOOK_PATCH_MARKER}\n"
            "void watchyWifiConfigured(Watchy *watchy) __attribute__((weak));\n"
            "void watchyWifiConfigured(Watchy *watchy) {\n"
            "  (void)watchy;\n"
            "}\n"
        )
        hooks = ""
        if NOTIFICATIONS_MENU_HOOK_PATCH_MARKER not in text:
            hooks += notifications_hook
        if NOTIFICATIONS_MENU_STATE_HOOK_PATCH_MARKER not in text:
            hooks += notifications_state_hook
        if DEEP_SLEEP_HOOK_PATCH_MARKER not in text:
            hooks += deep_sleep_hook
        return replace_or_log(
            text,
            anchor,
            anchor + hooks,
            "Watchy additional event hook patch",
        )

    anchor = (
        "GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> Watchy::display(\n"
        "    WatchyDisplay{});\n"
    )
    patch = anchor + (
        f"\n// {MINUTE_TICK_HOOK_PATCH_MARKER}\n"
        "void watchyMinuteTick(Watchy *watchy) __attribute__((weak));\n"
        "void watchyMinuteTick(Watchy *watchy) {\n"
        "  watchy->showWatchFace(true);\n"
        "}\n"
        f"\n// {WIFI_CONFIGURED_HOOK_PATCH_MARKER}\n"
        "void watchyWifiConfigured(Watchy *watchy) __attribute__((weak));\n"
        "void watchyWifiConfigured(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "}\n"
        + notifications_hook
        + notifications_state_hook
        + deep_sleep_hook
    )
    return replace_or_log(text, anchor, patch, "Watchy event hook patch")


def normalize_marker_paths(text):
    return text.replace(LEGACY_SCRIPT_PATH, SCRIPT_PATH)


def dedupe_event_hooks(text):
    block = (
        f"\n// {MINUTE_TICK_HOOK_PATCH_MARKER}\n"
        "void watchyMinuteTick(Watchy *watchy) __attribute__((weak));\n"
        "void watchyMinuteTick(Watchy *watchy) {\n"
        "  watchy->showWatchFace(true);\n"
        "}\n"
        f"\n// {WIFI_CONFIGURED_HOOK_PATCH_MARKER}\n"
        "void watchyWifiConfigured(Watchy *watchy) __attribute__((weak));\n"
        "void watchyWifiConfigured(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "}\n"
        f"\n// {NOTIFICATIONS_MENU_HOOK_PATCH_MARKER}\n"
        "void watchyNotificationsSelected(Watchy *watchy) __attribute__((weak));\n"
        "void watchyNotificationsSelected(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "}\n"
        f"\n// {NOTIFICATIONS_MENU_STATE_HOOK_PATCH_MARKER}\n"
        "bool watchyNotificationsEnabled(Watchy *watchy) __attribute__((weak));\n"
        "bool watchyNotificationsEnabled(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "  return false;\n"
        "}\n"
        f"\n// {DEEP_SLEEP_HOOK_PATCH_MARKER}\n"
        "bool watchyShouldDeepSleep(Watchy *watchy) __attribute__((weak));\n"
        "bool watchyShouldDeepSleep(Watchy *watchy) {\n"
        "  (void)watchy;\n"
        "  return true;\n"
        "}\n"
    )
    return keep_first_occurrence(text, block)


def dedupe_wifi_configured_hook(text):
    block = (
        f"    // {WIFI_CONFIGURED_HOOK_PATCH_MARKER}\n"
        "    watchyWifiConfigured(this);\n"
    )
    return keep_first_occurrence(text, block)


def keep_first_occurrence(text, block):
    first_index = text.find(block)
    if first_index == -1:
        return text

    search_from = first_index + len(block)
    while True:
        duplicate_index = text.find(block, search_from)
        if duplicate_index == -1:
            return text
        text = text[:duplicate_index] + text[duplicate_index + len(block):]
        search_from = first_index + len(block)


def patch_minute_tick(text):
    original = (
        "    case WATCHFACE_STATE:\n"
        "      showWatchFace(true); // partial updates on tick\n"
    )
    patched = (
        "    case WATCHFACE_STATE:\n"
        f"      // {MINUTE_TICK_HOOK_PATCH_MARKER}\n"
        "      watchyMinuteTick(this); // partial updates on tick\n"
    )
    return replace_or_log(text, original, patched, "Watchy minute tick hook patch")


def patch_quiet_hours_vibration(text):
    original = (
        "      if (settings.vibrateOClock) {\n"
        "        if (currentTime.Minute == 0) {\n"
    )
    patched = (
        "      if (settings.vibrateOClock) {\n"
        f"        // {QUIET_HOURS_VIBRATION_PATCH_MARKER}\n"
        "        if (currentTime.Minute == 0 && currentTime.Hour >= 10 && currentTime.Hour < 22) {\n"
    )
    return replace_or_log(text, original, patched, "Watchy quiet-hours vibration patch")


def patch_wifi_configured_hook(text):
    original = (
        "    weatherIntervalCounter = -1; // Reset to force weather to be read again\n"
        "    lastIPAddress = WiFi.localIP();\n"
        "    WiFi.SSID().toCharArray(lastSSID, 30);\n"
    )
    patched = original + (
        f"    // {WIFI_CONFIGURED_HOOK_PATCH_MARKER}\n"
        "    watchyWifiConfigured(this);\n"
    )
    return replace_or_log(text, original, patched, "Watchy WiFi configured hook patch")


def patch_wifi_setup_cancel(text):
    if WIFI_SETUP_CANCEL_PATCH_MARKER in text:
        print("Watchy WiFi setup cancel patch: already applied")
        return text

    original = (
        "void Watchy::setupWifi() {\n"
        "  display.epd2.setBusyCallback(0); // temporarily disable lightsleep on busy\n"
        "  WiFiManager wifiManager;\n"
        "  wifiManager.resetSettings();\n"
        "  wifiManager.setTimeout(WIFI_AP_TIMEOUT);\n"
        "  wifiManager.setAPCallback(_configModeCallback);\n"
        "  display.setFullWindow();\n"
        "  display.fillScreen(GxEPD_WHITE);\n"
        "  display.setFont(&FreeMonoBold9pt7b);\n"
        "  display.setTextColor(GxEPD_BLACK);\n"
        "  if (!wifiManager.autoConnect(WIFI_AP_SSID)) { // WiFi setup failed\n"
        "    display.println(\"Setup failed &\");\n"
        "    display.println(\"timed out!\");\n"
        "  } else {\n"
        "    display.println(\"Connected to:\");\n"
        "    display.println(WiFi.SSID());\n"
        "\t\tdisplay.println(\"Local IP:\");\n"
        "\t\tdisplay.println(WiFi.localIP());\n"
        "    weatherIntervalCounter = -1; // Reset to force weather to be read again\n"
        "    lastIPAddress = WiFi.localIP();\n"
        "    WiFi.SSID().toCharArray(lastSSID, 30);\n"
        f"    // {WIFI_CONFIGURED_HOOK_PATCH_MARKER}\n"
        "    watchyWifiConfigured(this);\n"
        "  }\n"
        "  display.display(true); // partial refresh\n"
        "  // turn off radios\n"
        "  WiFi.mode(WIFI_OFF);\n"
        "  btStop();\n"
        "  // enable lightsleep on busy\n"
        "  display.epd2.setBusyCallback(WatchyDisplay::busyCallback);\n"
        "  guiState = APP_STATE;\n"
        "}\n"
    )
    patched = (
        "void Watchy::setupWifi() {\n"
        "  // " + WIFI_SETUP_CANCEL_PATCH_MARKER + "\n"
        "  display.epd2.setBusyCallback(0); // temporarily disable lightsleep on busy\n"
        "  WiFiManager wifiManager;\n"
        "  wifiManager.setConfigPortalBlocking(false);\n"
        "  wifiManager.setConfigPortalTimeout(WIFI_AP_TIMEOUT);\n"
        "  wifiManager.setSaveConnectTimeout(20);\n"
        "  wifiManager.setAPCallback(_configModeCallback);\n"
        "  pinMode(BACK_BTN_PIN, INPUT);\n"
        "  display.setFullWindow();\n"
        "  display.fillRect(0, 24, 200, 176, GxEPD_WHITE);\n"
        "  display.setFont(&FreeMonoBold9pt7b);\n"
        "  display.setTextColor(GxEPD_BLACK);\n"
        "  display.setCursor(0, 54);\n"
        "  display.println(\"Starting WiFi\");\n"
        "  display.println(\"setup...\");\n"
        "  display.println(\"Back: cancel\");\n"
        "  display.displayWindow(0, 24, 200, 176);\n"
        "\n"
        "  bool connected = false;\n"
        "  bool canceled = false;\n"
        "  unsigned long startedAt = millis();\n"
        "  wifiManager.startConfigPortal(WIFI_AP_SSID);\n"
        "  while (!connected && !canceled) {\n"
        "    if (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {\n"
        "      canceled = true;\n"
        "      wifiManager.stopConfigPortal();\n"
        "      while (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {\n"
        "        delay(10);\n"
        "      }\n"
        "      break;\n"
        "    }\n"
        "    connected = wifiManager.process();\n"
        "    if (!connected && WIFI_AP_TIMEOUT > 0 &&\n"
        "        millis() - startedAt > (unsigned long)WIFI_AP_TIMEOUT * 1000UL) {\n"
        "      wifiManager.stopConfigPortal();\n"
        "      break;\n"
        "    }\n"
        "    delay(10);\n"
        "  }\n"
        "\n"
        "  if (connected) {\n"
        "    display.fillRect(0, 24, 200, 176, GxEPD_WHITE);\n"
        "    display.setCursor(0, 54);\n"
        "    display.println(\"Connected to:\");\n"
        "    display.println(WiFi.SSID());\n"
        "    display.println(\"Local IP:\");\n"
        "    display.println(WiFi.localIP());\n"
        "    weatherIntervalCounter = -1; // Reset to force weather to be read again\n"
        "    lastIPAddress = WiFi.localIP();\n"
        "    WiFi.SSID().toCharArray(lastSSID, 30);\n"
        f"    // {WIFI_CONFIGURED_HOOK_PATCH_MARKER}\n"
        "    watchyWifiConfigured(this);\n"
        "    display.displayWindow(0, 24, 200, 176);\n"
        "    guiState = APP_STATE;\n"
        "  } else {\n"
        "    guiState = MAIN_MENU_STATE;\n"
        "  }\n"
        "\n"
        "  // turn off radios\n"
        "  WiFi.mode(WIFI_OFF);\n"
        "  btStop();\n"
        "  // enable lightsleep on busy\n"
        "  display.epd2.setBusyCallback(WatchyDisplay::busyCallback);\n"
        "  if (!connected) {\n"
        "    showMenu(menuIndex, true);\n"
        "  }\n"
        "}\n"
    )
    text = replace_or_log(text, original, patched, "Watchy WiFi setup cancel patch")

    original_callback = (
        "void Watchy::_configModeCallback(WiFiManager *myWiFiManager) {\n"
        "  display.setFullWindow();\n"
        "  display.fillScreen(GxEPD_WHITE);\n"
        "  display.setFont(&FreeMonoBold9pt7b);\n"
        "  display.setTextColor(GxEPD_BLACK);\n"
        "  display.setCursor(0, 30);\n"
        "  display.println(\"Connect to\");\n"
        "  display.print(\"SSID: \");\n"
        "  display.println(WIFI_AP_SSID);\n"
        "  display.print(\"IP: \");\n"
        "  display.println(WiFi.softAPIP());\n"
        "\tdisplay.println(\"MAC address:\");\n"
        "\tdisplay.println(WiFi.softAPmacAddress().c_str());\n"
        "  display.display(true); // partial refresh\n"
        "}\n"
    )
    patched_callback = (
        "void Watchy::_configModeCallback(WiFiManager *myWiFiManager) {\n"
        "  display.setFullWindow();\n"
        "  display.fillRect(0, 24, 200, 176, GxEPD_WHITE);\n"
        "  display.setFont(&FreeMonoBold9pt7b);\n"
        "  display.setTextColor(GxEPD_BLACK);\n"
        "  display.setCursor(0, 54);\n"
        "  display.println(\"Connect to\");\n"
        "  display.print(\"SSID: \");\n"
        "  display.println(WIFI_AP_SSID);\n"
        "  display.print(\"IP: \");\n"
        "  display.println(WiFi.softAPIP());\n"
        "  display.println(\"Back: cancel\");\n"
        "  display.displayWindow(0, 24, 200, 176);\n"
        "}\n"
    )
    return replace_or_log(
        text,
        original_callback,
        patched_callback,
        "Watchy WiFi setup callback area patch",
    )


def patch_notifications_menu(text):
    original_items = (
        "  const char *menuItems[] = {\n"
        "      \"About Watchy\", \"Vibrate Motor\", \"Show Accelerometer\",\n"
        "      \"Set Time\",     \"Setup WiFi\",    /*\"Update Firmware\",*/\n"
        "      \"Sync NTP\"};\n"
    )
    old_state_items = (
        "  char notificationsMenuItem[12];\n"
        "  snprintf(\n"
        "      notificationsMenuItem,\n"
        "      sizeof(notificationsMenuItem),\n"
        "      \"iOS [%c]\",\n"
        "      watchyNotificationsEnabled(this) ? 'x' : ' '\n"
        "  );\n"
        "  const char *menuItems[] = {\n"
        f"      // {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "      \"About Watchy\", \"Vibrate Motor\", \"Show Accelerometer\",\n"
        "      \"Set Time\",     \"Setup WiFi\",    /*\"Update Firmware\",*/\n"
        "      \"Sync NTP\",     notificationsMenuItem};\n"
    )
    old_custom_items = (
        "  char bluetoothPushState[4];\n"
        "  snprintf(\n"
        "      bluetoothPushState,\n"
        "      sizeof(bluetoothPushState),\n"
        "      \"[%c]\",\n"
        "      watchyNotificationsEnabled(this) ? 'x' : ' '\n"
        "  );\n"
        "  const char *menuItems[] = {\n"
        f"      // {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "      \"Wi-Fi\", \"Bluetooth Push\", \"Set Time\",\n"
        "      \"Sync NTP\", \"Accelerometer\", \"About Watch\"};\n"
        "  const char *menuTrailItems[] = {\n"
        "      \">\", bluetoothPushState, \"\", \"\", \"\", \"\"};\n"
    )
    old_five_item_menu = (
        "  char bluetoothPushItem[18];\n"
        "  snprintf(\n"
        "      bluetoothPushItem,\n"
        "      sizeof(bluetoothPushItem),\n"
        "      \"BT iOS Push [%c]\",\n"
        "      watchyNotificationsEnabled(this) ? 'x' : ' '\n"
        "  );\n"
        "  const char *menuItems[] = {\n"
        f"      // {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "      \"Wi-Fi\", bluetoothPushItem, \"Date/Time\",\n"
        "      \"Accelerometer\", \"About Watchy\"};\n"
        "  const char *menuTrailItems[] = {\n"
        "      \">>\", \"\", \">>\", \"\", \">>\"};\n"
    )
    old_bluetooth_checkbox_items = (
        "  static const unsigned char menuWifiIcon[] PROGMEM = {\n"
        "      0x01, 0xf0, 0x00, 0x07, 0xfc, 0x00, 0x1e, 0x0f,\n"
        "      0x00, 0x39, 0xf3, 0x80, 0x77, 0xfd, 0xc0, 0xef,\n"
        "      0x1e, 0xe0, 0x5c, 0xe7, 0x40, 0x3b, 0xfb, 0x80,\n"
        "      0x17, 0x1d, 0x00, 0x0e, 0xee, 0x00, 0x05, 0xf4,\n"
        "      0x00, 0x03, 0xb8, 0x00, 0x01, 0x50, 0x00, 0x00,\n"
        "      0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00\n"
        "  };\n"
        "  char bluetoothPushItem[16];\n"
        "  snprintf(\n"
        "      bluetoothPushItem,\n"
        "      sizeof(bluetoothPushItem),\n"
        "      \"Bluetooth [%c]\",\n"
        "      watchyNotificationsEnabled(this) ? 'x' : ' '\n"
        "  );\n"
        "  const char *menuItems[] = {\n"
        f"      // {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "      lastSSID[0] != '\\0' ? lastSSID : \"Wi-Fi\", bluetoothPushItem,\n"
        "      \"Set Date/Time\", \"Accelerometer\", \"About Watch\"};\n"
        "  const char *menuTrailItems[] = {\n"
        "      \">\", \"\", \">\", \">\", \">\"};\n"
    )
    patched_items = (
        "  static const unsigned char menuWifiIcon[] PROGMEM = {\n"
        "      0x01, 0xf0, 0x00, 0x07, 0xfc, 0x00, 0x1e, 0x0f,\n"
        "      0x00, 0x39, 0xf3, 0x80, 0x77, 0xfd, 0xc0, 0xef,\n"
        "      0x1e, 0xe0, 0x5c, 0xe7, 0x40, 0x3b, 0xfb, 0x80,\n"
        "      0x17, 0x1d, 0x00, 0x0e, 0xee, 0x00, 0x05, 0xf4,\n"
        "      0x00, 0x03, 0xb8, 0x00, 0x01, 0x50, 0x00, 0x00,\n"
        "      0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00\n"
        "  };\n"
        "  char bluetoothPushState[6];\n"
        "  snprintf(\n"
        "      bluetoothPushState,\n"
        "      sizeof(bluetoothPushState),\n"
        "      \"[%s]\",\n"
        "      watchyNotificationsEnabled(this) ? \"ON\" : \"OFF\"\n"
        "  );\n"
        "  const char *menuItems[] = {\n"
        f"      // {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "      lastSSID[0] != '\\0' ? lastSSID : \"Wi-Fi\", \"Bluetooth\",\n"
        "      \"Set Date/Time\", \"Accelerometer\", \"About Watch\"};\n"
        "  const char *menuTrailItems[] = {\n"
        "      \">\", bluetoothPushState, \">\", \">\", \">\"};\n"
    )
    legacy_patched_items = (
        "  const char *menuItems[] = {\n"
        f"      // {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "      \"About Watchy\", \"Vibrate Motor\", \"Show Accelerometer\",\n"
        "      \"Set Time\",     \"Setup WiFi\",    /*\"Update Firmware\",*/\n"
        "      \"Sync NTP\",     \"iOS Notifications\"};\n"
    )
    legacy_phone_items = legacy_patched_items.replace(
        "\"iOS Notifications\"",
        "\"Phone Alerts\"",
    )
    item_count = text.count(original_items)
    if item_count > 0:
        text = text.replace(original_items, patched_items)
        print(f"Watchy notifications menu item patch: applied {item_count} time(s)")
    elif old_state_items in text:
        text = text.replace(old_state_items, patched_items)
        print("Watchy notifications menu item patch: reordered")
    elif old_custom_items in text:
        text = text.replace(old_custom_items, patched_items)
        print("Watchy notifications menu item patch: updated labels")
    elif old_five_item_menu in text:
        text = text.replace(old_five_item_menu, patched_items)
        print("Watchy notifications menu item patch: updated labels")
    elif old_bluetooth_checkbox_items in text:
        text = text.replace(old_bluetooth_checkbox_items, patched_items)
        print("Watchy notifications menu item patch: changed Bluetooth state label")
    elif legacy_patched_items in text:
        text = text.replace(legacy_patched_items, patched_items)
        print("Watchy notifications menu item patch: switched to state label")
    elif legacy_phone_items in text:
        text = text.replace(legacy_phone_items, patched_items)
        print("Watchy notifications menu item patch: renamed")
    elif patched_items in text:
        print("Watchy notifications menu item patch: already applied")
    else:
        print("Watchy notifications menu item patch: target code was not found")

    old_menu_loop = (
        "  for (int i = 0; i < MENU_LENGTH; i++) {\n"
        "    yPos = MENU_HEIGHT + (MENU_HEIGHT * i);\n"
        "    display.setCursor(0, yPos);\n"
        "    if (i == menuIndex) {\n"
        "      display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);\n"
        "      display.fillRect(x1 - 1, y1 - 10, 200, h + 15, GxEPD_BLACK);\n"
        "      display.setTextColor(GxEPD_WHITE);\n"
        "      display.println(menuItems[i]);\n"
        "    } else {\n"
        "      display.setTextColor(GxEPD_BLACK);\n"
        "      display.println(menuItems[i]);\n"
        "    }\n"
        "  }\n"
    )
    old_trailing_menu_loop = (
        "  for (int i = 0; i < MENU_LENGTH; i++) {\n"
        "    yPos = MENU_HEIGHT + (MENU_HEIGHT * i);\n"
        "    display.setCursor(0, yPos);\n"
        "    if (i == menuIndex) {\n"
        "      display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);\n"
        "      display.fillRect(0, y1 - 10, 200, h + 15, GxEPD_BLACK);\n"
        "      display.setTextColor(GxEPD_WHITE);\n"
        "    } else {\n"
        "      display.setTextColor(GxEPD_BLACK);\n"
        "    }\n"
        "    display.print(menuItems[i]);\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.getTextBounds(menuTrailItems[i], 0, yPos, &x1, &y1, &w, &h);\n"
        "      display.setCursor(198 - w, yPos);\n"
        "      display.print(menuTrailItems[i]);\n"
        "    }\n"
        "  }\n"
    )
    old_wifi_menu_loop = (
        "  for (int i = 0; i < MENU_LENGTH; i++) {\n"
        "    yPos = MENU_HEIGHT + (MENU_HEIGHT * i);\n"
        "    display.setCursor(0, yPos);\n"
        "    if (i == menuIndex) {\n"
        "      display.getTextBounds(menuItems[i], 0, yPos, &x1, &y1, &w, &h);\n"
        "      display.fillRect(0, y1 - 10, 200, h + 15, GxEPD_BLACK);\n"
        "      display.setTextColor(GxEPD_WHITE);\n"
        "    } else {\n"
        "      display.setTextColor(GxEPD_BLACK);\n"
        "    }\n"
        "    uint16_t trailWidth = 0;\n"
        "    int16_t trailX = 198;\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.getTextBounds(menuTrailItems[i], 0, yPos, &x1, &y1, &trailWidth, &h);\n"
        "      trailX = 198 - trailWidth;\n"
        "    }\n"
        "    if (i == 0) {\n"
        "      display.drawBitmap(0, yPos - 17, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);\n"
        "      String wifiText(menuItems[i]);\n"
        "      int16_t wifiTextX = 24;\n"
        "      int16_t maxWifiTextWidth = trailX - wifiTextX - 4;\n"
        "      while (wifiText.length() > 0) {\n"
        "        display.getTextBounds(wifiText.c_str(), wifiTextX, yPos, &x1, &y1, &w, &h);\n"
        "        if (w <= maxWifiTextWidth) {\n"
        "          break;\n"
        "        }\n"
        "        wifiText.remove(wifiText.length() - 1);\n"
        "      }\n"
        "      display.setCursor(wifiTextX, yPos);\n"
        "      display.print(wifiText);\n"
        "    } else {\n"
        "      display.print(menuItems[i]);\n"
        "    }\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.setCursor(trailX, yPos);\n"
        "      display.print(menuTrailItems[i]);\n"
        "    }\n"
        "  }\n"
    )
    old_centered_wifi_menu_loop = (
        "  for (int i = 0; i < MENU_LENGTH; i++) {\n"
        "    yPos = MENU_HEIGHT + (MENU_HEIGHT * i);\n"
        "    int16_t drawYPos = yPos;\n"
        "    if (i == 0) {\n"
        "      int16_t rowCenterY = MENU_HEIGHT * i + MENU_HEIGHT / 2;\n"
        "      display.getTextBounds(menuItems[i], 0, 0, &x1, &y1, &w, &h);\n"
        "      drawYPos = rowCenterY - y1 - h / 2;\n"
        "    }\n"
        "    display.setCursor(0, drawYPos);\n"
        "    if (i == menuIndex) {\n"
        "      display.getTextBounds(menuItems[i], 0, drawYPos, &x1, &y1, &w, &h);\n"
        "      display.fillRect(0, y1 - 10, 200, h + 15, GxEPD_BLACK);\n"
        "      display.setTextColor(GxEPD_WHITE);\n"
        "    } else {\n"
        "      display.setTextColor(GxEPD_BLACK);\n"
        "    }\n"
        "    uint16_t trailWidth = 0;\n"
        "    int16_t trailX = 198;\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.getTextBounds(menuTrailItems[i], 0, drawYPos, &x1, &y1, &trailWidth, &h);\n"
        "      trailX = 198 - trailWidth;\n"
        "    }\n"
        "    if (i == 0) {\n"
        "      int16_t rowCenterY = MENU_HEIGHT * i + MENU_HEIGHT / 2;\n"
        "      display.drawBitmap(1, rowCenterY - 8, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);\n"
        "      String wifiText(menuItems[i]);\n"
        "      int16_t wifiTextX = 24;\n"
        "      int16_t maxWifiTextWidth = trailX - wifiTextX - 4;\n"
        "      while (wifiText.length() > 0) {\n"
        "        display.getTextBounds(wifiText.c_str(), wifiTextX, drawYPos, &x1, &y1, &w, &h);\n"
        "        if (w <= maxWifiTextWidth) {\n"
        "          break;\n"
        "        }\n"
        "        wifiText.remove(wifiText.length() - 1);\n"
        "      }\n"
        "      display.setCursor(wifiTextX, drawYPos);\n"
        "      display.print(wifiText);\n"
        "    } else {\n"
        "      display.print(menuItems[i]);\n"
        "    }\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.setCursor(trailX, drawYPos);\n"
        "      display.print(menuTrailItems[i]);\n"
        "    }\n"
        "  }\n"
    )
    old_fixed_first_only_menu_loop = old_centered_wifi_menu_loop.replace(
        "      display.getTextBounds(menuItems[i], 0, drawYPos, &x1, &y1, &w, &h);\n"
        "      display.fillRect(0, y1 - 10, 200, h + 15, GxEPD_BLACK);\n",
        "      display.fillRect(0, MENU_HEIGHT * i, 200, MENU_HEIGHT, GxEPD_BLACK);\n",
    )
    old_uniform_menu_loop = (
        "  for (int i = 0; i < MENU_LENGTH; i++) {\n"
        "    int16_t rowTop = MENU_HEIGHT * i;\n"
        "    int16_t rowCenterY = rowTop + MENU_HEIGHT / 2;\n"
        "    display.getTextBounds(menuItems[i], 0, 0, &x1, &y1, &w, &h);\n"
        "    int16_t drawYPos = rowCenterY - y1 - h / 2;\n"
        "    if (i == menuIndex) {\n"
        "      display.fillRect(0, rowTop, 200, MENU_HEIGHT, GxEPD_BLACK);\n"
        "      display.setTextColor(GxEPD_WHITE);\n"
        "    } else {\n"
        "      display.setTextColor(GxEPD_BLACK);\n"
        "    }\n"
        "    uint16_t trailWidth = 0;\n"
        "    int16_t trailX = 198;\n"
        "    int16_t trailYPos = drawYPos;\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.getTextBounds(menuTrailItems[i], 0, 0, &x1, &y1, &trailWidth, &h);\n"
        "      trailX = 198 - trailWidth;\n"
        "      trailYPos = rowCenterY - y1 - h / 2;\n"
        "    }\n"
        "    if (i == 0) {\n"
        "      display.drawBitmap(1, rowCenterY - 8, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);\n"
        "      String wifiText(menuItems[i]);\n"
        "      int16_t wifiTextX = 24;\n"
        "      int16_t maxWifiTextWidth = trailX - wifiTextX - 4;\n"
        "      while (wifiText.length() > 0) {\n"
        "        display.getTextBounds(wifiText.c_str(), wifiTextX, drawYPos, &x1, &y1, &w, &h);\n"
        "        if (w <= maxWifiTextWidth) {\n"
        "          break;\n"
        "        }\n"
        "        wifiText.remove(wifiText.length() - 1);\n"
        "      }\n"
        "      display.setCursor(wifiTextX, drawYPos);\n"
        "      display.print(wifiText);\n"
        "    } else {\n"
        "      display.setCursor(0, drawYPos);\n"
        "      display.print(menuItems[i]);\n"
        "    }\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.setCursor(trailX, trailYPos);\n"
        "      display.print(menuTrailItems[i]);\n"
        "    }\n"
        "  }\n"
    )
    old_wrapped_menu_loop = (
        "  display.setTextWrap(false);\n"
        "  for (int i = 0; i < MENU_LENGTH; i++) {\n"
        "    int16_t rowTop = MENU_HEIGHT * i;\n"
        "    int16_t rowCenterY = rowTop + MENU_HEIGHT / 2;\n"
        "    display.getTextBounds(menuItems[i], 0, 0, &x1, &y1, &w, &h);\n"
        "    int16_t drawYPos = rowCenterY - y1 - h / 2;\n"
        "    if (i == menuIndex) {\n"
        "      display.fillRect(0, rowTop, 200, MENU_HEIGHT, GxEPD_BLACK);\n"
        "      display.setTextColor(GxEPD_WHITE);\n"
        "    } else {\n"
        "      display.setTextColor(GxEPD_BLACK);\n"
        "    }\n"
        "    uint16_t trailWidth = 0;\n"
        "    int16_t trailX = 198;\n"
        "    int16_t trailYPos = drawYPos;\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.getTextBounds(menuTrailItems[i], 0, 0, &x1, &y1, &trailWidth, &h);\n"
        "      trailX = 199 - x1 - trailWidth;\n"
        "      if (trailX < 0) {\n"
        "        trailX = 0;\n"
        "      }\n"
        "      trailYPos = rowCenterY - y1 - h / 2;\n"
        "    }\n"
        "    int16_t textX = i == 0 ? 24 : 0;\n"
        "    int16_t maxTextRight = menuTrailItems[i][0] != '\\0' ? trailX - 4 : 199;\n"
        "    String itemText(menuItems[i]);\n"
        "    while (itemText.length() > 0) {\n"
        "      display.getTextBounds(itemText.c_str(), textX, drawYPos, &x1, &y1, &w, &h);\n"
        "      if (x1 + w <= maxTextRight) {\n"
        "        break;\n"
        "      }\n"
        "      itemText.remove(itemText.length() - 1);\n"
        "    }\n"
        "    if (i == 0) {\n"
        "      display.drawBitmap(1, rowCenterY - 8, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);\n"
        "    }\n"
        "    display.setCursor(textX, drawYPos);\n"
        "    display.print(itemText);\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.setCursor(trailX, trailYPos);\n"
        "      display.print(menuTrailItems[i]);\n"
        "    }\n"
        "  }\n"
        "  display.setTextWrap(true);\n"
    )
    old_status_bar_menu_loop = (
        "  display.setTextWrap(false);\n"
        "  for (int i = 0; i < MENU_LENGTH; i++) {\n"
        "    int16_t rowTop = 24 + MENU_HEIGHT * i;\n"
        "    int16_t rowCenterY = rowTop + MENU_HEIGHT / 2;\n"
        "    display.getTextBounds(menuItems[i], 0, 0, &x1, &y1, &w, &h);\n"
        "    int16_t drawYPos = rowCenterY - y1 - h / 2;\n"
        "    if (i == menuIndex) {\n"
        "      display.fillRect(0, rowTop, 200, MENU_HEIGHT, GxEPD_BLACK);\n"
        "      display.setTextColor(GxEPD_WHITE);\n"
        "    } else {\n"
        "      display.setTextColor(GxEPD_BLACK);\n"
        "    }\n"
        "    uint16_t trailWidth = 0;\n"
        "    int16_t trailX = 198;\n"
        "    int16_t trailYPos = drawYPos;\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.getTextBounds(menuTrailItems[i], 0, 0, &x1, &y1, &trailWidth, &h);\n"
        "      trailX = 199 - x1 - trailWidth;\n"
        "      if (trailX < 0) {\n"
        "        trailX = 0;\n"
        "      }\n"
        "      trailYPos = rowCenterY - y1 - h / 2;\n"
        "    }\n"
        "    int16_t textX = i == 0 ? 24 : 0;\n"
        "    int16_t maxTextRight = menuTrailItems[i][0] != '\\0' ? trailX - 4 : 199;\n"
        "    String itemText(menuItems[i]);\n"
        "    while (itemText.length() > 0) {\n"
        "      display.getTextBounds(itemText.c_str(), textX, drawYPos, &x1, &y1, &w, &h);\n"
        "      if (x1 + w <= maxTextRight) {\n"
        "        break;\n"
        "      }\n"
        "      itemText.remove(itemText.length() - 1);\n"
        "    }\n"
        "    if (i == 0) {\n"
        "      display.drawBitmap(1, rowCenterY - 8, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);\n"
        "    }\n"
        "    display.setCursor(textX, drawYPos);\n"
        "    display.print(itemText);\n"
        "    if (menuTrailItems[i][0] != '\\0') {\n"
        "      display.setCursor(trailX, trailYPos);\n"
        "      display.print(menuTrailItems[i]);\n"
        "    }\n"
        "  }\n"
        "  display.setTextWrap(true);\n"
    )
    patched_menu_loop = old_status_bar_menu_loop.replace(
        "    int16_t textX = i == 0 ? 24 : 0;\n",
        "    int16_t textX = i == 0 ? 26 : 0;\n",
    ).replace(
        "      display.drawBitmap(1, rowCenterY - 8, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);\n",
        "      display.drawBitmap(3, rowCenterY - 8, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);\n",
    )
    loop_count = (
        text.count(old_menu_loop) +
        text.count(old_trailing_menu_loop) +
        text.count(old_wifi_menu_loop) +
        text.count(old_centered_wifi_menu_loop) +
        text.count(old_fixed_first_only_menu_loop) +
        text.count(old_uniform_menu_loop) +
        text.count(old_wrapped_menu_loop) +
        text.count(old_status_bar_menu_loop)
    )
    if loop_count > 0:
        text = text.replace(old_menu_loop, patched_menu_loop)
        text = text.replace(old_trailing_menu_loop, patched_menu_loop)
        text = text.replace(old_wifi_menu_loop, patched_menu_loop)
        text = text.replace(old_centered_wifi_menu_loop, patched_menu_loop)
        text = text.replace(old_fixed_first_only_menu_loop, patched_menu_loop)
        text = text.replace(old_uniform_menu_loop, patched_menu_loop)
        text = text.replace(old_wrapped_menu_loop, patched_menu_loop)
        text = text.replace(old_status_bar_menu_loop, patched_menu_loop)
        print(f"Watchy menu trailing indicator patch: applied {loop_count} time(s)")
    elif patched_menu_loop in text:
        print("Watchy menu trailing indicator patch: already applied")
    else:
        print("Watchy menu trailing indicator patch: target code was not found")

    total_case_count = 0
    already_patched = False
    for indent in ("      ", "          "):
        child_indent = indent + "  "
        original_case = (
            f"{indent}case 0:\n"
            f"{child_indent}showAbout();\n"
            f"{child_indent}break;\n"
            f"{indent}case 1:\n"
            f"{child_indent}showBuzz();\n"
            f"{child_indent}break;\n"
            f"{indent}case 2:\n"
            f"{child_indent}showAccelerometer();\n"
            f"{child_indent}break;\n"
            f"{indent}case 3:\n"
            f"{child_indent}setTime();\n"
            f"{child_indent}break;\n"
            f"{indent}case 4:\n"
            f"{child_indent}setupWifi();\n"
            f"{child_indent}break;\n"
            f"{indent}/*case 5:\n"
            f"{child_indent}showUpdateFW();\n"
            f"{child_indent}break;*/\n"
            f"{indent}case 5:\n"
            f"{child_indent}showSyncNTP();\n"
            f"{child_indent}break;\n"
            f"{indent}default:\n"
        )
        old_patched_case = (
            f"{indent}case 0:\n"
            f"{child_indent}showAbout();\n"
            f"{child_indent}break;\n"
            f"{indent}case 1:\n"
            f"{child_indent}showBuzz();\n"
            f"{child_indent}break;\n"
            f"{indent}case 2:\n"
            f"{child_indent}showAccelerometer();\n"
            f"{child_indent}break;\n"
            f"{indent}case 3:\n"
            f"{child_indent}setTime();\n"
            f"{child_indent}break;\n"
            f"{indent}case 4:\n"
            f"{child_indent}setupWifi();\n"
            f"{child_indent}break;\n"
            f"{indent}/*case 5:\n"
            f"{child_indent}showUpdateFW();\n"
            f"{child_indent}break;*/\n"
            f"{indent}case 5:\n"
            f"{child_indent}showSyncNTP();\n"
            f"{child_indent}break;\n"
            f"{indent}case 6:\n"
            f"{child_indent}// {NOTIFICATIONS_MENU_HOOK_PATCH_MARKER}\n"
            f"{child_indent}watchyNotificationsSelected(this);\n"
            f"{child_indent}return;\n"
            f"{indent}default:\n"
        )
        old_custom_case = (
            f"{indent}case 0:\n"
            f"{child_indent}setupWifi();\n"
            f"{child_indent}break;\n"
            f"{indent}case 1:\n"
            f"{child_indent}// {NOTIFICATIONS_MENU_HOOK_PATCH_MARKER}\n"
            f"{child_indent}watchyNotificationsSelected(this);\n"
            f"{child_indent}return;\n"
            f"{indent}case 2:\n"
            f"{child_indent}setTime();\n"
            f"{child_indent}break;\n"
            f"{indent}case 3:\n"
            f"{child_indent}showSyncNTP();\n"
            f"{child_indent}break;\n"
            f"{indent}case 4:\n"
            f"{child_indent}showAccelerometer();\n"
            f"{child_indent}break;\n"
            f"{indent}case 5:\n"
            f"{child_indent}showAbout();\n"
            f"{child_indent}break;\n"
            f"{indent}default:\n"
        )
        patched_case = (
            f"{indent}case 0:\n"
            f"{child_indent}setupWifi();\n"
            f"{child_indent}break;\n"
            f"{indent}case 1:\n"
            f"{child_indent}// {NOTIFICATIONS_MENU_HOOK_PATCH_MARKER}\n"
            f"{child_indent}watchyNotificationsSelected(this);\n"
            f"{child_indent}return;\n"
            f"{indent}case 2:\n"
            f"{child_indent}setTime();\n"
            f"{child_indent}break;\n"
            f"{indent}case 3:\n"
            f"{child_indent}showAccelerometer();\n"
            f"{child_indent}break;\n"
            f"{indent}case 4:\n"
            f"{child_indent}showAbout();\n"
            f"{child_indent}break;\n"
            f"{indent}default:\n"
        )
        legacy_patched_case = old_patched_case.replace(
            f"{child_indent}return;\n",
            f"{child_indent}break;\n",
        )
        case_count = (
            text.count(original_case) +
            text.count(old_patched_case) +
            text.count(old_custom_case)
        )
        if case_count > 0:
            text = text.replace(original_case, patched_case)
            text = text.replace(old_patched_case, patched_case)
            text = text.replace(old_custom_case, patched_case)
            total_case_count += case_count
        else:
            legacy_case_count = text.count(legacy_patched_case)
            if legacy_case_count > 0:
                text = text.replace(legacy_patched_case, patched_case)
                total_case_count += legacy_case_count
                continue

            if patched_case in text:
                already_patched = True

    if total_case_count > 0:
        print(f"Watchy notifications menu switch patch: applied {total_case_count} time(s)")
    elif already_patched:
        print("Watchy notifications menu switch patch: already applied")
    else:
        print("Watchy notifications menu switch patch: target code was not found")

    return text


def patch_deep_sleep_hook(text):
    original = "  deepSleep();\n}\nvoid Watchy::deepSleep() {\n"
    patched = (
        f"  // {DEEP_SLEEP_HOOK_PATCH_MARKER}\n"
        "  if (watchyShouldDeepSleep(this)) {\n"
        "    deepSleep();\n"
        "  }\n"
        "}\n"
        "void Watchy::deepSleep() {\n"
    )
    return replace_or_log(text, original, patched, "Watchy deep sleep hook patch")


def patch_menu_partial_refresh(text):
    if "showMenu(menuIndex, false)" in text:
        text = text.replace("showMenu(menuIndex, false)", "showMenu(menuIndex, true)")
        print("Watchy menu partial refresh patch: applied")
    elif "showMenu(menuIndex, true)" in text:
        print("Watchy menu partial refresh patch: already applied")

    replacements = (
        ("display.display(false); // full refresh", "display.display(true); // partial refresh"),
        ("display.display(true); // full refresh", "display.display(true); // partial refresh"),
    )
    for original, patched in replacements:
        if original in text:
            text = text.replace(original, patched)
            print("Watchy menu app partial refresh patch: applied")

    if MENU_WATCHFACE_PARTIAL_REFRESH_PATCH_MARKER in text:
        print("Watchy menu watchface partial refresh patch: already applied")
        return text

    for original, patched in (
        (
            "        guiState = WATCHFACE_STATE;\n"
            "        showWatchFace(false);\n",
            "        guiState = WATCHFACE_STATE;\n"
            f"        // {MENU_WATCHFACE_PARTIAL_REFRESH_PATCH_MARKER}\n"
            "        showWatchFace(true);\n",
        ),
        (
            "      RTC.read(currentTime);\n"
            "      showWatchFace(false);\n"
            "    } else if (guiState == APP_STATE) {\n",
            "      RTC.read(currentTime);\n"
            f"      // {MENU_WATCHFACE_PARTIAL_REFRESH_PATCH_MARKER}\n"
            "      showWatchFace(true);\n"
            "    } else if (guiState == APP_STATE) {\n",
        ),
        (
            "          RTC.read(currentTime);\n"
            "          showWatchFace(false);\n"
            "          break; // leave loop\n",
            "          RTC.read(currentTime);\n"
            f"          // {MENU_WATCHFACE_PARTIAL_REFRESH_PATCH_MARKER}\n"
            "          showWatchFace(true);\n"
            "          break; // leave loop\n",
        ),
    ):
        text = text.replace(original, patched, 1)

    print("Watchy menu watchface partial refresh patch: applied")
    return text


def patch_white_menu_theme(text):
    original_text = text

    replacements = (
        ("display.fillScreen(GxEPD_BLACK);", "display.fillScreen(GxEPD_WHITE);"),
        ("display.setTextColor(GxEPD_WHITE);", "display.setTextColor(GxEPD_BLACK);"),
        (
            "display.setTextColor(blink ? GxEPD_WHITE : GxEPD_BLACK);",
            "display.setTextColor(blink ? GxEPD_BLACK : GxEPD_WHITE);",
        ),
    )
    for original, patched in replacements:
        text = text.replace(original, patched)

    for fill_color in ("GxEPD_WHITE", "GxEPD_BLACK"):
        text = text.replace(
            f"      display.fillRect(x1 - 1, y1 - 10, 200, h + 15, {fill_color});\n"
            "      display.setTextColor(GxEPD_BLACK);\n",
            "      display.fillRect(x1 - 1, y1 - 10, 200, h + 15, GxEPD_BLACK);\n"
            "      display.setTextColor(GxEPD_WHITE);\n",
        )
        text = text.replace(
            f"      display.fillRect(0, y1 - 10, 200, h + 15, {fill_color});\n"
            "      display.setTextColor(GxEPD_BLACK);\n",
            "      display.fillRect(0, y1 - 10, 200, h + 15, GxEPD_BLACK);\n"
            "      display.setTextColor(GxEPD_WHITE);\n",
        )
        text = text.replace(
            f"      display.fillRect(0, MENU_HEIGHT * i, 200, MENU_HEIGHT, {fill_color});\n"
            "      display.setTextColor(GxEPD_BLACK);\n",
            "      display.fillRect(0, MENU_HEIGHT * i, 200, MENU_HEIGHT, GxEPD_BLACK);\n"
            "      display.setTextColor(GxEPD_WHITE);\n",
        )
        text = text.replace(
            f"      display.fillRect(0, rowTop, 200, MENU_HEIGHT, {fill_color});\n"
            "      display.setTextColor(GxEPD_BLACK);\n",
            "      display.fillRect(0, rowTop, 200, MENU_HEIGHT, GxEPD_BLACK);\n"
            "      display.setTextColor(GxEPD_WHITE);\n",
        )

    if text == original_text:
        print("Watchy white menu theme patch: already applied")
    else:
        print("Watchy white menu theme patch: applied")
    return text


def patch_menu_font(text):
    original_text = text

    font_include = "#include <Fonts/FreeMonoBold12pt7b.h>\n"
    if font_include in text:
        text = text.replace(font_include, "")

    for signature in (
        "void Watchy::showMenu(byte menuIndex, bool partialRefresh)",
        "void Watchy::showFastMenu(byte menuIndex)",
    ):
        text = text.replace(
            f"{signature} {{\n"
            "  display.setFullWindow();\n"
            "  display.fillScreen(GxEPD_WHITE);\n"
            "  display.setFont(&FreeMonoBold12pt7b);\n",
            f"{signature} {{\n"
            "  display.setFullWindow();\n"
            "  display.fillRect(0, 24, 200, 176, GxEPD_WHITE);\n"
            "  display.setFont(&FreeMonoBold9pt7b);\n",
        )
        text = text.replace(
            f"{signature} {{\n"
            "  display.setFullWindow();\n"
            "  display.fillScreen(GxEPD_WHITE);\n"
            "  display.setFont(&FreeMonoBold9pt7b);\n",
            f"{signature} {{\n"
            "  display.setFullWindow();\n"
            "  display.fillRect(0, 24, 200, 176, GxEPD_WHITE);\n"
            "  display.setFont(&FreeMonoBold9pt7b);\n",
        )
    text = text.replace(
        "  display.setTextWrap(true);\n\n"
        "  display.display(partialRefresh);\n",
        "  display.setTextWrap(true);\n\n"
        "  display.displayWindow(0, 24, 200, 176);\n",
    )
    text = text.replace(
        "  display.setTextWrap(true);\n\n"
        "  display.display(true);\n",
        "  display.setTextWrap(true);\n\n"
        "  display.displayWindow(0, 24, 200, 176);\n",
    )

    if text == original_text:
        print("Watchy menu font/area patch: already applied")
    else:
        print("Watchy menu font/area patch: applied")
    return text


def patch_reset_vibration(text):
    if RESET_VIBRATION_PATCH_MARKER in text:
        print("Watchy reset vibration patch: already applied")
        return text

    original = (
        "    showWatchFace(false); // full update on reset\n"
        "    vibMotor(75, 4);\n"
        "    // For some reason, seems to be enabled on first boot\n"
    )
    patched = (
        "    showWatchFace(false); // full update on reset\n"
        f"    // {RESET_VIBRATION_PATCH_MARKER}\n"
        "    // vibMotor(75, 4);\n"
        "    // For some reason, seems to be enabled on first boot\n"
    )
    return replace_or_log(text, original, patched, "Watchy reset vibration patch")


def patch_reset_watchface_refresh(text):
    original = "    showWatchFace(false); // full update on reset\n"
    patched = (
        f"    // {RESET_WATCHFACE_PARTIAL_PATCH_MARKER}\n"
        "    showWatchFace(true); // partial update on reset/upload\n"
    )
    return replace_or_log(text, original, patched, "Watchy reset watchface refresh patch")


def patch_display_cpp(display_cpp):
    text = display_cpp.read_text()
    original_text = text

    text = normalize_marker_paths(text)
    text = patch_initial_full_refresh(text)
    text = patch_forced_initial_partial_refresh(text)

    if text != original_text:
        display_cpp.write_text(text)


def patch_config_h(config_h):
    text = config_h.read_text()
    original_text = text

    patched_height = "#define MENU_HEIGHT     30\n"
    height_patched = False
    for original_height in (
        "#define MENU_HEIGHT     25\n",
        "#define MENU_HEIGHT     38\n",
        "#define MENU_HEIGHT     35\n",
    ):
        if original_height in text:
            text = text.replace(original_height, patched_height)
            print("Watchy menu height patch: set to 30")
            height_patched = True
            break
    if not height_patched and patched_height in text:
        print("Watchy menu height patch: already applied")
    elif not height_patched:
        print("Watchy menu height patch: target code was not found")

    original = "#define MENU_LENGTH     6\n"
    patched = (
        f"// {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "#define MENU_LENGTH     5\n"
    )
    old_custom_patched = (
        f"// {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "#define MENU_LENGTH     6\n"
    )
    legacy_patched = (
        f"// {MENU_NOTIFICATIONS_ITEM_PATCH_MARKER}\n"
        "#define MENU_LENGTH     7\n"
    )
    if old_custom_patched in text:
        text = text.replace(old_custom_patched, patched)
        print("Watchy menu length patch: reduced to custom 5-item menu")
    elif legacy_patched in text:
        text = text.replace(legacy_patched, patched)
        print("Watchy menu length patch: reduced to custom 5-item menu")
    elif patched in text:
        print("Watchy menu length patch: already applied")
    else:
        text = replace_or_log(text, original, patched, "Watchy menu length patch")

    if text != original_text:
        config_h.write_text(text)


def patch_initial_full_refresh(text):
    original = (
        "void WatchyDisplay::initWatchy() {\n"
        "  // Watchy default initialization\n"
        "  init(0, displayFullInit, 2, true);\n"
        "}\n"
    )
    patched = (
        "void WatchyDisplay::initWatchy() {\n"
        f"  // {DISPLAY_NO_INITIAL_FULL_REFRESH_PATCH_MARKER}\n"
        "  displayFullInit = false;\n"
        "  init(0, false, 2, true);\n"
        "}\n"
    )
    return replace_or_log(text, original, patched, "Watchy display initial full refresh patch")


def patch_forced_initial_partial_refresh(text):
    original = "  if (_initial_refresh) return refresh(false); // initial update needs be full update\n"
    patched = (
        f"  // {DISPLAY_FORCED_INITIAL_PARTIAL_REFRESH_PATCH_MARKER}\n"
        "  if (_initial_refresh) _initial_refresh = false;\n"
    )
    return replace_or_log(text, original, patched, "Watchy display forced initial partial refresh patch")


def replace_or_log(text, original, patched, label):
    if patched in text:
        print(f"{label}: already applied")
        return text
    if original in text:
        print(f"{label}: applied")
        return text.replace(original, patched, 1)
    print(f"{label}: target code was not found")
    return text


patch_watchy()
env.AddPreAction("buildprog", patch_watchy)
