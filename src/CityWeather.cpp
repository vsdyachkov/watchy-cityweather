#include <vector>
#include "Adafruit_GFX_ext.h"
#include "Images.h"
#include "FreeMonoBold7pt7b.h"
#include "OpenSansCondBoldCyrillic9pt.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include <stddef.h>
#include <string.h>
#include <Preferences.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "CityWeather.h"
#include "CityWeatherService.h"
#include "StatusBar.h"

#ifndef CITYWEATHER_CLEAN_VERSION
#define CITYWEATHER_CLEAN_VERSION "unknown"
#endif

#ifndef CITYWEATHER_REPOSITORY
#define CITYWEATHER_REPOSITORY "vsdyachkov/watchy-cityweather"
#endif

extern RTC_DATA_ATTR tmElements_t bootTime;
extern uint32_t lastIPAddress;
extern char lastSSID[30];

namespace
{
bool connectDefaultStoredWiFi(uint32_t timeoutMs);
}

CityWeather::CityWeather(const watchySettings &settings_) : Watchy(settings_), cityWeatherService(*this) {}

bool CityWeather::connectWiFi()
{
  restoreCityWeatherWiFiState();
  const uint32_t timeoutMs = isNotificationsActive() ? 15000 : 10000;

  if (connectCityWeatherStoredWiFi(timeoutMs))
  {
    return true;
  }

  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  delay(100);

  if (connectDefaultStoredWiFi(timeoutMs))
  {
    rememberCityWeatherWiFiState();
    rememberCityWeatherWiFiCredentials();
    return true;
  }

  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  delay(100);

  if (isNotificationsActive())
  {
    return false;
  }

  if (Watchy::connectWiFi())
  {
    rememberCityWeatherWiFiState();
    rememberCityWeatherWiFiCredentials();
    return true;
  }

  return connectCityWeatherStoredWiFi(timeoutMs);
}

bool CityWeather::refreshWeatherAfterWiFiConfigured()
{
  bool updated = cityWeatherService.updateWifiData();
  if (updated)
  {
    Watchy::RTC.read(currentTime);
  }
  return updated;
}

const uint8_t WEATHER_ICON_WIDTH = 25;
const uint8_t WEATHER_ICON_HEIGHT = 25;

