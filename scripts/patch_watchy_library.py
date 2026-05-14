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
QUIET_HOURS_VIBRATION_PATCH_MARKER = f"Hourly vibration quiet hours added by {SCRIPT_PATH}"
MENU_WATCHFACE_PARTIAL_REFRESH_PATCH_MARKER = (
    f"Menu returns switched to partial watchface refresh by {SCRIPT_PATH}"
)


def patch_watchy(*_args, **_kwargs):
    project_dir = Path(env.subst("$PROJECT_DIR"))
    pio_env = env.subst("$PIOENV")

    watchy_cpp = find_lib_file(project_dir, pio_env, "Watchy.cpp")
    if watchy_cpp is None:
        print("Watchy patch: Watchy.cpp not found yet")
    else:
        patch_watchy_cpp(watchy_cpp)

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
    text = patch_minute_tick(text)
    text = patch_quiet_hours_vibration(text)
    text = patch_wifi_configured_hook(text)
    text = patch_menu_partial_refresh(text)
    text = patch_white_menu_theme(text)
    text = patch_reset_vibration(text)
    text = patch_reset_watchface_refresh(text)

    if text != original_text:
        watchy_cpp.write_text(text)


def add_event_hooks(text):
    if MINUTE_TICK_HOOK_PATCH_MARKER in text and WIFI_CONFIGURED_HOOK_PATCH_MARKER in text:
        print("Watchy event hook patch: already applied")
        return text

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

    if text == original_text:
        print("Watchy white menu theme patch: already applied")
    else:
        print("Watchy white menu theme patch: applied")
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
