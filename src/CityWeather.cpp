#include <vector>
#include "Adafruit_GFX_ext.h"
#include "Images.h"
#include "FreeMonoBold7pt7b.h"
#include "OpenSans_CondBold9pt7b.h"
#include <Fonts/FreeSansBold12pt7b.h>
#include "CityWeather.h"
#include "CityWeatherService.h"

CityWeather::CityWeather(const watchySettings &settings_) : Watchy(settings_), cityWeatherService(*this) {}

const uint8_t WEATHER_ICON_WIDTH = 25;
const uint8_t WEATHER_ICON_HEIGHT = 25;
extern time_t savedTime;

void CityWeather::drawStatusBar()
{
  // time
  String timeStr =
      (currentTime.Hour < 10 ? "0" : "") + String(currentTime.Hour) + ":" +
      (currentTime.Minute < 10 ? "0" : "") + String(currentTime.Minute);

  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBold12pt7b);
  display.setCursor(-1, 17);
  display.print(timeStr);

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
  display.setFont(&OpenSans_CondBold9pt7b);
  String clippedCityName = clipStringToWidth (display, &OpenSans_CondBold9pt7b, locationData.city, 82);
  if (clippedCityName == "") {clippedCityName = "City name";};
  printCentered(display, clippedCityName, 153, 72);

  display.drawBitmap(0, 24, city, 200, 80, GxEPD_BLACK);
}

void CityWeather::drawTip()
{
    display.setFont(&OpenSans_CondBold9pt7b);
    printCentered(display, "To display the calendar", 100, 120);
    printCentered(display, "and weather forecast", 100, 140);
    printCentered(display, "you need to set up Wifi", 100, 160);
    printCentered(display, "using the Watchy menu", 100, 180);
    printCentered(display, "<---", 100, 200);   
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
    display.setFont(&OpenSans_CondBold9pt7b);
    String tMax = currentWeek[i].tempMax > 0 ? "+" + (String)currentWeek[i].tempMax : (String)currentWeek[i].tempMax;
    String tMin = currentWeek[i].tempMin > 0 ? "+" + (String)currentWeek[i].tempMin : (String)currentWeek[i].tempMin;
    printCentered (display, tMax.c_str(), (i*28) + 14, 178);
    printCentered (display, tMin.c_str(), (i*28) + 14, 196);
    
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
  cityWeatherService.updateWifiData();  

  display.fillScreen(GxEPD_WHITE);

  drawStatusBar();
  drawCity();

  if (savedTime == 0 && !WIFI_CONFIGURED) {
    drawTip();
  } else {
    drawCalendar();
  }

};