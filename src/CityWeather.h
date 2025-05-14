#pragma once
#include <Watchy.h>
#include "CityWeatherService.h"

class CityWeather : public Watchy
{
    public:             
        explicit CityWeather(const watchySettings &settings);
        void drawWatchFace();
        void changeSkyDithering(float d);
        void printTemperature(Adafruit_GFX &d, const String &text, int16_t centerX, int16_t y);

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
            changeSkyDithering(0.1);
            return;
        }
        if (wakeupBit & DOWN_BTN_MASK)  {
            changeSkyDithering(-0.1);
            return;
        }
        if (wakeupBit & BACK_BTN_MASK )  {
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
