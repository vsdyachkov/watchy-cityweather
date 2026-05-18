#include "StatusBar.h"

#include <Fonts/FreeSansBold12pt7b.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <stddef.h>
#include <string.h>

#include "Images.h"
#include "OpenSansCondBoldCyrillic9pt.h"

extern uint32_t lastIPAddress;
extern char lastSSID[30];

namespace
{
constexpr int16_t STATUS_BAR_BATTERY_RIGHT = 196;
constexpr int16_t STATUS_BAR_ICON_Y = 3;
constexpr int16_t STATUS_BAR_BLUETOOTH_ICON_Y = 2;
constexpr int16_t STATUS_BAR_ICON_WIDTH = 19;
constexpr int16_t STATUS_BAR_ICON_HEIGHT = 16;
constexpr int16_t STATUS_BAR_ICON_GAP = 4;
constexpr uint32_t WIFI_STATE_STORAGE_MAGIC = 0x43575746;
constexpr uint32_t WIFI_CREDENTIALS_STORAGE_MAGIC = 0x43575743;
constexpr uint16_t WIFI_STATE_STORAGE_VERSION = 1;
constexpr const char *WIFI_STATE_STORAGE_NAMESPACE = "cw-wifi";
constexpr const char *WIFI_STATE_STORAGE_KEY = "state";
constexpr const char *WIFI_CREDENTIALS_STORAGE_KEY = "credentials";

struct WiFiStateSnapshot
{
    uint32_t magic;
    uint16_t version;
    bool configured;
    uint8_t reserved;
    uint32_t ipAddress;
    char ssid[30];
    uint32_t checksum;
};

struct WiFiCredentialsSnapshot
{
    uint32_t magic;
    uint16_t version;
    char ssid[33];
    char password[65];
    uint32_t checksum;
};

RTC_DATA_ATTR uint32_t wifiStateLoadedMagic = 0;
RTC_DATA_ATTR bool storedWiFiConfigured = false;
RTC_DATA_ATTR uint32_t storedWiFiIPAddress = 0;
RTC_DATA_ATTR char storedWiFiSSID[30] = "";

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

uint32_t wifiStateSnapshotChecksum(const WiFiStateSnapshot &snapshot)
{
    return checksumBytes(
        reinterpret_cast<const uint8_t *>(&snapshot),
        offsetof(WiFiStateSnapshot, checksum)
    );
}

uint32_t wifiCredentialsSnapshotChecksum(const WiFiCredentialsSnapshot &snapshot)
{
    return checksumBytes(
        reinterpret_cast<const uint8_t *>(&snapshot),
        offsetof(WiFiCredentialsSnapshot, checksum)
    );
}

void copyWifiField(char *destination, size_t destinationSize, const uint8_t *source, size_t sourceSize)
{
    if (destinationSize == 0)
    {
        return;
    }

    size_t length = 0;
    while (length < sourceSize && source[length] != 0 && length + 1 < destinationSize)
    {
        destination[length] = static_cast<char>(source[length]);
        length++;
    }
    destination[length] = '\0';
}

bool loadWiFiStateFromStorage()
{
    Preferences preferences;
    if (!preferences.begin(WIFI_STATE_STORAGE_NAMESPACE, true))
    {
        return false;
    }

    size_t storedLength = preferences.getBytesLength(WIFI_STATE_STORAGE_KEY);
    if (storedLength != sizeof(WiFiStateSnapshot))
    {
        preferences.end();
        return false;
    }

    WiFiStateSnapshot snapshot = {};
    size_t readLength = preferences.getBytes(WIFI_STATE_STORAGE_KEY, &snapshot, sizeof(snapshot));
    preferences.end();

    if (readLength != sizeof(snapshot))
    {
        return false;
    }
    if (
        snapshot.magic != WIFI_STATE_STORAGE_MAGIC ||
        snapshot.version != WIFI_STATE_STORAGE_VERSION ||
        snapshot.checksum != wifiStateSnapshotChecksum(snapshot)
    )
    {
        return false;
    }

    snapshot.ssid[sizeof(snapshot.ssid) - 1] = '\0';
    storedWiFiConfigured = snapshot.configured && snapshot.ssid[0] != '\0';
    storedWiFiIPAddress = snapshot.ipAddress;
    strncpy(storedWiFiSSID, snapshot.ssid, sizeof(storedWiFiSSID) - 1);
    storedWiFiSSID[sizeof(storedWiFiSSID) - 1] = '\0';
    return storedWiFiConfigured;
}

bool loadWiFiCredentialsFromStorage(WiFiCredentialsSnapshot &snapshot)
{
    Preferences preferences;
    if (!preferences.begin(WIFI_STATE_STORAGE_NAMESPACE, true))
    {
        return false;
    }

    size_t storedLength = preferences.getBytesLength(WIFI_CREDENTIALS_STORAGE_KEY);
    if (storedLength != sizeof(WiFiCredentialsSnapshot))
    {
        preferences.end();
        return false;
    }

    size_t readLength =
        preferences.getBytes(WIFI_CREDENTIALS_STORAGE_KEY, &snapshot, sizeof(snapshot));
    preferences.end();

    if (readLength != sizeof(snapshot))
    {
        return false;
    }
    if (
        snapshot.magic != WIFI_CREDENTIALS_STORAGE_MAGIC ||
        snapshot.version != WIFI_STATE_STORAGE_VERSION ||
        snapshot.checksum != wifiCredentialsSnapshotChecksum(snapshot)
    )
    {
        return false;
    }

    snapshot.ssid[sizeof(snapshot.ssid) - 1] = '\0';
    snapshot.password[sizeof(snapshot.password) - 1] = '\0';
    return snapshot.ssid[0] != '\0';
}

void saveWiFiStateToStorage()
{
    WiFiStateSnapshot snapshot = {};
    snapshot.magic = WIFI_STATE_STORAGE_MAGIC;
    snapshot.version = WIFI_STATE_STORAGE_VERSION;
    snapshot.configured = storedWiFiConfigured;
    snapshot.ipAddress = storedWiFiIPAddress;
    strncpy(snapshot.ssid, storedWiFiSSID, sizeof(snapshot.ssid) - 1);
    snapshot.ssid[sizeof(snapshot.ssid) - 1] = '\0';
    snapshot.checksum = wifiStateSnapshotChecksum(snapshot);

    Preferences preferences;
    if (!preferences.begin(WIFI_STATE_STORAGE_NAMESPACE, false))
    {
        return;
    }
    preferences.putBytes(WIFI_STATE_STORAGE_KEY, &snapshot, sizeof(snapshot));
    preferences.end();
}

void saveWiFiCredentialsToStorage(const char *ssid, const char *password)
{
    if (ssid == nullptr || ssid[0] == '\0')
    {
        return;
    }

    WiFiCredentialsSnapshot snapshot = {};
    snapshot.magic = WIFI_CREDENTIALS_STORAGE_MAGIC;
    snapshot.version = WIFI_STATE_STORAGE_VERSION;
    strncpy(snapshot.ssid, ssid, sizeof(snapshot.ssid) - 1);
    snapshot.ssid[sizeof(snapshot.ssid) - 1] = '\0';
    if (password != nullptr)
    {
        strncpy(snapshot.password, password, sizeof(snapshot.password) - 1);
        snapshot.password[sizeof(snapshot.password) - 1] = '\0';
    }
    snapshot.checksum = wifiCredentialsSnapshotChecksum(snapshot);

    Preferences preferences;
    if (!preferences.begin(WIFI_STATE_STORAGE_NAMESPACE, false))
    {
        return;
    }
    preferences.putBytes(WIFI_CREDENTIALS_STORAGE_KEY, &snapshot, sizeof(snapshot));
    preferences.end();
}

void printRightAligned(Adafruit_GFX &display, int16_t xRight, int16_t y, const String &text)
{
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;
    display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);
    display.setCursor(xRight - static_cast<int16_t>(width), y);
    display.print(text);
}

