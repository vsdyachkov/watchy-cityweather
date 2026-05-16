#include <vector>
#include "Adafruit_GFX_ext.h"
#include "Images.h"
#include "FreeMonoBold7pt7b.h"
#include "OpenSansCondBoldCyrillic9pt.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include "CityWeather.h"
#include "CityWeatherService.h"

CityWeather::CityWeather(const watchySettings &settings_) : Watchy(settings_), cityWeatherService(*this) {}

const uint8_t WEATHER_ICON_WIDTH = 25;
const uint8_t WEATHER_ICON_HEIGHT = 25;

void CityWeather::drawTime()
{
  String timeStr =
      (currentTime.Hour < 10 ? "0" : "") + String(currentTime.Hour) + ":" +
      (currentTime.Minute < 10 ? "0" : "") + String(currentTime.Minute);

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(-1, 17);
  display.print(timeStr);
}

void CityWeather::drawStatusBar()
{
  drawTime();

  display.drawBitmap(136, 3, wifi, 19, 16, 0);
  if (!WIFI_CONFIGURED) {
    display.drawLine (139, 3, 139+12, 3+14, GxEPD_BLACK);
    display.drawLine (140, 3, 140+12, 3+14, GxEPD_BLACK);
  }

  // battery
  // display.drawBitmap(143, 1, battery, 9, 15, GxEPD_BLACK);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  float voltage = getBatteryVoltage();
  int batteryPercent = constrain((voltage - 3.3) * 111.11, 0, 100);
  String batteryStr = String(batteryPercent) + "%";
  drawTextRightAligned(display, 196, 16, batteryStr);

  drawLine (display, 0, 21, 199, 21, GxEPD_BLACK, 3);
}

void CityWeather::drawCity()
{
  // city & country name
  String cityName = locationData.city;
  cityName.trim();
  if (cityName == "") {cityName = "City name";};
  OpenSansCondensed::printCentered(
      display,
      OpenSansCondBoldCyrillic9pt,
      cityName,
      153,
      72,
      82,
      GxEPD_BLACK
  );

  display.drawBitmap(0, 24, city, 200, 80, GxEPD_BLACK);
}

void CityWeather::drawTip()
{
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "To display the calendar", 100, 120, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "and weather forecast", 100, 140, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "you need to set up Wifi", 100, 160, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "using the Watchy menu", 100, 180, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "<---", 100, 200, 192, GxEPD_BLACK);   
}

void CityWeather::drawWeatherUnavailable()
{
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "Weather data unavailable", 100, 130, 192, GxEPD_BLACK);
    OpenSansCondensed::printCentered(display, OpenSansCondBoldCyrillic9pt, "Will retry later", 100, 150, 192, GxEPD_BLACK);
}

void CityWeather::drawCalendar()
{
  DailyForecast currentWeek[7];
  cityWeatherService.getCurrentWeekForecast(currentWeek);

  for (int i = 0; i < 7; i++)
  {
    // fill current day
    tmElements_t tmNow;
    Watchy::RTC.read(tmNow);
    uint32_t today = uint32_t(tmNow.Year + 1970) * 10000 + uint32_t(tmNow.Month) * 100 + uint32_t(tmNow.Day);
    if (currentWeek[i].date == today)
    {
      fillRect(display, 1 + i*28, 105, 28, 200 - 105, GxEPD_BLACK, 2);
    }

    display.setTextColor(GxEPD_BLACK);

    // weekday
    display.setFont(&FreeMonoBold7pt7b);
    printCentered(display, currentWeek[i].weekDay, (i*28) + 16, 115);

    // day
    display.setFont(&FreeMonoBold9pt7b);
    int day = currentWeek[i].date % 100;
    printCentered(display, (String)day, (i*28) + 14, 132);
    
    // weather
    const unsigned char* weatherIcon = cityWeatherService.weatherNameFromCode(currentWeek[i].weatherCode);
    display.drawBitmap(i*28 + 3, 137, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK, GxEPD_WHITE);

    // tMax & tMin
    String tMax = currentWeek[i].tempMax > 0 ? "+" + (String)currentWeek[i].tempMax : (String)currentWeek[i].tempMax;
    String tMin = currentWeek[i].tempMin > 0 ? "+" + (String)currentWeek[i].tempMin : (String)currentWeek[i].tempMin;
    OpenSansCondensed::printCenteredOutlined(display, OpenSansCondBoldCyrillic9pt, tMax, (i*28) + 14, 178, 28, GxEPD_BLACK, GxEPD_WHITE);
    OpenSansCondensed::printCenteredOutlined(display, OpenSansCondBoldCyrillic9pt, tMin, (i*28) + 14, 196, 28, GxEPD_BLACK, GxEPD_WHITE);
    
    // lines between days
    if (i > 0)
    {
      drawLine(display, 1 + i*28, 95, 1 + i*28, 200);
    }

    if (currentWeek[i].date == today)
    {
      display.drawFastVLine (i*28 + 1, 104, 200-104, GxEPD_BLACK);
      display.drawFastVLine (i*28 + 29, 104, 200-104, GxEPD_BLACK);
    }
  }
  
  drawLine(display, 1, 135, 199, 135, GxEPD_BLACK, 2); // day bottom
}

void CityWeather::drawWatchFace()
{
  Watchy::RTC.read(currentTime);
  if (!isNotificationsActive() && cityWeatherService.updateWifiData())
  {
    Watchy::RTC.read(currentTime);
  }
  drawWatchFaceContent();
};

void CityWeather::drawWatchFaceContent()
{
  display.fillScreen(GxEPD_WHITE);

  drawStatusBar();
  drawCity();

  const bool hasForecastData = cityWeatherService.hasForecastData();
  if (isNotificationsActive() && !hasForecastData) {
    return;
  }

  if (!hasForecastData && !WIFI_CONFIGURED) {
    drawTip();
  } else if (!hasForecastData) {
    drawWeatherUnavailable();
  } else {
    drawCalendar();
  }
};

void CityWeather::showMinuteTick()
{
  if (isNotificationsActive())
  {
    return;
  }

  if (cityWeatherService.updateWifiData())
  {
    Watchy::RTC.read(currentTime);
    display.setFullWindow();
    display.epd2.asyncPowerOn();
    drawWatchFaceContent();
    display.display(true);
    guiState = WATCHFACE_STATE;
    return;
  }

  display.setFullWindow();
  display.epd2.asyncPowerOn();
  display.fillScreen(GxEPD_WHITE);
  drawTime();
  display.displayWindow(0, 0, 80, 21);
  guiState = WATCHFACE_STATE;
}

void CityWeather::showNotifications()
{
  if (notificationService.isActive())
  {
    stopNotifications();
    return;
  }

  notificationService.start(*this);
  Watchy::showMenu(menuIndex, true);
}

void CityWeather::stopNotifications()
{
  notificationService.stop();
  Watchy::showMenu(menuIndex, true);
}

void watchyMinuteTick(Watchy *watchy)
{
  static_cast<CityWeather *>(watchy)->showMinuteTick();
}

void watchyNotificationsSelected(Watchy *watchy)
{
  static_cast<CityWeather *>(watchy)->showNotifications();
}

bool watchyShouldDeepSleep(Watchy *watchy)
{
  return !static_cast<CityWeather *>(watchy)->isNotificationsActive();
}

bool watchyNotificationsEnabled(Watchy *watchy)
{
  return static_cast<CityWeather *>(watchy)->isNotificationsActive();
}

void watchyWifiConfigured(Watchy *)
{
  resetCityWeatherNetworkCache();
}
