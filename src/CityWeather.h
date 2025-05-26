#pragma once
#include <Watchy.h>
#include "CityWeatherService.h"

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
        
        // virtual void handleButtonPress();
};

// inline void CityWeather::handleButtonPress()
// {
//     if (guiState == WATCHFACE_STATE)
//     {
//         // Up and Down switch watch faces
//         uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
//         if (wakeupBit & UP_BTN_MASK)  {
//             return;
//         }
//         if (wakeupBit & DOWN_BTN_MASK)  {
//             return;
//         }
//         if (wakeupBit & BACK_BTN_MASK )  {
//             return;
//         }
//         if (wakeupBit & MENU_BTN_MASK) {
//             Watchy::handleButtonPress();
//             return;
//         }
//     } else {
//         Watchy::handleButtonPress();
//     }
//     return;
// }
