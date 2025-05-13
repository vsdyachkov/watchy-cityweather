#pragma once
#include <Watchy.h>
#include "CityWeatherService.h"

static float ditheringValue = 0.0;

class CityWeather : public Watchy
{
    public:             
        explicit CityWeather(const watchySettings &settings);
        void drawWatchFace();

        void drawStatusBar();
        void drawCity();
        void drawCalendar();

    private:
        CityWeatherService cityWeatherService;
        
        virtual void handleButtonPress();
};

inline void CityWeather::handleButtonPress()
{
    if (guiState == WATCHFACE_STATE)
    {
        // Up and Down switch watch faces
        uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
        if (wakeupBit & UP_BTN_MASK)  {
            ditheringValue += 0.1;
            if (ditheringValue > 1.0)
            {
                ditheringValue = 1.0;
            }
            RTC.read(currentTime);
            showWatchFace(true);
            return;
        }
        if (wakeupBit & DOWN_BTN_MASK)  {
            ditheringValue -= 0.1;
            if (ditheringValue < -1.0)
            {
                ditheringValue = -1.0;
            }
            RTC.read(currentTime);
            showWatchFace(true);
            return;
        }
        if (wakeupBit & BACK_BTN_MASK)  {
            // light = !light;
            RTC.read(currentTime);
            showWatchFace(true);
            return;
        }
        if (wakeupBit & MENU_BTN_MASK) {
            Watchy::handleButtonPress();
            return;
        }
    } else {
        Watchy::handleButtonPress();
    }
    return;
}