namespace
{
constexpr uint8_t BATTERY_HISTORY_CAPACITY = 168;
constexpr uint8_t BATTERY_GRAPH_HOURS = 24;
constexpr uint32_t BATTERY_SAMPLE_INTERVAL_SECONDS = 30UL * SECS_PER_MIN;
constexpr uint8_t BATTERY_GRAPH_SAMPLES =
    (BATTERY_GRAPH_HOURS * SECS_PER_HOUR) / BATTERY_SAMPLE_INTERVAL_SECONDS;
constexpr uint8_t BATTERY_LEGACY_HOURLY_TO_SAMPLE_MULTIPLIER =
    SECS_PER_HOUR / BATTERY_SAMPLE_INTERVAL_SECONDS;
constexpr int16_t ABOUT_BATTERY_GRAPH_X = 0;
constexpr int16_t ABOUT_BATTERY_GRAPH_Y = 82;
constexpr int16_t ABOUT_BATTERY_GRAPH_W = 200;
constexpr int16_t ABOUT_BATTERY_GRAPH_H = 110;
constexpr uint32_t INVALID_BATTERY_SAMPLE_HOUR = 0xFFFFFFFF;
constexpr uint32_t BATTERY_HISTORY_STORAGE_MAGIC = 0x43574248;
constexpr uint16_t BATTERY_HISTORY_STORAGE_VERSION_HOURLY = 1;
constexpr uint16_t BATTERY_HISTORY_STORAGE_VERSION = 2;
constexpr const char *BATTERY_HISTORY_STORAGE_NAMESPACE = "cw-battery";
constexpr const char *BATTERY_HISTORY_STORAGE_KEY = "history";
constexpr const char *BATTERY_HISTORY_STORAGE_BACKUP_KEY = "history-bak";

struct BatteryHistorySnapshot
{
  uint32_t magic;
  uint16_t version;
  uint8_t capacity;
  uint8_t count;
  uint8_t start;
  uint8_t reserved[3];
  uint32_t lastSampleHour;
  uint32_t hours[BATTERY_HISTORY_CAPACITY];
  uint8_t percents[BATTERY_HISTORY_CAPACITY];
  uint32_t checksum;
};

RTC_DATA_ATTR uint32_t batteryHistoryHours[BATTERY_HISTORY_CAPACITY] = {};
RTC_DATA_ATTR uint8_t batteryHistoryPercents[BATTERY_HISTORY_CAPACITY] = {};
RTC_DATA_ATTR uint8_t batteryHistoryCount = 0;
RTC_DATA_ATTR uint8_t batteryHistoryStart = 0;
RTC_DATA_ATTR uint32_t lastBatterySampleHour = INVALID_BATTERY_SAMPLE_HOUR;
RTC_DATA_ATTR uint32_t batteryHistoryLoadedMagic = 0;

uint8_t batteryHistoryIndex(uint8_t logicalIndex)
{
  return (batteryHistoryStart + logicalIndex) % BATTERY_HISTORY_CAPACITY;
}

void appendBatteryHistorySample(uint32_t hour, uint8_t percent);

bool timeElementsLookValid(const tmElements_t &time)
{
  uint16_t fullYear = time.Year + 1970;
  return fullYear >= 2020 &&
      fullYear <= 2099 &&
      time.Month >= 1 &&
      time.Month <= 12 &&
      time.Day >= 1 &&
      time.Day <= 31 &&
      time.Hour <= 23 &&
      time.Minute <= 59 &&
      time.Second <= 59;
}

uint32_t aboutUptimeSeconds(tmElements_t current, tmElements_t boot)
{
  constexpr uint32_t MAX_REASONABLE_UPTIME_SECONDS = 366UL * SECS_PER_DAY;
  if (timeElementsLookValid(current) && timeElementsLookValid(boot))
  {
    time_t currentTimestamp = makeTime(current);
    time_t bootTimestamp = makeTime(boot);
    if (currentTimestamp >= bootTimestamp)
    {
      uint32_t elapsedSeconds = static_cast<uint32_t>(currentTimestamp - bootTimestamp);
      if (elapsedSeconds <= MAX_REASONABLE_UPTIME_SECONDS)
      {
        return elapsedSeconds;
      }
    }
  }

  return static_cast<uint32_t>(millis() / 1000UL);
}

void resetBatteryHistory()
{
  batteryHistoryCount = 0;
  batteryHistoryStart = 0;
  lastBatterySampleHour = INVALID_BATTERY_SAMPLE_HOUR;
}

uint32_t checksumBytes(const uint8_t *bytes, size_t length)
{
  uint32_t checksum = 2166136261UL;
  for (size_t i = 0; i < length; i++)
  {
    checksum ^= bytes[i];
    checksum *= 16777619UL;
  }
  return checksum;
}

uint32_t batteryHistorySnapshotChecksum(const BatteryHistorySnapshot &snapshot)
{
  return checksumBytes(
      reinterpret_cast<const uint8_t *>(&snapshot),
      offsetof(BatteryHistorySnapshot, checksum)
  );
}

bool batteryHistoryInMemoryLooksValid()
{
  if (batteryHistoryCount > BATTERY_HISTORY_CAPACITY)
  {
    return false;
  }
  if (batteryHistoryStart >= BATTERY_HISTORY_CAPACITY)
  {
    return false;
  }
  if (batteryHistoryCount == 0)
  {
    return lastBatterySampleHour == INVALID_BATTERY_SAMPLE_HOUR;
  }

  uint32_t previousHour = 0;
  for (uint8_t logicalIndex = 0; logicalIndex < batteryHistoryCount; logicalIndex++)
  {
    uint8_t sampleIndex = batteryHistoryIndex(logicalIndex);
    uint32_t hour = batteryHistoryHours[sampleIndex];
    if (hour == 0 || hour == INVALID_BATTERY_SAMPLE_HOUR)
    {
      return false;
    }
    if (batteryHistoryPercents[sampleIndex] > 100)
    {
      return false;
    }
    if (logicalIndex > 0 && hour <= previousHour)
    {
      return false;
    }
    previousHour = hour;
  }

  return lastBatterySampleHour == previousHour;
}

bool restoreBatteryHistorySnapshot(const BatteryHistorySnapshot &snapshot)
{
  if (
      snapshot.magic != BATTERY_HISTORY_STORAGE_MAGIC ||
      (
          snapshot.version != BATTERY_HISTORY_STORAGE_VERSION &&
          snapshot.version != BATTERY_HISTORY_STORAGE_VERSION_HOURLY
      ) ||
      snapshot.capacity != BATTERY_HISTORY_CAPACITY ||
      snapshot.count > BATTERY_HISTORY_CAPACITY ||
      snapshot.start >= BATTERY_HISTORY_CAPACITY ||
      snapshot.checksum != batteryHistorySnapshotChecksum(snapshot)
  )
  {
    return false;
  }

  resetBatteryHistory();
  uint32_t previousHour = 0;
  for (uint8_t logicalIndex = 0; logicalIndex < snapshot.count; logicalIndex++)
  {
    uint8_t sampleIndex = (snapshot.start + logicalIndex) % BATTERY_HISTORY_CAPACITY;
    uint32_t hour = snapshot.hours[sampleIndex];
    uint8_t percent = snapshot.percents[sampleIndex];
    if (snapshot.version == BATTERY_HISTORY_STORAGE_VERSION_HOURLY)
    {
      hour *= BATTERY_LEGACY_HOURLY_TO_SAMPLE_MULTIPLIER;
    }
    if (hour == 0 || hour == INVALID_BATTERY_SAMPLE_HOUR || percent > 100)
    {
      continue;
    }
    if (batteryHistoryCount > 0 && hour < previousHour)
    {
      continue;
    }
    if (batteryHistoryCount > 0 && hour == previousHour)
    {
      uint8_t lastIndex = batteryHistoryIndex(batteryHistoryCount - 1);
      batteryHistoryPercents[lastIndex] = percent;
      continue;
    }

    appendBatteryHistorySample(hour, percent);
    previousHour = hour;
  }

  if (batteryHistoryCount == 0)
  {
    return snapshot.count == 0;
  }
  return batteryHistoryInMemoryLooksValid();
}

void saveBatteryHistoryToStorage()
{
  BatteryHistorySnapshot snapshot = {};
  snapshot.magic = BATTERY_HISTORY_STORAGE_MAGIC;
  snapshot.version = BATTERY_HISTORY_STORAGE_VERSION;
  snapshot.capacity = BATTERY_HISTORY_CAPACITY;
  snapshot.count = batteryHistoryCount;
  snapshot.start = batteryHistoryStart;
  snapshot.lastSampleHour = lastBatterySampleHour;
  memcpy(snapshot.hours, batteryHistoryHours, sizeof(batteryHistoryHours));
  memcpy(snapshot.percents, batteryHistoryPercents, sizeof(batteryHistoryPercents));
  snapshot.checksum = batteryHistorySnapshotChecksum(snapshot);

  Preferences preferences;
  if (!preferences.begin(BATTERY_HISTORY_STORAGE_NAMESPACE, false))
  {
    return;
  }
  preferences.putBytes(BATTERY_HISTORY_STORAGE_KEY, &snapshot, sizeof(snapshot));
  preferences.putBytes(BATTERY_HISTORY_STORAGE_BACKUP_KEY, &snapshot, sizeof(snapshot));
  preferences.end();
}

bool loadBatteryHistoryFromStorageKey(Preferences &preferences, const char *key)
{
  size_t storedLength = preferences.getBytesLength(key);
  if (storedLength != sizeof(BatteryHistorySnapshot))
  {
    return false;
  }

  BatteryHistorySnapshot snapshot = {};
  size_t readLength = preferences.getBytes(key, &snapshot, sizeof(snapshot));
  if (readLength != sizeof(snapshot))
  {
    return false;
  }
  return restoreBatteryHistorySnapshot(snapshot);
}

bool loadBatteryHistoryFromStorage()
{
  Preferences preferences;
  if (!preferences.begin(BATTERY_HISTORY_STORAGE_NAMESPACE, true))
  {
    return false;
  }

  bool loaded =
      loadBatteryHistoryFromStorageKey(preferences, BATTERY_HISTORY_STORAGE_KEY) ||
      loadBatteryHistoryFromStorageKey(preferences, BATTERY_HISTORY_STORAGE_BACKUP_KEY);
  preferences.end();
  if (loaded)
  {
    saveBatteryHistoryToStorage();
  }
  return loaded;
}

void ensureBatteryHistoryLoaded()
{
  if (
      batteryHistoryLoadedMagic == BATTERY_HISTORY_STORAGE_MAGIC &&
      batteryHistoryInMemoryLooksValid()
  )
  {
    return;
  }

  if (loadBatteryHistoryFromStorage())
  {
    batteryHistoryLoadedMagic = BATTERY_HISTORY_STORAGE_MAGIC;
    return;
  }

  if (batteryHistoryInMemoryLooksValid() && batteryHistoryCount > 0)
  {
    batteryHistoryLoadedMagic = BATTERY_HISTORY_STORAGE_MAGIC;
    saveBatteryHistoryToStorage();
    return;
  }

  resetBatteryHistory();
  batteryHistoryLoadedMagic = BATTERY_HISTORY_STORAGE_MAGIC;
}

void appendBatteryHistorySample(uint32_t hour, uint8_t percent)
{
  if (batteryHistoryCount > 0)
  {
    uint8_t lastIndex = batteryHistoryIndex(batteryHistoryCount - 1);
    if (batteryHistoryHours[lastIndex] == hour)
    {
      batteryHistoryPercents[lastIndex] = percent;
      lastBatterySampleHour = hour;
      return;
    }
  }

  uint8_t insertIndex;
  if (batteryHistoryCount < BATTERY_HISTORY_CAPACITY)
  {
    insertIndex = batteryHistoryIndex(batteryHistoryCount);
    batteryHistoryCount++;
  }
  else
  {
    insertIndex = batteryHistoryStart;
    batteryHistoryStart = (batteryHistoryStart + 1) % BATTERY_HISTORY_CAPACITY;
  }

  batteryHistoryHours[insertIndex] = hour;
  batteryHistoryPercents[insertIndex] = percent;
  lastBatterySampleHour = hour;
}

void printFitLine(Adafruit_GFX &display, String text, int16_t x, int16_t y, int16_t maxWidth)
{
  int16_t x1;
  int16_t y1;
  uint16_t width;
  uint16_t height;
  while (text.length() > 0)
  {
    display.getTextBounds(text, x, y, &x1, &y1, &width, &height);
    if (x1 + width <= x + maxWidth)
    {
      break;
    }
    text.remove(text.length() - 1);
  }
  display.setCursor(x, y);
  display.print(text);
}

bool versionParts(const String &version, int parts[4])
{
  String normalized = version;
  normalized.trim();
  if (normalized.length() == 0)
  {
    return false;
  }
  if (normalized[0] == 'v' || normalized[0] == 'V')
  {
    normalized.remove(0, 1);
  }
  if (normalized.length() == 0 || !isDigit(normalized[0]))
  {
    return false;
  }

  for (uint8_t i = 0; i < 4; i++)
  {
    parts[i] = 0;
  }

  uint8_t partIndex = 0;
  bool hasPart = false;
  uint16_t index = 0;
  while (index < normalized.length() && partIndex < 4)
  {
    while (index < normalized.length() && !isDigit(normalized[index]))
    {
      index++;
    }
    if (index >= normalized.length())
    {
      break;
    }

    long value = 0;
    while (index < normalized.length() && isDigit(normalized[index]))
    {
      value = value * 10 + (normalized[index] - '0');
      index++;
    }
    parts[partIndex] = static_cast<int>(value);
    hasPart = true;
    partIndex++;
  }

  return hasPart;
}

String shortVersionLabel(const String &version)
{
  int parts[4];
  if (!versionParts(version, parts))
  {
    String fallback = version;
    fallback.trim();
    return fallback;
  }

  return String(parts[0]) + "." + String(parts[1]);
}

int compareVersionStrings(const String &currentVersion, const String &latestVersion, bool *comparable)
{
  int currentParts[4];
  int latestParts[4];
  if (!versionParts(currentVersion, currentParts) || !versionParts(latestVersion, latestParts))
  {
    *comparable = false;
    return 0;
  }

  *comparable = true;
  for (uint8_t i = 0; i < 4; i++)
  {
    if (currentParts[i] < latestParts[i])
    {
      return -1;
    }
    if (currentParts[i] > latestParts[i])
    {
      return 1;
    }
  }
  return 0;
}

bool connectDefaultStoredWiFi(uint32_t timeoutMs)
{
  WiFi.mode(WIFI_STA);
  WiFi.persistent(true);
  if (WiFi.begin() == WL_CONNECT_FAILED)
  {
    restoreCityWeatherWiFiState();
    return false;
  }

  if (WiFi.waitForConnectResult(timeoutMs) != WL_CONNECTED)
  {
    restoreCityWeatherWiFiState();
    return false;
  }

  lastIPAddress = WiFi.localIP();
  WiFi.SSID().toCharArray(lastSSID, 30);
  WIFI_CONFIGURED = true;
  return true;
}

bool githubGetText(const String &url, String *payload)
{
  WiFiClientSecure client;
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(5000);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  http.useHTTP10(true);
  if (!http.begin(client, url))
  {
    return false;
  }

  http.addHeader("Accept", "application/vnd.github+json");
  http.addHeader("User-Agent", "Watchy-CityWeather");
  http.addHeader("Connection", "close");
  int httpCode = http.GET();
  bool success = false;
  if (httpCode == 200)
  {
    *payload = http.getString();
    success = payload->length() > 0;
  }
  http.end();
  return success;
}

bool extractJsonStringProperty(const String &payload, const char *propertyName, String *value)
{
  String propertyKey = "\"" + String(propertyName) + "\"";
  int keyIndex = payload.indexOf(propertyKey);
  if (keyIndex < 0)
  {
    return false;
  }

  int colonIndex = payload.indexOf(':', keyIndex + propertyKey.length());
  if (colonIndex < 0)
  {
    return false;
  }

  int quoteIndex = payload.indexOf('"', colonIndex + 1);
  if (quoteIndex < 0)
  {
    return false;
  }

  value->remove(0);
  bool escaped = false;
  for (int i = quoteIndex + 1; i < payload.length(); i++)
  {
    char character = payload[i];
    if (escaped)
    {
      *value += character;
      escaped = false;
      continue;
    }
    if (character == '\\')
    {
      escaped = true;
      continue;
    }
    if (character == '"')
    {
      value->trim();
      return value->length() > 0;
    }
    *value += character;
  }

  return false;
}

bool githubLatestTag(String *tag)
{
  String payload;
  String tagsUrl = "https://api.github.com/repos/" + String(CITYWEATHER_REPOSITORY) + "/tags?per_page=1";
  if (githubGetText(tagsUrl, &payload) && extractJsonStringProperty(payload, "name", tag))
  {
    tag->trim();
    return tag->length() > 0;
  }

  payload = "";
  String releaseUrl =
      "https://api.github.com/repos/" + String(CITYWEATHER_REPOSITORY) + "/releases/latest";
  if (githubGetText(releaseUrl, &payload) && extractJsonStringProperty(payload, "tag_name", tag))
  {
    tag->trim();
    return tag->length() > 0;
  }

  return false;
}
}

