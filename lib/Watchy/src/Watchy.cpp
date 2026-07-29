#include "Watchy.h"

#ifndef CITYWEATHER_VERSION
#define CITYWEATHER_VERSION "unknown"
#endif

#ifdef ARDUINO_ESP32S3_DEV
  Watchy32KRTC Watchy::RTC;
  #define ACTIVE_LOW 0
#else
  WatchyRTC Watchy::RTC;
  #define ACTIVE_LOW 1
#endif
GxEPD2_BW<WatchyDisplay, WatchyDisplay::HEIGHT> Watchy::display(
    WatchyDisplay{});

RTC_DATA_ATTR int guiState;
RTC_DATA_ATTR int menuIndex;
RTC_DATA_ATTR BMA423 sensor;
RTC_DATA_ATTR bool WIFI_CONFIGURED;
RTC_DATA_ATTR bool BLE_CONFIGURED;
RTC_DATA_ATTR weatherData currentWeather;
RTC_DATA_ATTR int weatherIntervalCounter = -1;
RTC_DATA_ATTR long gmtOffset = 0;
RTC_DATA_ATTR bool alreadyInMenu         = true;
static int previousFastMenuIndex = -1;
RTC_DATA_ATTR bool USB_PLUGGED_IN = false;
RTC_DATA_ATTR tmElements_t bootTime;
RTC_DATA_ATTR uint32_t lastIPAddress;
RTC_DATA_ATTR char lastSSID[30];

void Watchy::init(String datetime) {
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause(); // get wake up reason
  #ifdef ARDUINO_ESP32S3_DEV
    Wire.begin(WATCHY_V3_SDA, WATCHY_V3_SCL);     // init i2c
  #else
    Wire.begin(SDA, SCL);                         // init i2c
  #endif
  RTC.init();
  // Init the display since is almost sure we will use it
  display.epd2.initWatchy();

  switch (wakeup_reason) {
  #ifdef ARDUINO_ESP32S3_DEV
  case ESP_SLEEP_WAKEUP_TIMER: // RTC Alarm
  #else
  case ESP_SLEEP_WAKEUP_EXT0: // RTC Alarm
  #endif
    RTC.read(currentTime);
    switch (guiState) {
    case WATCHFACE_STATE:
      onMinuteTick(); // partial updates on tick
      if (settings.vibrateOClock) {
        if (currentTime.Minute == 0 && currentTime.Hour >= 10 && currentTime.Hour < 22) {
          // The RTC wakes us up once per minute
          vibMotor(75, 4);
        }
      }
      break;
    case APP_STATE:
      onAppTick();
      break;
    case MAIN_MENU_STATE:
      // Return to watchface if in menu for more than one tick
      if (alreadyInMenu) {
        guiState = WATCHFACE_STATE;
        showWatchFace(true);
      } else {
        alreadyInMenu = true;
      }
      break;
    }
    break;
  case ESP_SLEEP_WAKEUP_EXT1: // button Press
    handleButtonPress();
    break;
  #ifdef ARDUINO_ESP32S3_DEV
  case ESP_SLEEP_WAKEUP_EXT0: // USB plug in
    pinMode(USB_DET_PIN, INPUT);
    USB_PLUGGED_IN = (digitalRead(USB_DET_PIN) == 1);
    if(guiState == WATCHFACE_STATE){
      RTC.read(currentTime);
      showWatchFace(true);
    }
    break;
  #endif
  default: // reset
    RTC.config(datetime);
    _bmaConfig();
    #ifdef ARDUINO_ESP32S3_DEV
    pinMode(USB_DET_PIN, INPUT);
    USB_PLUGGED_IN = (digitalRead(USB_DET_PIN) == 1);
    #endif    
    gmtOffset = settings.gmtOffset;
    RTC.read(currentTime);
    RTC.read(bootTime);
    showWatchFace(true); // partial update on reset/upload
    // vibMotor(75, 4);
    // For some reason, seems to be enabled on first boot
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    break;
  }
  if (shouldDeepSleep()) {
    deepSleep();
  }
}
void Watchy::deepSleep() {
  display.hibernate();
  RTC.clearAlarm();        // resets the alarm flag in the RTC
  #ifdef ARDUINO_ESP32S3_DEV
  esp_sleep_enable_ext0_wakeup((gpio_num_t)USB_DET_PIN, USB_PLUGGED_IN ? LOW : HIGH); //// enable deep sleep wake on USB plug in/out
  rtc_gpio_set_direction((gpio_num_t)USB_DET_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_en((gpio_num_t)USB_DET_PIN);

  esp_sleep_enable_ext1_wakeup(
      BTN_PIN_MASK,
      ESP_EXT1_WAKEUP_ANY_LOW); // enable deep sleep wake on button press
  rtc_gpio_set_direction((gpio_num_t)UP_BTN_PIN, RTC_GPIO_MODE_INPUT_ONLY);
  rtc_gpio_pullup_en((gpio_num_t)UP_BTN_PIN);

  rtc_clk_32k_enable(true);
  //rtc_clk_slow_freq_set(RTC_SLOW_FREQ_32K_XTAL);
  struct tm timeinfo;
  getLocalTime(&timeinfo);
  int secToNextMin = 60 - timeinfo.tm_sec;
  esp_sleep_enable_timer_wakeup(secToNextMin * uS_TO_S_FACTOR);
  #else
  // Set GPIOs 0-39 to input to avoid power leaking out
  const uint64_t ignore = 0b11110001000000110000100111000010; // Ignore some GPIOs due to resets
  for (int i = 0; i < GPIO_NUM_MAX; i++) {
    if ((ignore >> i) & 0b1)
      continue;
    pinMode(i, INPUT);
  }
  esp_sleep_enable_ext0_wakeup((gpio_num_t)RTC_INT_PIN,
                               0); // enable deep sleep wake on RTC interrupt
  esp_sleep_enable_ext1_wakeup(
      BTN_PIN_MASK,
      ESP_EXT1_WAKEUP_ANY_HIGH); // enable deep sleep wake on button press
  #endif
  esp_deep_sleep_start();
}

void Watchy::handleButtonPress() {
  uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
  if ((wakeupBit & MENU_BTN_MASK) && screenshotRequested()) {
    return;
  }
  // Menu Button
  if (wakeupBit & MENU_BTN_MASK) {
    if (guiState ==
        WATCHFACE_STATE) { // enter menu state if coming from watch face
      showMenu(menuIndex, true);
    } else if (guiState ==
               MAIN_MENU_STATE) { // if already in menu, then select menu item
      switch (menuIndex) {
      case 0:
        setupWifi();
        break;
      case 1:
        onNotificationsSelected();
        return;
      case 2:
        setTime();
        break;
      case 3:
        showAccelerometer();
        break;
      case 4:
        if (handleAbout()) {
          return;
        }
        break;
      default:
        break;
      }
    } /*else if (guiState == FW_UPDATE_STATE) {
      updateFWBegin();
    }*/
  }
  // Back Button
  else if (wakeupBit & BACK_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) { // exit to watch face if already in menu
      RTC.read(currentTime);
      showWatchFace(true);
    } else if (guiState == APP_STATE) {
      showMenu(menuIndex, true); // exit to menu if already in app
    } else if (guiState == FW_UPDATE_STATE) {
      showMenu(menuIndex, true); // exit to menu if already in app
    } else if (guiState == WATCHFACE_STATE) {
      return;
    }
  }
  // Up Button
  else if (wakeupBit & UP_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) { // increment menu index
      menuIndex--;
      if (menuIndex < 0) {
        menuIndex = MENU_LENGTH - 1;
      }
      showMenu(menuIndex, true);
    } else if (guiState == WATCHFACE_STATE) {
      return;
    }
  }
  // Down Button
  else if (wakeupBit & DOWN_BTN_MASK) {
    if (guiState == MAIN_MENU_STATE) { // decrement menu index
      menuIndex++;
      if (menuIndex > MENU_LENGTH - 1) {
        menuIndex = 0;
      }
      showMenu(menuIndex, true);
    } else if (guiState == WATCHFACE_STATE) {
      return;
    }
  }

  /***************** fast menu *****************/
  bool timeout     = false;
  long lastTimeout = millis();
  pinMode(MENU_BTN_PIN, INPUT);
  pinMode(BACK_BTN_PIN, INPUT);
  pinMode(UP_BTN_PIN, INPUT);
  pinMode(DOWN_BTN_PIN, INPUT);
  while (!timeout) {
    onMenuLoop();
    if (millis() - lastTimeout > 5000) {
      timeout = true;
    } else {
      if (digitalRead(MENU_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (screenshotRequested()) {
          continue;
        }
        if (guiState ==
            MAIN_MENU_STATE) { // if already in menu, then select menu item
          switch (menuIndex) {
          case 0:
            setupWifi();
            break;
          case 1:
            onNotificationsSelected();
            return;
          case 2:
            setTime();
            break;
          case 3:
            showAccelerometer();
            break;
          case 4:
            showAbout();
            break;
          default:
            break;
          }
        }/* else if (guiState == FW_UPDATE_STATE) {
          updateFWBegin();
        }*/
      } else if (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (guiState ==
            MAIN_MENU_STATE) { // exit to watch face if already in menu
          RTC.read(currentTime);
          showWatchFace(true);
          break; // leave loop
        } else if (guiState == APP_STATE) {
          showMenu(menuIndex, true); // exit to menu if already in app
        } else if (guiState == FW_UPDATE_STATE) {
          showMenu(menuIndex, true); // exit to menu if already in app
        }
      } else if (digitalRead(UP_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) { // increment menu index
          menuIndex--;
          if (menuIndex < 0) {
            menuIndex = MENU_LENGTH - 1;
          }
          showFastMenu(menuIndex);
        }
      } else if (digitalRead(DOWN_BTN_PIN) == ACTIVE_LOW) {
        lastTimeout = millis();
        if (guiState == MAIN_MENU_STATE) { // decrement menu index
          menuIndex++;
          if (menuIndex > MENU_LENGTH - 1) {
            menuIndex = 0;
          }
          showFastMenu(menuIndex);
        }
      }
    }
  }
}

