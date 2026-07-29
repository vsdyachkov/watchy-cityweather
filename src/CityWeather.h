#pragma once
#include <Watchy.h>
#include "CityWeatherService.h"
#include "NotificationService.h"
#include "Screenshot.h"
#include "StatusBar.h"

class CityWeather : public Watchy
{
    public:             
        explicit CityWeather(const watchySettings &settings);
        void drawWatchFace();
        void showMinuteTick();
        void showAppTick();
        void showNotifications();
        void stopNotifications();
        void loop();
        bool isNotificationsActive() const;
        bool connectWiFi();
        bool refreshWeatherAfterWiFiConfigured();
        void updateMenuStatusBar(bool force = false, bool refresh = true);
        void resetMenuStatusBarRefresh();
        void showAboutScreen();
        bool isAboutScreenActive() const;
        void refreshCachedReleaseStatusIfDue();

        void drawTime();
        void drawStatusBar();
        void drawCity();
        void drawCalendar(bool showWeather = true);
        void drawTip();
        void drawWeatherUnavailable();
        void drawWatchFaceContent();

    private:
        CityWeatherService cityWeatherService;
        NotificationService notificationService;
        bool menuStatusBarDrawn = false;
        bool aboutScreenVisible = false;
        uint8_t lastMenuStatusMinute = 255;
        uint32_t lastMenuStatusBarRefreshAtMs = 0;
        uint32_t lastAboutButtonActionAtMs = 0;
        
        virtual void handleButtonPress();
        void handleAboutScreenLoop();
        void recordBatteryHistory();
        String fetchLatestReleaseStatus();
        String batteryRuntimeEstimateText();
        void drawAboutScreenContent(const String &updateStatus);
        void refreshAboutBatteryGraphIfNeeded();
        void drawBatteryHistoryGraph(int16_t x, int16_t y, int16_t w, int16_t h);

    virtual void onMinuteTick();
    virtual void onAppTick();
    virtual bool shouldDeepSleep();
    virtual bool screenshotRequested();
    virtual bool notificationsEnabled();
    virtual void onWifiConfigured();
    virtual void onMenuLoop();
    virtual void onMenuShown();
    virtual bool handleAbout();
    virtual void onNotificationsSelected();
};

inline void CityWeather::handleButtonPress()
{
    if (guiState == WATCHFACE_STATE)
    {
        // Up and Down switch watch faces
        uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
        if ((wakeupBit & MENU_BTN_MASK) && handleCityWeatherScreenshotShortcut(this)) {
            return;
        }
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
            restoreCityWeatherWiFiState();
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
    if (guiState == APP_STATE && menuIndex == 4)
    {
        handleAboutScreenLoop();
    }
    if (notificationService.isMenuVisible())
    {
        updateMenuStatusBar();
    }
    else
    {
        resetMenuStatusBarRefresh();
    }
    delay(20);
}

inline bool CityWeather::isNotificationsActive() const
{
    return notificationService.isActive();
}

inline bool CityWeather::isAboutScreenActive() const
{
    return aboutScreenVisible && guiState == APP_STATE && menuIndex == 4;
}