void CityWeather::drawTime()
{
  String timeStr =
      (currentTime.Hour < 10 ? "0" : "") + String(currentTime.Hour) + ":" +
      (currentTime.Minute < 10 ? "0" : "") + String(currentTime.Minute);

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(-1, 17);
  display.print(timeStr);
}

void CityWeather::drawStatusBar()
{
  recordBatteryHistory();
  drawCityWeatherStatusBar(*this, isNotificationsActive());
}

void CityWeather::recordBatteryHistory()
{
  ensureBatteryHistoryLoaded();
  Watchy::RTC.read(currentTime);
  time_t currentTimestamp = makeTime(currentTime);
  if (currentTimestamp <= 0)
  {
    return;
  }

  uint32_t currentSample =
      static_cast<uint32_t>(currentTimestamp / BATTERY_SAMPLE_INTERVAL_SECONDS);
  uint8_t batteryPercent = statusBarBatteryPercentFromVoltage(getBatteryVoltage());
  if (
      lastBatterySampleHour != INVALID_BATTERY_SAMPLE_HOUR &&
      currentSample <= lastBatterySampleHour &&
      batteryHistoryCount > 0
  )
  {
    uint8_t lastIndex = batteryHistoryIndex(batteryHistoryCount - 1);
    if (batteryHistoryPercents[lastIndex] != batteryPercent)
    {
      batteryHistoryPercents[lastIndex] = batteryPercent;
      saveBatteryHistoryToStorage();
    }
    return;
  }

  appendBatteryHistorySample(currentSample, batteryPercent);
  saveBatteryHistoryToStorage();
}

