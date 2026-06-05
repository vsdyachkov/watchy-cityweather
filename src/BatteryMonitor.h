#pragma once

#include <Arduino.h>
#include <Watchy.h>

struct BatteryReading
{
    uint16_t voltageMv;
    uint8_t percent;
};

BatteryReading readCityWeatherBattery(Watchy &watchy);
uint8_t cityWeatherBatteryPercent(Watchy &watchy);
uint8_t cityWeatherBatteryPercentFromMillivolts(uint16_t voltageMv);