int16_t textStartForRightAligned(Adafruit_GFX &display, int16_t xRight, const String &text)
{
    int16_t x1;
    int16_t y1;
    uint16_t width;
    uint16_t height;
    display.getTextBounds(text, 0, 0, &x1, &y1, &width, &height);
    return xRight - static_cast<int16_t>(width);
}

int16_t drawConnectivityIcons(
    Adafruit_GFX &display,
    int16_t rightEdge,
    bool bluetoothVisible,
    bool wifiVisible
)
{
    int16_t leftEdge = rightEdge;
    if (wifiVisible)
    {
        int16_t wifiX = rightEdge - STATUS_BAR_ICON_WIDTH;
        display.drawBitmap(
            wifiX,
            STATUS_BAR_ICON_Y,
            wifi,
            STATUS_BAR_ICON_WIDTH,
            STATUS_BAR_ICON_HEIGHT,
            GxEPD_BLACK
        );
        leftEdge = wifiX;
        rightEdge = wifiX - STATUS_BAR_ICON_GAP;
    }

    if (bluetoothVisible)
    {
        int16_t bluetoothX = rightEdge - STATUS_BAR_ICON_WIDTH;
        display.drawBitmap(
            bluetoothX,
            STATUS_BAR_BLUETOOTH_ICON_Y,
            bluetooth,
            STATUS_BAR_ICON_WIDTH,
            STATUS_BAR_ICON_HEIGHT,
            GxEPD_BLACK
        );
        leftEdge = bluetoothX;
    }
    return leftEdge;
}