void CityWeather::updateMenuStatusBar(bool force, bool refresh)
{
  Watchy::RTC.read(currentTime);

  const uint32_t now = millis();
  if (
      !force &&
      menuStatusBarDrawn &&
      lastMenuStatusMinute == currentTime.Minute &&
      now - lastMenuStatusBarRefreshAtMs < 30000
  )
  {
    return;
  }

  menuStatusBarDrawn = true;
  lastMenuStatusMinute = currentTime.Minute;
  lastMenuStatusBarRefreshAtMs = now;

  display.setFullWindow();
  display.epd2.asyncPowerOn();
  display.fillRect(0, 0, 200, 24, GxEPD_WHITE);
  drawStatusBar();
  if (refresh)
  {
    display.displayWindow(0, 0, 200, 24);
  }
}

void CityWeather::resetMenuStatusBarRefresh()
{
  menuStatusBarDrawn = false;
  lastMenuStatusMinute = 255;
  lastMenuStatusBarRefreshAtMs = 0;
}

void CityWeather::showAboutScreen()
{
  recordBatteryHistory();
  Watchy::RTC.read(currentTime);

  aboutScreenVisible = true;
  lastAboutButtonActionAtMs = 0;
  drawAboutScreenContent("Check updates...");
  display.display(true);

  display.setTextWrap(true);
  guiState = APP_STATE;
  startAboutUpdateCheck();
}

