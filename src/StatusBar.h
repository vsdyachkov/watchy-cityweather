#pragma once

#include <Arduino.h>
#include <Watchy.h>

void restoreCityWeatherWiFiState();
void rememberCityWeatherWiFiState();
void rememberCityWeatherWiFiCredentials();
bool connectCityWeatherStoredWiFi(uint32_t timeoutMs = 10000);
void drawCityWeatherStatusBar(
    Watchy &watchy,
    bool bluetoothVisible,
    const char *notificationCounter = nullptr
);
