#pragma once
#include <Watchy.h>
#include "CityWeatherService.h"
#include "NotificationService.h"

class CityWeather : public Watchy
{
    public:             
        explicit CityWeather(const watchySettings &settings);
        void drawWatchFace();
        void showMinuteTick();
        void showNotifications();
        void stopNotifications();
        void loop();
        bool isNotificationsActive() const;

        void drawTime();
        void drawStatusBar();
        void drawCity();
        void drawCalendar();
        void drawTip();
        void drawWeatherUnavailable();
        void drawWatchFaceContent();

    private:
        CityWeatherService cityWeatherService;
        NotificationService notificationService;
        
        virtual void handleButtonPress();
};

inline void CityWeather::handleButtonPress()
{
    if (guiState == WATCHFACE_STATE)
    {
        // Up and Down switch watch faces
        uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
        if (wakeupBit & UP_BTN_MASK)  {
            Watchy::showWatchFace(true);
            return;
        }
        if (wakeupBit & DOWN_BTN_MASK)  {
            Watchy::showWatchFace(true);
            return;
        }
        if (wakeupBit & BACK_BTN_MASK )  {
            Watchy::showWatchFace(true);
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

inline void CityWeather::loop()
{
    notificationService.tick(*this);
    delay(20);
}

inline bool CityWeather::isNotificationsActive() const
{
    return notificationService.isActive();
}