String CityWeather::checkLatestReleaseStatus()
{
  if (!connectWiFi())
  {
    WiFi.mode(WIFI_OFF);
    return "Check failed";
  }

  String latestTag;
  bool hasLatestTag = githubLatestTag(&latestTag);
  WiFi.mode(WIFI_OFF);

  if (!hasLatestTag)
  {
    return "Check failed";
  }

  bool comparable = false;
  int comparison =
      compareVersionStrings(String(CITYWEATHER_CLEAN_VERSION), latestTag, &comparable);
  if (!comparable)
  {
    return "Latest: " + shortVersionLabel(latestTag);
  }
  if (comparison < 0)
  {
    return "New version: " + shortVersionLabel(latestTag);
  }
  return "Up to date";
}

void CityWeather::runAboutUpdateCheckTask(void *parameter)
{
  CityWeather *self = static_cast<CityWeather *>(parameter);
  if (self != nullptr)
  {
    String updateStatus = self->checkLatestReleaseStatus();
    updateStatus.toCharArray(self->aboutUpdateStatusText, sizeof(self->aboutUpdateStatusText));
    self->aboutUpdateStatusText[sizeof(self->aboutUpdateStatusText) - 1] = '\0';
    self->aboutUpdateStatusReady = true;
    self->aboutUpdateCheckRunning = false;
    self->aboutUpdateTaskHandle = nullptr;
  }

  vTaskDelete(nullptr);
}

void CityWeather::startAboutUpdateCheck()
{
  aboutUpdateStatusReady = false;
  aboutUpdateStatusText[0] = '\0';

  if (aboutUpdateCheckRunning)
  {
    return;
  }

  aboutUpdateCheckRunning = true;
  BaseType_t created = xTaskCreatePinnedToCore(
      runAboutUpdateCheckTask,
      "AboutUpdate",
      8192,
      this,
      1,
      &aboutUpdateTaskHandle,
      1
  );
  if (created != pdPASS)
  {
    aboutUpdateTaskHandle = nullptr;
    strncpy(aboutUpdateStatusText, "Check failed", sizeof(aboutUpdateStatusText) - 1);
    aboutUpdateStatusText[sizeof(aboutUpdateStatusText) - 1] = '\0';
    aboutUpdateStatusReady = true;
    aboutUpdateCheckRunning = false;
  }
}

void CityWeather::refreshAboutUpdateCheckIfNeeded()
{
  if (!aboutUpdateStatusReady)
  {
    return;
  }

  char statusText[sizeof(aboutUpdateStatusText)];
  strncpy(statusText, aboutUpdateStatusText, sizeof(statusText) - 1);
  statusText[sizeof(statusText) - 1] = '\0';
  aboutUpdateStatusReady = false;

  if (!isAboutScreenActive())
  {
    return;
  }

  drawAboutUpdateStatus(String(statusText));
  display.displayWindow(0, 20, 200, 22);
  display.setTextWrap(true);
}

void CityWeather::handleAboutScreenLoop()
{
  showAppTick();

#ifdef ARDUINO_ESP32S3_DEV
  const bool backPressed = digitalRead(BACK_BTN_PIN) == LOW;
#else
  const bool backPressed = digitalRead(BACK_BTN_PIN) == HIGH;
#endif
  if (!backPressed || millis() - lastAboutButtonActionAtMs < 250)
  {
    return;
  }

  lastAboutButtonActionAtMs = millis();
  aboutScreenVisible = false;
  WiFi.disconnect(false);
  WiFi.mode(WIFI_OFF);
  Watchy::showMenu(menuIndex, true);

  const uint32_t releaseStartMs = millis();
  while (millis() - releaseStartMs < 700)
  {
#ifdef ARDUINO_ESP32S3_DEV
    if (digitalRead(BACK_BTN_PIN) != LOW)
#else
    if (digitalRead(BACK_BTN_PIN) != HIGH)
#endif
    {
      break;
    }
    delay(20);
  }

  if (!isNotificationsActive())
  {
    deepSleep();
  }
}

