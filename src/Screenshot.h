#pragma once

#include <Arduino.h>
#include <Watchy.h>

bool handleCityWeatherScreenshotShortcut(Watchy *watchy, uint16_t holdMs = 1200);
void dumpCityWeatherScreenshotToSerial();