void drawDitheredHorizontalLine(
    Adafruit_GFX &display,
    int16_t x0,
    int16_t x1,
    int16_t y,
    uint16_t color,
    uint8_t step
)
{
    for (int16_t x = x0; x <= x1; x++)
    {
        if ((x - x0) % step == 0)
        {
            display.drawPixel(x, y, color);
        }
    }
}
}

uint8_t statusBarBatteryPercentFromVoltage(float voltage)
{
    return constrain(static_cast<int>((voltage - 3.0f) * 83.33f), 0, 100);
}

void restoreCityWeatherWiFiState()
{
    if (wifiStateLoadedMagic != WIFI_STATE_STORAGE_MAGIC)
    {
        loadWiFiStateFromStorage();
        wifiStateLoadedMagic = WIFI_STATE_STORAGE_MAGIC;
    }

    if (!storedWiFiConfigured)
    {
        WiFiCredentialsSnapshot credentials = {};
        if (loadWiFiCredentialsFromStorage(credentials))
        {
            storedWiFiConfigured = true;
            storedWiFiIPAddress = 0;
            strncpy(storedWiFiSSID, credentials.ssid, sizeof(storedWiFiSSID) - 1);
            storedWiFiSSID[sizeof(storedWiFiSSID) - 1] = '\0';
            saveWiFiStateToStorage();
        }
    }

    if (!storedWiFiConfigured)
    {
        return;
    }

    WIFI_CONFIGURED = true;
    lastIPAddress = storedWiFiIPAddress;
    strncpy(lastSSID, storedWiFiSSID, 29);
    lastSSID[29] = '\0';
}

void rememberCityWeatherWiFiState()
{
    lastSSID[29] = '\0';
    if (lastSSID[0] == '\0')
    {
        return;
    }
    WIFI_CONFIGURED = true;

    if (
        storedWiFiConfigured &&
        storedWiFiIPAddress == lastIPAddress &&
        strncmp(storedWiFiSSID, lastSSID, sizeof(storedWiFiSSID)) == 0
    )
    {
        return;
    }

    storedWiFiConfigured = true;
    storedWiFiIPAddress = lastIPAddress;
    strncpy(storedWiFiSSID, lastSSID, sizeof(storedWiFiSSID) - 1);
    storedWiFiSSID[sizeof(storedWiFiSSID) - 1] = '\0';
    wifiStateLoadedMagic = WIFI_STATE_STORAGE_MAGIC;
    saveWiFiStateToStorage();
}