void CityWeather::drawAboutScreenContent(const String &updateStatus)
{
  display.setFullWindow();
  display.fillScreen(GxEPD_WHITE);
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextWrap(false);
  display.setTextColor(GxEPD_BLACK);

  constexpr int16_t lineStep = 20;
  constexpr int16_t appLineY = 14;
  constexpr int16_t updateLineY = appLineY + lineStep;
  constexpr int16_t uptimeLineY = updateLineY + lineStep;
  constexpr int16_t graphTitleY = uptimeLineY + lineStep;
  constexpr int16_t graphTitleGap = 8;

  printFitLine(
      display,
      "CityWeather: " + shortVersionLabel(String(CITYWEATHER_CLEAN_VERSION)),
      0,
      appLineY,
      200
  );
  printFitLine(display, updateStatus, 0, updateLineY, 200);

  uint32_t totalSeconds = aboutUptimeSeconds(currentTime, bootTime);
  uint16_t minutes = (totalSeconds % SECS_PER_HOUR) / SECS_PER_MIN;
  uint16_t hours = (totalSeconds % SECS_PER_DAY) / SECS_PER_HOUR;
  uint16_t days = totalSeconds / SECS_PER_DAY;
  printFitLine(
      display,
      "Uptime: " + String(days) + "d" + String(hours) + "h" + String(minutes) + "m",
      0,
      uptimeLineY,
      200
  );

  printFitLine(display, "Battery Usage 24H", 0, graphTitleY, 200);
  drawBatteryHistoryGraph(
      ABOUT_BATTERY_GRAPH_X,
      ABOUT_BATTERY_GRAPH_Y,
      ABOUT_BATTERY_GRAPH_W,
      ABOUT_BATTERY_GRAPH_H
  );
}

void CityWeather::drawAboutUpdateStatus(const String &updateStatus)
{
  display.setFullWindow();
  display.setFont(&FreeMonoBold9pt7b);
  display.setTextColor(GxEPD_BLACK);
  display.setTextWrap(false);
  display.fillRect(0, 20, 200, 22, GxEPD_WHITE);
  printFitLine(display, updateStatus, 0, 34, 200);
}

void CityWeather::refreshAboutBatteryGraphIfNeeded()
{
  ensureBatteryHistoryLoaded();
  Watchy::RTC.read(currentTime);
  time_t currentTimestamp = makeTime(currentTime);
  if (currentTimestamp <= 0)
  {
    return;
  }

  uint32_t currentSample =
      static_cast<uint32_t>(currentTimestamp / BATTERY_SAMPLE_INTERVAL_SECONDS);
  if (lastBatterySampleHour == currentSample)
  {
    return;
  }

  recordBatteryHistory();
  display.setFullWindow();
  display.epd2.asyncPowerOn();
  display.fillRect(
      ABOUT_BATTERY_GRAPH_X,
      ABOUT_BATTERY_GRAPH_Y,
      ABOUT_BATTERY_GRAPH_W,
      200 - ABOUT_BATTERY_GRAPH_Y,
      GxEPD_WHITE
  );
  drawBatteryHistoryGraph(
      ABOUT_BATTERY_GRAPH_X,
      ABOUT_BATTERY_GRAPH_Y,
      ABOUT_BATTERY_GRAPH_W,
      ABOUT_BATTERY_GRAPH_H
  );
  display.displayWindow(
      ABOUT_BATTERY_GRAPH_X,
      ABOUT_BATTERY_GRAPH_Y,
      ABOUT_BATTERY_GRAPH_W,
      200 - ABOUT_BATTERY_GRAPH_Y
  );
}

