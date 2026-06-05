#include "BatteryMonitor.h"

namespace
{
constexpr uint8_t BATTERY_SAMPLE_COUNT = 5;
constexpr uint8_t MAX_UNCONFIRMED_DROP_PERCENT = 15;
constexpr uint8_t DROP_CONFIRMATION_TOLERANCE_PERCENT = 5;
constexpr uint8_t REQUIRED_DROP_CONFIRMATIONS = 2;
constexpr uint32_t BATTERY_MONITOR_STATE_MAGIC = 0x4357424D;

struct BatteryCurvePoint
{
    uint16_t voltageMv;
    uint8_t percent;
};

constexpr BatteryCurvePoint BATTERY_CURVE[] = {
    {4200, 100},
    {4100, 90},
    {4000, 80},
    {3920, 70},
    {3850, 60},
    {3790, 50},
    {3750, 40},
    {3700, 30},
    {3630, 20},
    {3550, 10},
    {3400, 5},
    {3300, 0},
};

RTC_DATA_ATTR uint32_t batteryMonitorStateMagic = 0;
RTC_DATA_ATTR uint8_t stableBatteryPercent = 0;
RTC_DATA_ATTR uint16_t stableBatteryVoltageMv = 0;
RTC_DATA_ATTR uint8_t pendingDropPercent = 0;
RTC_DATA_ATTR uint8_t pendingDropConfirmations = 0;

uint16_t readBatteryVoltageMv(Watchy &watchy)
{
    uint16_t samples[BATTERY_SAMPLE_COUNT] = {};
    for (uint8_t i = 0; i < BATTERY_SAMPLE_COUNT; i++)
    {
        float voltage = watchy.getBatteryVoltage();
        if (voltage < 0.0f)
        {
            voltage = 0.0f;
        }
        samples[i] = static_cast<uint16_t>((voltage * 1000.0f) + 0.5f);
        delay(1);
    }

    for (uint8_t i = 1; i < BATTERY_SAMPLE_COUNT; i++)
    {
        uint16_t value = samples[i];
        int8_t j = i - 1;
        while (j >= 0 && samples[j] > value)
        {
            samples[j + 1] = samples[j];
            j--;
        }
        samples[j + 1] = value;
    }

    uint32_t trimmedSum = 0;
    for (uint8_t i = 1; i < BATTERY_SAMPLE_COUNT - 1; i++)
    {
        trimmedSum += samples[i];
    }
    return static_cast<uint16_t>(
        (trimmedSum + ((BATTERY_SAMPLE_COUNT - 2) / 2)) / (BATTERY_SAMPLE_COUNT - 2)
    );
}

void acceptBatteryReading(uint16_t voltageMv, uint8_t percent)
{
    batteryMonitorStateMagic = BATTERY_MONITOR_STATE_MAGIC;
    stableBatteryVoltageMv = voltageMv;
    stableBatteryPercent = percent;
    pendingDropConfirmations = 0;
    pendingDropPercent = 0;
}

uint8_t stabilizedBatteryPercent(uint16_t voltageMv, uint8_t measuredPercent)
{
    if (batteryMonitorStateMagic != BATTERY_MONITOR_STATE_MAGIC)
    {
        acceptBatteryReading(voltageMv, measuredPercent);
        return stableBatteryPercent;
    }

    if (measuredPercent >= stableBatteryPercent)
    {
        acceptBatteryReading(voltageMv, measuredPercent);
        return stableBatteryPercent;
    }

    uint8_t drop = stableBatteryPercent - measuredPercent;
    if (drop <= MAX_UNCONFIRMED_DROP_PERCENT)
    {
        acceptBatteryReading(voltageMv, measuredPercent);
        return stableBatteryPercent;
    }

    uint8_t difference = pendingDropPercent > measuredPercent
        ? pendingDropPercent - measuredPercent
        : measuredPercent - pendingDropPercent;
    if (
        pendingDropConfirmations > 0 &&
        difference <= DROP_CONFIRMATION_TOLERANCE_PERCENT
    )
    {
        pendingDropConfirmations++;
    }
    else
    {
        pendingDropPercent = measuredPercent;
        pendingDropConfirmations = 1;
    }

    stableBatteryVoltageMv = voltageMv;
    if (pendingDropConfirmations >= REQUIRED_DROP_CONFIRMATIONS)
    {
        acceptBatteryReading(voltageMv, measuredPercent);
    }
    return stableBatteryPercent;
}
}

uint8_t cityWeatherBatteryPercentFromMillivolts(uint16_t voltageMv)
{
    if (voltageMv >= BATTERY_CURVE[0].voltageMv)
    {
        return BATTERY_CURVE[0].percent;
    }

    constexpr uint8_t curveLength = sizeof(BATTERY_CURVE) / sizeof(BATTERY_CURVE[0]);
    for (uint8_t i = 1; i < curveLength; i++)
    {
        const BatteryCurvePoint &upper = BATTERY_CURVE[i - 1];
        const BatteryCurvePoint &lower = BATTERY_CURVE[i];
        if (voltageMv >= lower.voltageMv)
        {
            uint16_t voltageRange = upper.voltageMv - lower.voltageMv;
            uint16_t voltageOffset = voltageMv - lower.voltageMv;
            uint8_t percentRange = upper.percent - lower.percent;
            return lower.percent + static_cast<uint8_t>(
                ((static_cast<uint32_t>(voltageOffset) * percentRange) + (voltageRange / 2)) /
                voltageRange
            );
        }
    }

    return BATTERY_CURVE[curveLength - 1].percent;
}

BatteryReading readCityWeatherBattery(Watchy &watchy)
{
    uint16_t voltageMv = readBatteryVoltageMv(watchy);
    uint8_t measuredPercent = cityWeatherBatteryPercentFromMillivolts(voltageMv);
    return {
        voltageMv,
        stabilizedBatteryPercent(voltageMv, measuredPercent),
    };
}

uint8_t cityWeatherBatteryPercent(Watchy &watchy)
{
    return readCityWeatherBattery(watchy).percent;
}