void rememberCityWeatherWiFiCredentials()
{
    wifi_config_t config = {};
    if (esp_wifi_get_config(WIFI_IF_STA, &config) != ESP_OK)
    {
        return;
    }

    char ssid[33] = "";
    char password[65] = "";
    copyWifiField(ssid, sizeof(ssid), config.sta.ssid, sizeof(config.sta.ssid));
    copyWifiField(password, sizeof(password), config.sta.password, sizeof(config.sta.password));
    if (ssid[0] == '\0')
    {
        return;
    }

    saveWiFiCredentialsToStorage(ssid, password);
    if (lastSSID[0] == '\0')
    {
        strncpy(lastSSID, ssid, 29);
        lastSSID[29] = '\0';
    }
    rememberCityWeatherWiFiState();
}

bool connectCityWeatherStoredWiFi(uint32_t timeoutMs)
{
    WiFiCredentialsSnapshot snapshot = {};
    if (!loadWiFiCredentialsFromStorage(snapshot))
    {
        return false;
    }

    WiFi.mode(WIFI_STA);
    WiFi.persistent(true);
    WiFi.begin(snapshot.ssid, snapshot.password);
    if (WiFi.waitForConnectResult(timeoutMs) != WL_CONNECTED)
    {
        WIFI_CONFIGURED = true;
        strncpy(lastSSID, snapshot.ssid, 29);
        lastSSID[29] = '\0';
        rememberCityWeatherWiFiState();
        return false;
    }

    lastIPAddress = WiFi.localIP();
    WiFi.SSID().toCharArray(lastSSID, 30);
    WIFI_CONFIGURED = true;
    rememberCityWeatherWiFiState();
    rememberCityWeatherWiFiCredentials();
    return true;
}

void drawCityWeatherStatusBar(
    Watchy &watchy,
    bool bluetoothVisible,
    const char *notificationCounter
)
{
    restoreCityWeatherWiFiState();
    rememberCityWeatherWiFiState();
    Watchy::RTC.read(watchy.currentTime);

    String timeText =
        (watchy.currentTime.Hour < 10 ? "0" : "") + String(watchy.currentTime.Hour) + ":" +
        (watchy.currentTime.Minute < 10 ? "0" : "") + String(watchy.currentTime.Minute);

    Watchy::display.setTextColor(GxEPD_BLACK);
    Watchy::display.setFont(&FreeSansBold12pt7b);
    Watchy::display.setCursor(-1, 17);
    Watchy::display.print(timeText);
    int16_t timeX1;
    int16_t timeY1;
    uint16_t timeWidth;
    uint16_t timeHeight;
    Watchy::display.getTextBounds(timeText, -1, 17, &timeX1, &timeY1, &timeWidth, &timeHeight);
    int16_t timeRight = timeX1 + static_cast<int16_t>(timeWidth);

    Watchy::display.setFont(&FreeMonoBold9pt7b);
    uint8_t batteryPercent = statusBarBatteryPercentFromVoltage(watchy.getBatteryVoltage());
    String batteryText = String(batteryPercent) + "%";
    int16_t batterySlotLeft =
        textStartForRightAligned(Watchy::display, STATUS_BAR_BATTERY_RIGHT, "100%");
    int16_t iconRight = batterySlotLeft - STATUS_BAR_ICON_GAP;
    int16_t iconsLeft =
        drawConnectivityIcons(Watchy::display, iconRight, bluetoothVisible, WIFI_CONFIGURED);
    if (notificationCounter != nullptr && notificationCounter[0] != '\0')
    {
        int16_t counterLeft = timeRight + STATUS_BAR_ICON_GAP;
        int16_t counterRight = iconsLeft - STATUS_BAR_ICON_GAP;
        int16_t counterWidth = counterRight - counterLeft;
        if (counterWidth > 10)
        {
            OpenSansCondensed::printCentered(
                Watchy::display,
                OpenSansCondBoldCyrillic9pt,
                notificationCounter,
                counterLeft + counterWidth / 2,
                17,
                counterWidth,
                GxEPD_BLACK
            );
        }
    }
    printRightAligned(Watchy::display, STATUS_BAR_BATTERY_RIGHT, 16, batteryText);

    drawDitheredHorizontalLine(Watchy::display, 0, 199, 21, GxEPD_BLACK, 3);
}