void CityWeather::drawBatteryHistoryGraph(int16_t x, int16_t y, int16_t w, int16_t h)
{
  ensureBatteryHistoryLoaded();
  Watchy::RTC.read(currentTime);
  time_t currentTimestamp = makeTime(currentTime);
  if (currentTimestamp <= 0)
  {
    return;
  }

  uint32_t currentSample =
      static_cast<uint32_t>(currentTimestamp / BATTERY_SAMPLE_INTERVAL_SECONDS);
  if (
      lastBatterySampleHour != INVALID_BATTERY_SAMPLE_HOUR &&
      lastBatterySampleHour > currentSample
  )
  {
    currentSample = lastBatterySampleHour;
  }
  uint32_t startSample =
      currentSample >= BATTERY_GRAPH_SAMPLES ? currentSample - BATTERY_GRAPH_SAMPLES : 0;
  uint32_t graphSamples = currentSample - startSample;
  if (graphSamples == 0)
  {
    graphSamples = 1;
  }

  bool hasWindowSample = false;
  for (uint8_t logicalIndex = 0; logicalIndex < batteryHistoryCount; logicalIndex++)
  {
    uint8_t sampleIndex = batteryHistoryIndex(logicalIndex);
    uint32_t sampleSlot = batteryHistoryHours[sampleIndex];
    if (sampleSlot >= startSample && sampleSlot <= currentSample)
    {
      hasWindowSample = true;
      break;
    }
  }
  if (
      !hasWindowSample &&
      lastBatterySampleHour != INVALID_BATTERY_SAMPLE_HOUR &&
      batteryHistoryCount > 0
  )
  {
    currentSample = lastBatterySampleHour;
    startSample =
        currentSample >= BATTERY_GRAPH_SAMPLES ? currentSample - BATTERY_GRAPH_SAMPLES : 0;
    graphSamples = currentSample - startSample;
    if (graphSamples == 0)
    {
      graphSamples = 1;
    }
  }

  int16_t plotLeft = x + 34;
  int16_t plotRight = x + w - 3;
  int16_t percentLabelRight = plotLeft - 4;
  int16_t plotTop = y + 4;
  int16_t plotBottom = y + h - 17;
  int16_t plotWidth = plotRight - plotLeft;
  int16_t plotHeight = plotBottom - plotTop;

  display.setFont(&FreeMonoBold7pt7b);
  display.setTextColor(GxEPD_BLACK);

  const uint8_t gridPercents[] = {100, 75, 50, 25};
  for (uint8_t i = 0; i < sizeof(gridPercents) / sizeof(gridPercents[0]); i++)
  {
    int16_t gridY =
        plotBottom - static_cast<int16_t>((gridPercents[i] * plotHeight) / 100);
    drawTextRightAligned(display, percentLabelRight, gridY + 4, String(gridPercents[i]) + "%");
    for (int16_t dashX = plotLeft + 2; dashX <= plotRight; dashX += 8)
    {
      int16_t dashEnd = dashX + 2;
      if (dashEnd > plotRight)
      {
        dashEnd = plotRight;
      }
      display.drawFastHLine(dashX, gridY, dashEnd - dashX + 1, GxEPD_BLACK);
    }
  }
  drawTextRightAligned(display, percentLabelRight, plotBottom + 4, "0%");

  display.drawFastVLine(plotLeft, plotTop, plotHeight + 1, GxEPD_BLACK);
  display.drawFastHLine(plotLeft, plotBottom, plotWidth + 1, GxEPD_BLACK);

  display.setFont(&FreeMonoBold7pt7b);
  for (uint8_t tickSample = 0; tickSample <= graphSamples; tickSample++)
  {
    int16_t tickX =
        plotLeft + static_cast<int16_t>((tickSample * static_cast<uint32_t>(plotWidth)) / graphSamples);
    if (tickSample % 12 == 0 || tickSample == graphSamples)
    {
      if (tickX > plotLeft && tickX < plotRight)
      {
        for (int16_t dashY = plotTop + 3; dashY < plotBottom - 8; dashY += 9)
        {
          display.drawPixel(tickX, dashY, GxEPD_BLACK);
        }
      }
      display.drawFastVLine(tickX, plotBottom - 6, 7, GxEPD_BLACK);
      uint32_t sample = startSample + tickSample;
      tmElements_t tickTime;
      breakTime(static_cast<time_t>(sample * BATTERY_SAMPLE_INTERVAL_SECONDS), tickTime);
      String axisLabel = tickTime.Hour < 10 ? "0" + String(tickTime.Hour) : String(tickTime.Hour);
      int16_t labelX1;
      int16_t labelY1;
      uint16_t labelWidth;
      uint16_t labelHeight;
      display.getTextBounds(axisLabel, 0, 0, &labelX1, &labelY1, &labelWidth, &labelHeight);
      int16_t labelX = tickX - labelX1 - static_cast<int16_t>(labelWidth / 2);
      if (labelX < x)
      {
        labelX = x;
      }
      if (labelX + static_cast<int16_t>(labelWidth) > x + w)
      {
        labelX = x + w - static_cast<int16_t>(labelWidth);
      }
      display.setCursor(labelX, plotBottom + 14);
      display.print(axisLabel);
    }
    else if (tickSample % 6 == 0)
    {
      display.drawFastVLine(tickX, plotBottom - 2, 3, GxEPD_BLACK);
    }
  }
  display.setFont(&FreeMonoBold9pt7b);

  bool hasPreviousPoint = false;
  int16_t previousX = 0;
  int16_t previousY = 0;

  for (uint8_t logicalIndex = 0; logicalIndex < batteryHistoryCount; logicalIndex++)
  {
    uint8_t sampleIndex = batteryHistoryIndex(logicalIndex);
    uint32_t sampleSlot = batteryHistoryHours[sampleIndex];
    if (sampleSlot < startSample || sampleSlot > currentSample)
    {
      continue;
    }

    uint32_t age = sampleSlot - startSample;
    int16_t sampleX =
        plotLeft + static_cast<int16_t>((age * static_cast<uint32_t>(plotWidth)) / graphSamples);
    int16_t sampleY =
        plotBottom - static_cast<int16_t>((batteryHistoryPercents[sampleIndex] * plotHeight) / 100);

    if (hasPreviousPoint)
    {
      display.drawLine(previousX, previousY, sampleX, sampleY, GxEPD_BLACK);
    }
    display.fillRect(sampleX - 1, sampleY - 1, 3, 3, GxEPD_BLACK);

    previousX = sampleX;
    previousY = sampleY;
    hasPreviousPoint = true;
  }
}

void CityWeather::drawCity()
{
  // city & country name
  String cityName = locationData.city;
  cityName.trim();
  if (cityName == "") {cityName = "City name";};
  OpenSansCondensed::printCentered(
      display,
      OpenSansCondBoldCyrillic9pt,
      cityName,
      153,
      72,
      82,
      GxEPD_BLACK
  );

  display.drawBitmap(0, 24, city, 200, 80, GxEPD_BLACK);
}

void CityWeather::drawTip()
{
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "To display the calendar", 100, 120, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "and weather forecast", 100, 140, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "you need to set up Wifi", 100, 160, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "using the Watchy menu", 100, 180, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "<---", 100, 200, 192, GxEPD_BLACK);   
}

void CityWeather::drawWeatherUnavailable()
{
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "Weather data unavailable", 100, 130, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "Will retry later", 100, 150, 192, GxEPD_BLACK);
}