void Watchy::showMenu(byte menuIndex, bool partialRefresh) {
  display.setFullWindow();
  display.fillRect(0, 24, 200, 176, GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);

  int16_t x1, y1;
  uint16_t w, h;
  int16_t yPos;

  static const unsigned char menuWifiIcon[] PROGMEM = {
      0x01, 0xf0, 0x00, 0x07, 0xfc, 0x00, 0x1e, 0x0f,
      0x00, 0x39, 0xf3, 0x80, 0x77, 0xfd, 0xc0, 0xef,
      0x1e, 0xe0, 0x5c, 0xe7, 0x40, 0x3b, 0xfb, 0x80,
      0x17, 0x1d, 0x00, 0x0e, 0xee, 0x00, 0x05, 0xf4,
      0x00, 0x03, 0xb8, 0x00, 0x01, 0x50, 0x00, 0x00,
      0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00
  };
  static const unsigned char menuBluetoothIcon[] PROGMEM = {
      0x00, 0x60, 0x00, 0x00, 0x70, 0x00, 0x00, 0x78,
      0x00, 0x06, 0x6c, 0x00, 0x06, 0x66, 0x00, 0x03,
      0x7c, 0x00, 0x01, 0xf8, 0x00, 0x00, 0xe0, 0x00,
      0x00, 0xe0, 0x00, 0x01, 0xf8, 0x00, 0x03, 0x7c,
      0x00, 0x06, 0x66, 0x00, 0x06, 0x6c, 0x00, 0x00,
      0x78, 0x00, 0x00, 0x70, 0x00, 0x00, 0x60, 0x00
  };
  static const unsigned char menuDateTimeIcon[] PROGMEM = {
      0x06, 0x30, 0x00, 0x3f, 0xff, 0x00, 0x3f, 0xff,
      0x00, 0x3f, 0xff, 0x00, 0x20, 0x03, 0x00, 0x20,
      0x03, 0x00, 0x23, 0xe3, 0x00, 0x26, 0x33, 0x00,
      0x24, 0x93, 0x00, 0x24, 0xd3, 0x00, 0x24, 0x53,
      0x00, 0x26, 0x33, 0x00, 0x23, 0xe3, 0x00, 0x20,
      0x03, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x00, 0x00
  };
  static const unsigned char menuAccelerometerIcon[] PROGMEM = {
      0x00, 0x80, 0x00, 0x01, 0xc0, 0x00, 0x03, 0xe0,
      0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00,
      0x80, 0x00, 0x10, 0x82, 0x00, 0x38, 0x87, 0x00,
      0x7f, 0xff, 0x80, 0x38, 0x87, 0x00, 0x10, 0x82,
      0x00, 0x00, 0x80, 0x00, 0x01, 0xc0, 0x00, 0x07,
      0x70, 0x00, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00
  };
  static const unsigned char menuAboutIcon[] PROGMEM = {
      0x07, 0xf0, 0x00, 0x1f, 0xf8, 0x00, 0x3c, 0x7c,
      0x00, 0x70, 0x1c, 0x00, 0x61, 0xc6, 0x00, 0xc1,
      0xc3, 0x00, 0xc0, 0x03, 0x00, 0xc0, 0x83, 0x00,
      0xc0, 0x83, 0x00, 0xc0, 0x83, 0x00, 0xc0, 0x83,
      0x00, 0x61, 0xc6, 0x00, 0x70, 0x1c, 0x00, 0x3c,
      0x7c, 0x00, 0x1f, 0xf8, 0x00, 0x07, 0xf0, 0x00
  };
  char bluetoothPushState[6];
  snprintf(
      bluetoothPushState,
      sizeof(bluetoothPushState),
      "[%s]",
      notificationsEnabled() ? "ON" : "OFF"
  );
  const char *menuItems[] = {
      lastSSID[0] != '\0' ? lastSSID : "Wi-Fi", "Bluetooth",
      "Set Date/Time", "Accelerometer", "About"};
  const char *menuTrailItems[] = {
      ">", bluetoothPushState, ">", ">", ">"};
  display.setTextWrap(false);
  for (int i = 0; i < MENU_LENGTH; i++) {
    int16_t rowTop = 24 + MENU_HEIGHT * i;
    int16_t rowCenterY = rowTop + MENU_HEIGHT / 2;
    display.getTextBounds(menuItems[i], 0, 0, &x1, &y1, &w, &h);
    int16_t drawYPos = rowCenterY - y1 - h / 2;
    if (i == menuIndex) {
      display.fillRect(0, rowTop, 200, MENU_HEIGHT, GxEPD_BLACK);
      display.setTextColor(GxEPD_WHITE);
    } else {
      display.setTextColor(GxEPD_BLACK);
    }
    uint16_t trailWidth = 0;
    int16_t trailX = 196;
    int16_t trailYPos = drawYPos;
    if (menuTrailItems[i][0] != '\0') {
      display.getTextBounds(menuTrailItems[i], 0, 0, &x1, &y1, &trailWidth, &h);
      trailX = 196 - x1 - trailWidth;
      if (trailX < 0) {
        trailX = 0;
      }
      trailYPos = rowCenterY - y1 - h / 2;
    }
    int16_t textX = 26;
    int16_t maxTextRight = menuTrailItems[i][0] != '\0' ? trailX - 4 : 196;
    String itemText(menuItems[i]);
    while (itemText.length() > 0) {
      display.getTextBounds(itemText.c_str(), textX, drawYPos, &x1, &y1, &w, &h);
      if (x1 + w <= maxTextRight) {
        break;
      }
      itemText.remove(itemText.length() - 1);
    }
    if (i == 0) {
      display.drawBitmap(3, rowCenterY - 8, menuWifiIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 1) {
      display.drawBitmap(3, rowCenterY - 8, menuBluetoothIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 2) {
      display.drawBitmap(3, rowCenterY - 8, menuDateTimeIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 3) {
      display.drawBitmap(3, rowCenterY - 8, menuAccelerometerIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 4) {
      display.drawBitmap(3, rowCenterY - 8, menuAboutIcon, 19, 16, i == menuIndex ? GxEPD_WHITE : GxEPD_BLACK);
    }
    display.setCursor(textX, drawYPos);
    display.print(itemText);
    if (menuTrailItems[i][0] != '\0') {
      display.setCursor(trailX, trailYPos);
      display.print(menuTrailItems[i]);
    }
  }
  display.setTextWrap(true);

  onMenuShown();
  display.displayWindow(0, 0, 200, 200);

  guiState = MAIN_MENU_STATE;
  previousFastMenuIndex = menuIndex;
  alreadyInMenu = false;
}

void Watchy::showFastMenu(byte menuIndex) {
  display.setFullWindow();
  display.setFont(&FreeMonoBold9pt7b);

  int16_t x1, y1;
  uint16_t w, h;

  static const unsigned char menuWifiIcon[] PROGMEM = {
      0x01, 0xf0, 0x00, 0x07, 0xfc, 0x00, 0x1e, 0x0f,
      0x00, 0x39, 0xf3, 0x80, 0x77, 0xfd, 0xc0, 0xef,
      0x1e, 0xe0, 0x5c, 0xe7, 0x40, 0x3b, 0xfb, 0x80,
      0x17, 0x1d, 0x00, 0x0e, 0xee, 0x00, 0x05, 0xf4,
      0x00, 0x03, 0xb8, 0x00, 0x01, 0x50, 0x00, 0x00,
      0xe0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00
  };
  static const unsigned char menuBluetoothIcon[] PROGMEM = {
      0x00, 0x60, 0x00, 0x00, 0x70, 0x00, 0x00, 0x78,
      0x00, 0x06, 0x6c, 0x00, 0x06, 0x66, 0x00, 0x03,
      0x7c, 0x00, 0x01, 0xf8, 0x00, 0x00, 0xe0, 0x00,
      0x00, 0xe0, 0x00, 0x01, 0xf8, 0x00, 0x03, 0x7c,
      0x00, 0x06, 0x66, 0x00, 0x06, 0x6c, 0x00, 0x00,
      0x78, 0x00, 0x00, 0x70, 0x00, 0x00, 0x60, 0x00
  };
  static const unsigned char menuDateTimeIcon[] PROGMEM = {
      0x06, 0x30, 0x00, 0x3f, 0xff, 0x00, 0x3f, 0xff,
      0x00, 0x3f, 0xff, 0x00, 0x20, 0x03, 0x00, 0x20,
      0x03, 0x00, 0x23, 0xe3, 0x00, 0x26, 0x33, 0x00,
      0x24, 0x93, 0x00, 0x24, 0xd3, 0x00, 0x24, 0x53,
      0x00, 0x26, 0x33, 0x00, 0x23, 0xe3, 0x00, 0x20,
      0x03, 0x00, 0x3f, 0xff, 0x00, 0x00, 0x00, 0x00
  };
  static const unsigned char menuAccelerometerIcon[] PROGMEM = {
      0x00, 0x80, 0x00, 0x01, 0xc0, 0x00, 0x03, 0xe0,
      0x00, 0x00, 0x80, 0x00, 0x00, 0x80, 0x00, 0x00,
      0x80, 0x00, 0x10, 0x82, 0x00, 0x38, 0x87, 0x00,
      0x7f, 0xff, 0x80, 0x38, 0x87, 0x00, 0x10, 0x82,
      0x00, 0x00, 0x80, 0x00, 0x01, 0xc0, 0x00, 0x07,
      0x70, 0x00, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00
  };
  static const unsigned char menuAboutIcon[] PROGMEM = {
      0x07, 0xf0, 0x00, 0x1f, 0xf8, 0x00, 0x3c, 0x7c,
      0x00, 0x70, 0x1c, 0x00, 0x61, 0xc6, 0x00, 0xc1,
      0xc3, 0x00, 0xc0, 0x03, 0x00, 0xc0, 0x83, 0x00,
      0xc0, 0x83, 0x00, 0xc0, 0x83, 0x00, 0xc0, 0x83,
      0x00, 0x61, 0xc6, 0x00, 0x70, 0x1c, 0x00, 0x3c,
      0x7c, 0x00, 0x1f, 0xf8, 0x00, 0x07, 0xf0, 0x00
  };
  char bluetoothPushState[6];
  snprintf(
      bluetoothPushState,
      sizeof(bluetoothPushState),
      "[%s]",
      notificationsEnabled() ? "ON" : "OFF"
  );
  const char *menuItems[] = {
      lastSSID[0] != '\0' ? lastSSID : "Wi-Fi", "Bluetooth",
      "Set Date/Time", "Accelerometer", "About"};
  const char *menuTrailItems[] = {
      ">", bluetoothPushState, ">", ">", ">"};

  int selectedIndex = menuIndex;
  int previousIndex = previousFastMenuIndex;
  if (previousIndex < 0 || previousIndex >= MENU_LENGTH) {
    previousIndex = selectedIndex;
  }

  int previousTop = 24 + MENU_HEIGHT * previousIndex;
  int selectedTop = 24 + MENU_HEIGHT * selectedIndex;
  int updateTop = previousTop < selectedTop ? previousTop : selectedTop;
  int updateBottom = previousTop > selectedTop ? previousTop + MENU_HEIGHT : selectedTop + MENU_HEIGHT;
  int distance = selectedIndex - previousIndex;
  if (distance < 0) {
    distance = -distance;
  }

  display.setTextWrap(false);
  auto drawMenuRow = [&](int i, bool selected, bool clearBackground) {
    int16_t rowTop = 24 + MENU_HEIGHT * i;
    int16_t rowCenterY = rowTop + MENU_HEIGHT / 2;
    display.getTextBounds(menuItems[i], 0, 0, &x1, &y1, &w, &h);
    int16_t drawYPos = rowCenterY - y1 - h / 2;
    if (clearBackground) {
      display.fillRect(0, rowTop, 200, MENU_HEIGHT, selected ? GxEPD_BLACK : GxEPD_WHITE);
    }
    display.setTextColor(selected ? GxEPD_WHITE : GxEPD_BLACK);

    uint16_t trailWidth = 0;
    int16_t trailX = 196;
    int16_t trailYPos = drawYPos;
    if (menuTrailItems[i][0] != '\0') {
      display.getTextBounds(menuTrailItems[i], 0, 0, &x1, &y1, &trailWidth, &h);
      trailX = 196 - x1 - trailWidth;
      if (trailX < 0) {
        trailX = 0;
      }
      trailYPos = rowCenterY - y1 - h / 2;
    }

    int16_t textX = 26;
    int16_t maxTextRight = menuTrailItems[i][0] != '\0' ? trailX - 4 : 196;
    String itemText(menuItems[i]);
    while (itemText.length() > 0) {
      display.getTextBounds(itemText.c_str(), textX, drawYPos, &x1, &y1, &w, &h);
      if (x1 + w <= maxTextRight) {
        break;
      }
      itemText.remove(itemText.length() - 1);
    }

    if (i == 0) {
      display.drawBitmap(3, rowCenterY - 8, menuWifiIcon, 19, 16, selected ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 1) {
      display.drawBitmap(3, rowCenterY - 8, menuBluetoothIcon, 19, 16, selected ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 2) {
      display.drawBitmap(3, rowCenterY - 8, menuDateTimeIcon, 19, 16, selected ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 3) {
      display.drawBitmap(3, rowCenterY - 8, menuAccelerometerIcon, 19, 16, selected ? GxEPD_WHITE : GxEPD_BLACK);
    } else if (i == 4) {
      display.drawBitmap(3, rowCenterY - 8, menuAboutIcon, 19, 16, selected ? GxEPD_WHITE : GxEPD_BLACK);
    }
    display.setCursor(textX, drawYPos);
    display.print(itemText);
    if (menuTrailItems[i][0] != '\0') {
      display.setCursor(trailX, trailYPos);
      display.print(menuTrailItems[i]);
    }
  };

  if (distance == 1) {
    for (int frame = 1; frame <= 1; frame++) {
      display.fillRect(0, updateTop, 200, updateBottom - updateTop, GxEPD_WHITE);
      int highlightTop = previousTop + (selectedTop - previousTop) / 2;
      int highlightCenter = highlightTop + MENU_HEIGHT / 2;
      display.fillRect(0, highlightTop, 200, MENU_HEIGHT, GxEPD_BLACK);
      drawMenuRow(
          previousIndex,
          highlightCenter >= previousTop && highlightCenter < previousTop + MENU_HEIGHT,
          false
      );
      drawMenuRow(
          selectedIndex,
          highlightCenter >= selectedTop && highlightCenter < selectedTop + MENU_HEIGHT,
          false
      );
      display.displayWindow(0, updateTop, 200, updateBottom - updateTop);
    }
  }

  drawMenuRow(previousIndex, false, true);
  drawMenuRow(selectedIndex, true, true);
  display.setTextWrap(true);

  if (distance <= 1) {
    display.displayWindow(0, updateTop, 200, updateBottom - updateTop);
  } else {
    display.displayWindow(0, previousTop, 200, MENU_HEIGHT);
    display.displayWindow(0, selectedTop, 200, MENU_HEIGHT);
  }

  previousFastMenuIndex = selectedIndex;
  guiState = MAIN_MENU_STATE;
}

void Watchy::showAbout() {
  if (handleAbout()) {
    return;
  }
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 20);

  display.print("CityWeather: ");
  display.println(CITYWEATHER_VERSION);

  display.print("LibVer: ");
  display.println(WATCHY_LIB_VER);

  display.print("Rev: v");
  display.println(getBoardRevision());

  display.print("Batt: ");
  float voltage = getBatteryVoltage();
  display.print(voltage);
  display.println("V");

  #ifndef ARDUINO_ESP32S3_DEV
  display.print("Uptime: ");
  RTC.read(currentTime);
  time_t b = makeTime(bootTime);
  time_t c = makeTime(currentTime);
  int totalSeconds = c-b;
  //int seconds = (totalSeconds % 60);
  int minutes = (totalSeconds % 3600) / 60;
  int hours = (totalSeconds % 86400) / 3600;
  int days = (totalSeconds % (86400 * 30)) / 86400; 
  display.print(days);
  display.print("d");
  display.print(hours);
  display.print("h");
  display.print(minutes);
  display.println("m");  
  #endif
  
  if(WIFI_CONFIGURED){
    display.print("SSID: ");
    display.println(lastSSID);
    display.print("IP: ");
    display.println(IPAddress(lastIPAddress).toString());
  }else{
    display.println("WiFi Not Connected");
  }
  display.display(true); // partial refresh

  guiState = APP_STATE;
}

void Watchy::showBuzz() {
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(70, 80);
  display.println("Buzz!");
  display.display(true); // partial refresh
  vibMotor();
  showMenu(menuIndex, true);
}

void Watchy::vibMotor(uint8_t intervalMs, uint8_t length) {
  pinMode(VIB_MOTOR_PIN, OUTPUT);
  bool motorOn = false;
  for (int i = 0; i < length; i++) {
    motorOn = !motorOn;
    digitalWrite(VIB_MOTOR_PIN, motorOn);
    delay(intervalMs);
  }
}

void Watchy::setTime() {

  guiState = APP_STATE;

  RTC.read(currentTime);

  #ifdef ARDUINO_ESP32S3_DEV
  uint8_t minute = currentTime.Minute;
  uint8_t hour   = currentTime.Hour;
  uint8_t day    = currentTime.Day;
  uint8_t month  = currentTime.Month;
  uint8_t year   = currentTime.Year;  
  #else
  int8_t minute = currentTime.Minute;
  int8_t hour   = currentTime.Hour;
  int8_t day    = currentTime.Day;
  int8_t month  = currentTime.Month;
  int8_t year   = tmYearToY2k(currentTime.Year);
  #endif

  int8_t setIndex = SET_HOUR;

  int8_t blink = 0;

  pinMode(DOWN_BTN_PIN, INPUT);
  pinMode(UP_BTN_PIN, INPUT);
  pinMode(MENU_BTN_PIN, INPUT);
  pinMode(BACK_BTN_PIN, INPUT);

  display.setFullWindow();

  while (1) {

    if (digitalRead(MENU_BTN_PIN) == ACTIVE_LOW) {
      if (screenshotRequested()) {
        continue;
      }
      setIndex++;
      if (setIndex > SET_DAY) {
        break;
      }
    }
    if (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {
      if (setIndex != SET_HOUR) {
        setIndex--;
      }
    }

    blink = 1 - blink;

    if (digitalRead(DOWN_BTN_PIN) == ACTIVE_LOW) {
      blink = 1;
      switch (setIndex) {
      case SET_HOUR:
        hour == 23 ? (hour = 0) : hour++;
        break;
      case SET_MINUTE:
        minute == 59 ? (minute = 0) : minute++;
        break;
      case SET_YEAR:
        year == 99 ? (year = 0) : year++;
        break;
      case SET_MONTH:
        month == 12 ? (month = 1) : month++;
        break;
      case SET_DAY:
        day == 31 ? (day = 1) : day++;
        break;
      default:
        break;
      }
    }

    if (digitalRead(UP_BTN_PIN) == ACTIVE_LOW) {
      blink = 1;
      switch (setIndex) {
      case SET_HOUR:
        hour == 0 ? (hour = 23) : hour--;
        break;
      case SET_MINUTE:
        minute == 0 ? (minute = 59) : minute--;
        break;
      case SET_YEAR:
        year == 0 ? (year = 99) : year--;
        break;
      case SET_MONTH:
        month == 1 ? (month = 12) : month--;
        break;
      case SET_DAY:
        day == 1 ? (day = 31) : day--;
        break;
      default:
        break;
      }
    }

    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&DSEG7_Classic_Bold_53);

    display.setCursor(5, 80);
    if (setIndex == SET_HOUR) { // blink hour digits
      display.setTextColor(blink ? GxEPD_BLACK : GxEPD_WHITE);
    }
    if (hour < 10) {
      display.print("0");
    }
    display.print(hour);

    display.setTextColor(GxEPD_BLACK);
    display.print(":");

    display.setCursor(108, 80);
    if (setIndex == SET_MINUTE) { // blink minute digits
      display.setTextColor(blink ? GxEPD_BLACK : GxEPD_WHITE);
    }
    if (minute < 10) {
      display.print("0");
    }
    display.print(minute);

    display.setTextColor(GxEPD_BLACK);

    display.setFont(&FreeMonoBold9pt7b);
    display.setCursor(45, 150);
    if (setIndex == SET_YEAR) { // blink minute digits
      display.setTextColor(blink ? GxEPD_BLACK : GxEPD_WHITE);
    }
    display.print(2000 + year);

    display.setTextColor(GxEPD_BLACK);
    display.print("/");

    if (setIndex == SET_MONTH) { // blink minute digits
      display.setTextColor(blink ? GxEPD_BLACK : GxEPD_WHITE);
    }
    if (month < 10) {
      display.print("0");
    }
    display.print(month);

    display.setTextColor(GxEPD_BLACK);
    display.print("/");

    if (setIndex == SET_DAY) { // blink minute digits
      display.setTextColor(blink ? GxEPD_BLACK : GxEPD_WHITE);
    }
    if (day < 10) {
      display.print("0");
    }
    display.print(day);
    display.display(true); // partial refresh
  }

  tmElements_t tm;
  tm.Month  = month;
  tm.Day    = day;
  #ifdef ARDUINO_ESP32S3_DEV
  tm.Year   = year;
  #else
  tm.Year   = y2kYearToTm(year);
  #endif
  tm.Hour   = hour;
  tm.Minute = minute;
  tm.Second = 0;

  RTC.set(tm);

  showMenu(menuIndex, true);
}

void Watchy::showAccelerometer() {
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);

  Accel acc;

  long previousMillis = 0;
  long interval       = 200;

  guiState = APP_STATE;

  pinMode(BACK_BTN_PIN, INPUT);
  pinMode(MENU_BTN_PIN, INPUT);

  while (1) {

    if (digitalRead(MENU_BTN_PIN) == ACTIVE_LOW && screenshotRequested()) {
      continue;
    }

    unsigned long currentMillis = millis();

    if (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {
      break;
    }

    if (currentMillis - previousMillis > interval) {
      previousMillis = currentMillis;
      // Get acceleration data
      bool res          = sensor.getAccel(acc);
      uint8_t direction = sensor.getDirection();
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(0, 30);
      if (res == false) {
        display.println("getAccel FAIL");
      } else {
        display.print("  X:");
        display.println(acc.x);
        display.print("  Y:");
        display.println(acc.y);
        display.print("  Z:");
        display.println(acc.z);

        display.setCursor(30, 130);
        switch (direction) {
        case DIRECTION_DISP_DOWN:
          display.println("FACE DOWN");
          break;
        case DIRECTION_DISP_UP:
          display.println("FACE UP");
          break;
        case DIRECTION_BOTTOM_EDGE:
          display.println("BOTTOM EDGE");
          break;
        case DIRECTION_TOP_EDGE:
          display.println("TOP EDGE");
          break;
        case DIRECTION_RIGHT_EDGE:
          display.println("RIGHT EDGE");
          break;
        case DIRECTION_LEFT_EDGE:
          display.println("LEFT EDGE");
          break;
        default:
          display.println("ERROR!!!");
          break;
        }
      }
      display.display(true); // partial refresh
    }
  }

  showMenu(menuIndex, true);
}

void Watchy::showWatchFace(bool partialRefresh) {
  display.setFullWindow();
  // At this point it is sure we are going to update
  display.epd2.asyncPowerOn();
  drawWatchFace();
  display.display(partialRefresh); // partial refresh
  guiState = WATCHFACE_STATE;
}

void Watchy::drawWatchFace() {
  display.setFont(&DSEG7_Classic_Bold_53);
  display.setCursor(5, 53 + 60);
  if (currentTime.Hour < 10) {
    display.print("0");
  }
  display.print(currentTime.Hour);
  display.print(":");
  if (currentTime.Minute < 10) {
    display.print("0");
  }
  display.println(currentTime.Minute);
}

weatherData Watchy::getWeatherData() {
  return _getWeatherData(settings.cityID, settings.lat, settings.lon,
    settings.weatherUnit, settings.weatherLang, settings.weatherURL,
    settings.weatherAPIKey, settings.weatherUpdateInterval);
}

weatherData Watchy::_getWeatherData(String cityID, String lat, String lon, String units, String lang,
                                   String url, String apiKey,
                                   uint8_t updateInterval) {
  currentWeather.isMetric = units == String("metric");
  if (weatherIntervalCounter < 0) { //-1 on first run, set to updateInterval
    weatherIntervalCounter = updateInterval;
  }
  if (weatherIntervalCounter >=
      updateInterval) { // only update if WEATHER_UPDATE_INTERVAL has elapsed
                        // i.e. 30 minutes
    if (connectWiFi()) {
      HTTPClient http; // Use Weather API for live data if WiFi is connected
      http.setConnectTimeout(3000); // 3 second max timeout
      String weatherQueryURL = url;
      if(cityID != ""){
        weatherQueryURL.replace("{cityID}", cityID);
      }else{
        weatherQueryURL.replace("{lat}", lat);
        weatherQueryURL.replace("{lon}", lon);
      }
      weatherQueryURL.replace("{units}", units);
      weatherQueryURL.replace("{lang}", lang);
      weatherQueryURL.replace("{apiKey}", apiKey);
      http.begin(weatherQueryURL.c_str());
      int httpResponseCode = http.GET();
      if (httpResponseCode == 200) {
        String payload             = http.getString();
        JSONVar responseObject     = JSON.parse(payload);
        currentWeather.temperature = int(responseObject["main"]["temp"]);
        currentWeather.weatherConditionCode =
            int(responseObject["weather"][0]["id"]);
        currentWeather.weatherDescription =
		        JSONVar::stringify(responseObject["weather"][0]["main"]);
	      currentWeather.external = true;
		        breakTime((time_t)(int)responseObject["sys"]["sunrise"], currentWeather.sunrise);
		        breakTime((time_t)(int)responseObject["sys"]["sunset"], currentWeather.sunset);
        // sync NTP during weather API call and use timezone of lat & lon
        gmtOffset = int(responseObject["timezone"]);
        syncNTP(gmtOffset);
      } else {
        // http error
      }
      http.end();
      // turn off radios
      WiFi.mode(WIFI_OFF);
      btStop();
    } else { // No WiFi, use internal temperature sensor
      uint8_t temperature = sensor.readTemperature(); // celsius
      if (!currentWeather.isMetric) {
        temperature = temperature * 9. / 5. + 32.; // fahrenheit
      }
      currentWeather.temperature          = temperature;
      currentWeather.weatherConditionCode = 800;
      currentWeather.external             = false;
    }
    weatherIntervalCounter = 0;
  } else {
    weatherIntervalCounter++;
  }
  return currentWeather;
}

float Watchy::getBatteryVoltage() {
  #ifdef ARDUINO_ESP32S3_DEV
    return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f * ADC_VOLTAGE_DIVIDER;
  #else
  if (RTC.rtcType == DS3231) {
    return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f *
           2.0f; // Battery voltage goes through a 1/2 divider.
  } else {
    return analogReadMilliVolts(BATT_ADC_PIN) / 1000.0f * 2.0f;
  }
  #endif
}

uint8_t Watchy::getBoardRevision() {
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);
  if(chip_info.model == CHIP_ESP32){ //Revision 1.0 - 2.0
    Wire.beginTransmission(0x68); //v1.0 has DS3231
    if (Wire.endTransmission() == 0){
      return 10;
    }
    delay(1);
    Wire.beginTransmission(0x51); //v1.5 and v2.0 have PCF8563
    if (Wire.endTransmission() == 0){
        pinMode(35, INPUT);
        if(digitalRead(35) == 0){
          return 20; //in rev 2.0, pin 35 is BTN 3 and has a pulldown
        }else{
          return 15; //in rev 1.5, pin 35 is the battery ADC
        }
    }
  }
  if(chip_info.model == CHIP_ESP32S3){ //Revision 3.0
    return 30;
  }
  return -1;
}

uint16_t Watchy::_readRegister(uint8_t address, uint8_t reg, uint8_t *data,
                               uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom((uint8_t)address, (uint8_t)len);
  uint8_t i = 0;
  while (Wire.available()) {
    data[i++] = Wire.read();
  }
  return 0;
}

uint16_t Watchy::_writeRegister(uint8_t address, uint8_t reg, uint8_t *data,
                                uint16_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(data, len);
  return (0 != Wire.endTransmission());
}

void Watchy::_bmaConfig() {

  if (sensor.begin(_readRegister, _writeRegister, delay) == false) {
    // fail to init BMA
    return;
  }

  // Accel parameter structure
  Acfg cfg;
  /*!
      Output data rate in Hz, Optional parameters:
          - BMA4_OUTPUT_DATA_RATE_0_78HZ
          - BMA4_OUTPUT_DATA_RATE_1_56HZ
          - BMA4_OUTPUT_DATA_RATE_3_12HZ
          - BMA4_OUTPUT_DATA_RATE_6_25HZ
          - BMA4_OUTPUT_DATA_RATE_12_5HZ
          - BMA4_OUTPUT_DATA_RATE_25HZ
          - BMA4_OUTPUT_DATA_RATE_50HZ
          - BMA4_OUTPUT_DATA_RATE_100HZ
          - BMA4_OUTPUT_DATA_RATE_200HZ
          - BMA4_OUTPUT_DATA_RATE_400HZ
          - BMA4_OUTPUT_DATA_RATE_800HZ
          - BMA4_OUTPUT_DATA_RATE_1600HZ
  */
  cfg.odr = BMA4_OUTPUT_DATA_RATE_100HZ;
  /*!
      G-range, Optional parameters:
          - BMA4_ACCEL_RANGE_2G
          - BMA4_ACCEL_RANGE_4G
          - BMA4_ACCEL_RANGE_8G
          - BMA4_ACCEL_RANGE_16G
  */
  cfg.range = BMA4_ACCEL_RANGE_2G;
  /*!
      Bandwidth parameter, determines filter configuration, Optional parameters:
          - BMA4_ACCEL_OSR4_AVG1
          - BMA4_ACCEL_OSR2_AVG2
          - BMA4_ACCEL_NORMAL_AVG4
          - BMA4_ACCEL_CIC_AVG8
          - BMA4_ACCEL_RES_AVG16
          - BMA4_ACCEL_RES_AVG32
          - BMA4_ACCEL_RES_AVG64
          - BMA4_ACCEL_RES_AVG128
  */
  cfg.bandwidth = BMA4_ACCEL_NORMAL_AVG4;

  /*! Filter performance mode , Optional parameters:
      - BMA4_CIC_AVG_MODE
      - BMA4_CONTINUOUS_MODE
  */
  cfg.perf_mode = BMA4_CONTINUOUS_MODE;

  // Configure the BMA423 accelerometer
  sensor.setAccelConfig(cfg);

  // Enable BMA423 accelerometer
  // Warning : Need to use feature, you must first enable the accelerometer
  // Warning : Need to use feature, you must first enable the accelerometer
  sensor.enableAccel();

  struct bma4_int_pin_config config;
  config.edge_ctrl = BMA4_LEVEL_TRIGGER;
  config.lvl       = BMA4_ACTIVE_HIGH;
  config.od        = BMA4_PUSH_PULL;
  config.output_en = BMA4_OUTPUT_ENABLE;
  config.input_en  = BMA4_INPUT_DISABLE;
  // The correct trigger interrupt needs to be configured as needed
  sensor.setINTPinConfig(config, BMA4_INTR1_MAP);

  struct bma423_axes_remap remap_data;
  remap_data.x_axis      = 1;
  remap_data.x_axis_sign = 0xFF;
  remap_data.y_axis      = 0;
  remap_data.y_axis_sign = 0xFF;
  remap_data.z_axis      = 2;
  remap_data.z_axis_sign = 0xFF;
  // Need to raise the wrist function, need to set the correct axis
  sensor.setRemapAxes(&remap_data);

  // Enable BMA423 isStepCounter feature
  sensor.enableFeature(BMA423_STEP_CNTR, true);
  // Enable BMA423 isTilt feature
  sensor.enableFeature(BMA423_TILT, true);
  // Enable BMA423 isDoubleClick feature
  sensor.enableFeature(BMA423_WAKEUP, true);

  // Reset steps
  sensor.resetStepCounter();

  // Turn on feature interrupt
  sensor.enableStepCountInterrupt();
  sensor.enableTiltInterrupt();
  // It corresponds to isDoubleClick interrupt
  sensor.enableWakeupInterrupt();
}

void Watchy::setupWifi() {
  display.epd2.setBusyCallback(0); // temporarily disable lightsleep on busy
  WiFiManager wifiManager;
  wifiManager.setConfigPortalBlocking(false);
  wifiManager.setConfigPortalTimeout(WIFI_AP_TIMEOUT);
  wifiManager.setSaveConnectTimeout(20);
  wifiManager.setAPCallback(_configModeCallback);
  pinMode(BACK_BTN_PIN, INPUT);
  pinMode(MENU_BTN_PIN, INPUT);
  display.setFullWindow();
  display.fillRect(0, 24, 200, 176, GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 54);
  display.println("Starting WiFi");
  display.println("setup...");
  display.println("Back: cancel");
  display.displayWindow(0, 24, 200, 176);

  bool connected = false;
  bool canceled = false;
  unsigned long startedAt = millis();
  wifiManager.startConfigPortal(WIFI_AP_SSID);
  while (!connected && !canceled) {
    if (digitalRead(MENU_BTN_PIN) == ACTIVE_LOW && screenshotRequested()) {
      continue;
    }
    if (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {
      canceled = true;
      wifiManager.stopConfigPortal();
      while (digitalRead(BACK_BTN_PIN) == ACTIVE_LOW) {
        delay(10);
      }
      break;
    }
    connected = wifiManager.process();
    if (!connected && WIFI_AP_TIMEOUT > 0 &&
        millis() - startedAt > (unsigned long)WIFI_AP_TIMEOUT * 1000UL) {
      wifiManager.stopConfigPortal();
      break;
    }
    delay(10);
  }

  if (connected) {
    display.fillRect(0, 24, 200, 176, GxEPD_WHITE);
    display.setCursor(0, 54);
    display.println("Connected to:");
    display.println(WiFi.SSID());
    display.println("Local IP:");
    display.println(WiFi.localIP());
    weatherIntervalCounter = -1; // Reset to force weather to be read again
    lastIPAddress = WiFi.localIP();
    WiFi.SSID().toCharArray(lastSSID, 30);
    onWifiConfigured();
    display.displayWindow(0, 24, 200, 176);
    guiState = APP_STATE;
  } else {
    guiState = MAIN_MENU_STATE;
  }

  // turn off radios
  WiFi.mode(WIFI_OFF);
  btStop();
  // enable lightsleep on busy
  display.epd2.setBusyCallback(WatchyDisplay::busyCallback);
  if (!connected) {
    showMenu(menuIndex, true);
  }
}

void Watchy::_configModeCallback(WiFiManager *myWiFiManager) {
  display.setFullWindow();
  display.fillRect(0, 24, 200, 176, GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 38);
  display.println("Current WiFi");
  String currentSsid = lastSSID[0] != '\0' ? String(lastSSID) : String("-");
  if (currentSsid.length() > 14) {
    currentSsid.remove(14);
  }
  display.print("SSID: ");
  display.println(currentSsid);
  display.print("IP: ");
  if (lastIPAddress != 0) {
    display.println(IPAddress(lastIPAddress).toString());
  } else {
    display.println("-");
  }
  display.println("Connect to");
  display.print("SSID: ");
  display.println(WIFI_AP_SSID);
  display.print("IP: ");
  display.println(WiFi.softAPIP());
  display.println("Back: Cancel");
  display.displayWindow(0, 24, 200, 176);
}

bool Watchy::connectWiFi() {
  if (WL_CONNECT_FAILED ==
      WiFi.begin()) { // WiFi not setup, you can also use hard coded credentials
                      // with WiFi.begin(SSID,PASS);
    WIFI_CONFIGURED = false;
  } else {
    if (WL_CONNECTED ==
        WiFi.waitForConnectResult()) { // attempt to connect for 10s
      lastIPAddress = WiFi.localIP();
      WiFi.SSID().toCharArray(lastSSID, 30);
      WIFI_CONFIGURED = true;
    } else { // connection failed, time out
      WIFI_CONFIGURED = false;
      // turn off radios
      WiFi.mode(WIFI_OFF);
      btStop();
    }
  }
  return WIFI_CONFIGURED;
}
/*
void Watchy::showUpdateFW() {
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Please visit");
  display.println("watchy.sqfmi.com");
  display.println("with a Bluetooth");
  display.println("enabled device");
  display.println(" ");
  display.println("Press menu button");
  display.println("again when ready");
  display.println(" ");
  display.println("Keep USB powered");
  display.display(true); // partial refresh

  guiState = FW_UPDATE_STATE;
}

void Watchy::updateFWBegin() {
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Bluetooth Started");
  display.println(" ");
  display.println("Watchy BLE OTA");
  display.println(" ");
  display.println("Waiting for");
  display.println("connection...");
  display.display(true); // partial refresh

  BLE BT;
  BT.begin("Watchy BLE OTA");
  int prevStatus = -1;
  int currentStatus;

  while (1) {
    currentStatus = BT.updateStatus();
    if (prevStatus != currentStatus || prevStatus == 1) {
      if (currentStatus == 0) {
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("BLE Connected!");
        display.println(" ");
        display.println("Waiting for");
        display.println("upload...");
        display.display(true); // partial refresh
      }
      if (currentStatus == 1) {
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("Downloading");
        display.println("firmware:");
        display.println(" ");
        display.print(BT.howManyBytes());
        display.println(" bytes");
        display.display(true); // partial refresh
      }
      if (currentStatus == 2) {
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("Download");
        display.println("completed!");
        display.println(" ");
        display.println("Rebooting...");
        display.display(true); // partial refresh

        delay(2000);
        esp_restart();
      }
      if (currentStatus == 4) {
        display.setFullWindow();
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold9pt7b);
        display.setTextColor(GxEPD_BLACK);
        display.setCursor(0, 30);
        display.println("BLE Disconnected!");
        display.println(" ");
        display.println("exiting...");
        display.display(true); // partial refresh
        delay(1000);
        break;
      }
      prevStatus = currentStatus;
    }
    delay(100);
  }

  // turn off radios
  WiFi.mode(WIFI_OFF);
  btStop();
  showMenu(menuIndex, true);
}
*/
void Watchy::showSyncNTP() {
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setCursor(0, 30);
  display.println("Syncing NTP... ");
  display.print("GMT offset: ");
  display.println(gmtOffset);
  display.display(true); // partial refresh
  if (connectWiFi()) {
    if (syncNTP()) {
      display.println("NTP Sync Success\n");
      display.println("Current Time Is:");

      RTC.read(currentTime);

      display.print(tmYearToCalendar(currentTime.Year));
      display.print("/");
      display.print(currentTime.Month);
      display.print("/");
      display.print(currentTime.Day);
      display.print(" - ");

      if (currentTime.Hour < 10) {
        display.print("0");
      }
      display.print(currentTime.Hour);
      display.print(":");
      if (currentTime.Minute < 10) {
        display.print("0");
      }
      display.println(currentTime.Minute);
    } else {
      display.println("NTP Sync Failed");
    }
    WiFi.mode(WIFI_OFF);
    btStop();
  } else {
    display.println("WiFi Not Configured");
  }
  display.display(true); // partial refresh
  delay(3000);
  showMenu(menuIndex, true);
}

bool Watchy::syncNTP() { // NTP sync - call after connecting to WiFi and
                         // remember to turn it back off
  return syncNTP(gmtOffset,
                 settings.ntpServer.c_str());
}

bool Watchy::syncNTP(long gmt) {
  return syncNTP(gmt, settings.ntpServer.c_str());
}

bool Watchy::syncNTP(long gmt, String ntpServer) {
   // NTP sync - call after connecting to
   // WiFi and remember to turn it back off
   WiFiUDP ntpUDP;
   NTPClient timeClient(ntpUDP, ntpServer.c_str(), gmt);
   timeClient.begin();
   if (!timeClient.forceUpdate()) {
     return false; // NTP sync failed
   }
   tmElements_t tm;
   breakTime((time_t)timeClient.getEpochTime(), tm);
   RTC.set(tm);
   return true;
 }

void Watchy::onMinuteTick() {
  showWatchFace(true);
}

void Watchy::onAppTick() {
}

bool Watchy::shouldDeepSleep() {
  return true;
}

bool Watchy::screenshotRequested() {
  return false;
}

bool Watchy::notificationsEnabled() {
  return false;
}

void Watchy::onWifiConfigured() {
}

void Watchy::onMenuLoop() {
}

void Watchy::onMenuShown() {
}

bool Watchy::handleAbout() {
  return false;
}

void Watchy::onNotificationsSelected() {
}