void CityWeather::drawCalendar(bool showWeather)
{
  DailyForecast currentWeek[7];
  cityWeatherService.getCurrentWeekForecast(currentWeek);

  for (int i = 0; i < 7; i++)
  {
    // fill current day
    tmElements_t tmNow;
    Watchy::RTC.read(tmNow);
    uint32_t today = uint32_t(tmNow.Year + 1970) * 10000 + uint32_t(tmNow.Month) * 100 + uint32_t(tmNow.Day);
    if (currentWeek[i].date == today)
    {
      fillRect(display, 1 + i*28, 105, 28, 200 - 105, GxEPD_BLACK, 2);
    }

    display.setTextColor(GxEPD_BLACK);

    // weekday
    display.setFont(&FreeMonoBold7pt7b);
    printCentered(display, currentWeek[i].weekDay, (i*28) + 16, 115);

    // day
    display.setFont(&FreeMonoBold9pt7b);
    int day = currentWeek[i].date % 100;
    printCentered(display, (String)day, (i*28) + 14, 132);
    
    if (showWeather)
    {
      // weather
      const unsigned char* weatherIcon = cityWeatherService.weatherNameFromCode(currentWeek[i].weatherCode);
      display.drawBitmap(i*28 + 3, 137, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK, GxEPD_WHITE);

      // tMax & tMin
      String tMax = currentWeek[i].tempMax > 0 ? "+" + (String)currentWeek[i].tempMax : (String)currentWeek[i].tempMax;
      String tMin = currentWeek[i].tempMin > 0 ? "+" + (String)currentWeek[i].tempMin : (String)currentWeek[i].tempMin;
      OpenSansCondensed::printCenteredOutlined(display, OpenSansCondBoldCyrillic9pt, tMax, (i*28) + 14, 178, 28, GxEPD_BLACK, GxEPD_WHITE);
      OpenSansCondensed::printCenteredOutlined(display, OpenSansCondBoldCyrillic9pt, tMin, (i*28) + 14, 196, 28, GxEPD_BLACK, GxEPD_WHITE);
    }
    
    // lines between days
    if (i > 0)
    {
      drawLine(display, 1 + i*28, 95, 1 + i*28, 200);
    }

    if (currentWeek[i].date == today)
    {
      display.drawFastVLine (i*28 + 1, 104, 200-104, GxEPD_BLACK);
      display.drawFastVLine (i*28 + 29, 104, 200-104, GxEPD_BLACK);
    }
  }
  
  drawLine(display, 1, 135, 199, 135, GxEPD_BLACK, 2); // day bottom
}

void CityWeather::drawWatchFace()
{
  Watchy::RTC.read(currentTime);
  if (!isNotificationsActive() && cityWeatherService.updateWifiData())
  {
    Watchy::RTC.read(currentTime);
  }
  drawWatchFaceContent();
};

void CityWeather::drawWatchFaceContent()
{
  display.fillScreen(GxEPD_WHITE);

  drawStatusBar();
  drawCity();

  const bool hasForecastData = cityWeatherService.hasCurrentWeekForecastData();
  if (!hasForecastData && !WIFI_CONFIGURED) {
    drawTip();
  } else if (!hasForecastData) {
    drawWeatherUnavailable();
  } else {
    drawCalendar();
  }
};

void CityWeather::showMinuteTick()
{
  if (isNotificationsActive())
  {
    return;
  }

  if (cityWeatherService.updateWifiData())
  {
    Watchy::RTC.read(currentTime);
    display.setFullWindow();
    display.epd2.asyncPowerOn();
    drawWatchFaceContent();
    display.display(true);
    guiState = WATCHFACE_STATE;
    return;
  }

  display.setFullWindow();
  display.epd2.asyncPowerOn();
  display.fillScreen(GxEPD_WHITE);
  drawTime();
  display.displayWindow(0, 0, 80, 21);
  guiState = WATCHFACE_STATE;
}

void CityWeather::showAppTick()
{
  if (isAboutScreenActive())
  {
    refreshAboutUpdateCheckIfNeeded();
    refreshAboutBatteryGraphIfNeeded();
  }
}

void CityWeather::showNotifications()
{
  if (notificationService.isActive())
  {
    stopNotifications();
    return;
  }

  notificationService.start(*this);
  Watchy::showMenu(menuIndex, true);
}

void CityWeather::stopNotifications()
{
  notificationService.stop();
  Watchy::showMenu(menuIndex, true);
}

void watchyMinuteTick(Watchy *watchy)
{
  static_cast<CityWeather *>(watchy)->showMinuteTick();
}

void watchyAppTick(Watchy *watchy)
{
  static_cast<CityWeather *>(watchy)->showAppTick();
}

void watchyNotificationsSelected(Watchy *watchy)
{
  static_cast<CityWeather *>(watchy)->showNotifications();
}

bool watchyShouldDeepSleep(Watchy *watchy)
{
  CityWeather *cityWeather = static_cast<CityWeather *>(watchy);
  return !cityWeather->isNotificationsActive() && !cityWeather->isAboutScreenActive();
}

bool watchyNotificationsEnabled(Watchy *watchy)
{
  return static_cast<CityWeather *>(watchy)->isNotificationsActive();
}

void watchyWifiConfigured(Watchy *watchy)
{
  rememberCityWeatherWiFiState();
  rememberCityWeatherWiFiCredentials();
  resetCityWeatherNetworkCache();
  static_cast<CityWeather *>(watchy)->refreshWeatherAfterWiFiConfigured();
}

void watchyMenuLoop(Watchy *watchy)
{
  CityWeather *cityWeather = static_cast<CityWeather *>(watchy);
  if (cityWeather->isAboutScreenActive())
  {
    cityWeather->showAppTick();
    return;
  }
  cityWeather->updateMenuStatusBar();
}

void watchyMenuShown(Watchy *watchy)
{
  static_cast<CityWeather *>(watchy)->updateMenuStatusBar(true, false);
}

bool watchyShowAbout(Watchy *watchy)
{
  static_cast<CityWeather *>(watchy)->showAboutScreen();
  return true;
}
