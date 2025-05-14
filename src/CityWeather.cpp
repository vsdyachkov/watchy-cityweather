#include <vector>
#include "Adafruit_GFX_ext.h"
#include "Images.h"
#include "FreeMonoBold7pt7b.h"
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "CityWeather.h"
#include "CityWeatherService.h"

CityWeather::CityWeather(const watchySettings &settings_) : Watchy(settings_), cityWeatherService(*this) {}

RTC_DATA_ATTR float ditheringValue = 0.3f;
const uint8_t WEATHER_ICON_WIDTH = 25;
const uint8_t WEATHER_ICON_HEIGHT = 25;

/*
#define STEPSGOAL 5000

const uint8_t WEATHER_ICON_WIDTH = 48;
const uint8_t WEATHER_ICON_HEIGHT = 32;

RTC_DATA_ATTR uint8_t vaultBoyNum;


void WatchyPipBoy::drawSteps(){
    // reset step counter at midnight
    if (currentTime.Hour == 0 && currentTime.Minute == 0){
      sensor.resetStepCounter();
    }

    //draw progress bar
    uint32_t stepCount = sensor.getCounter();
    uint8_t progress = (uint8_t)(stepCount * 100.0 / STEPSGOAL);
    progress = progress > 100 ? 100 : progress;
    display.drawBitmap(60, 155, gauge, 73, 10, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.fillRect(60+13, 155+5, (progress/2)+5, 4, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);

    //show step count
    display.setFont(&monofonto8pt7b);
    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.setCursor(150, 160);
    display.print("STEPS");
    display.setCursor(150, 175);
    display.print(stepCount);
}

*/

void CityWeather::changeSkyDithering(float d)
{
  ditheringValue += d;
  ditheringValue = constrain (ditheringValue, -1.0, 1.0);
  Serial.println(ditheringValue);
  RTC.read(currentTime);
  showWatchFace(true);
}

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

  // display.drawBitmap(64, 2, wifi, 19, 16, 0);
  // display.drawBitmap(88, 2, geolocation, 13, 16, 0);
  // display.drawBitmap(105, 2, ntp, 13, 16, 0);
  // display.drawBitmap(122, 1, weather, 15, 16, 0);

  // battery
  display.drawBitmap(143, 1, battery, 9, 15, GxEPD_BLACK);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeMonoBold9pt7b);
  float voltage = getBatteryVoltage();
  int batteryPercent = constrain((voltage - 3.3) * 111.11, 0, 100);
  String batteryStr = String(batteryPercent) + "%";
  drawTextRightAligned(display, 198, 14, batteryStr);
}

void CityWeather::drawCity()
{
  // sky
  drawStuckiDitherRect(display, 0, 19, 200, 88, ditheringValue);

  // city
  int y = 40;
  display.drawBitmap(0, y - 1, city, 200, 65, 1);
  display.drawBitmap(-1, y, city, 200, 65, 1);
  display.drawBitmap(1, y, city, 200, 65, 1);
  display.drawBitmap(0, y, city, 200, 65, 0);
}

void CityWeather::printTemperature(Adafruit_GFX &d, const String &text, int16_t centerX, int16_t y)
{
  if (text.length() > 2) {
    d.setFont(&FreeMonoBold7pt7b);
    y--;
  } else  {
    d.setFont(&FreeMonoBold9pt7b);
    centerX--;
  }

  printCentered (display, text, centerX, y);
}

void CityWeather::drawCalendar()
{
  DailyForecast currentWeek[7];
  cityWeatherService.getCurrentWeekForecast(currentWeek);

  // int x = 1;
  // int y = 168;
  // float xScale = 33;
  // float yScale = 2.0;

  // std::vector<int> valuesDay = {4, 6, 10, 5, 14, 14, 10};
  // std::vector<int> valuesNight = {-3, -8, -3, 2, 3, 2, 2};
  // drawTwoSplines(display, valuesDay, valuesNight, x, y, xScale, yScale);

  for (int i = 0; i < 7; i++)
  {
    // fill current day
    tmElements_t tmNow;
    Watchy::RTC.read(tmNow);
    uint32_t today = uint32_t(tmNow.Year + 1970) * 10000 + uint32_t(tmNow.Month) * 100 + uint32_t(tmNow.Day);
    if (currentWeek[i].date == today)
    {
      display.drawFastVLine (i*28 + 1, 105, 200-105, GxEPD_BLACK);
      display.drawFastVLine (i*28 + 29, 105, 200-105, GxEPD_BLACK);
      fillRect(display, 1 + i*28, 105, 28, 200 - 105, GxEPD_BLACK, 2);
    }

    // lines between days
    if (i > 0)
    {
      drawLine(display, 1 + i*28, 95, 1 + i*28, 200);
    }
    
    display.setTextColor(GxEPD_BLACK);

    // weekday
    display.setFont();
    printCentered(display, currentWeek[i].weekDay, (i * 28) + 16, 108);

    // day
    display.setFont(&FreeMonoBold9pt7b);
    int day = currentWeek[i].date % 100;
    printCentered(display, (String)day, (i * 28) + 13, 129);
    
    // weather
    const unsigned char* weatherIcon = cityWeatherService.weatherNameFromCode(currentWeek[i].weatherCode);
    display.drawBitmap(i*28 + 3, 136, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, GxEPD_BLACK, GxEPD_WHITE);
    drawLine(display, i*28 + 2, 135, i*28 + 29, 135, GxEPD_WHITE, 1); // top border
    drawLine(display, i*28 + 2, 161, i*28 + 29, 161, GxEPD_WHITE, 1); // bottom border

    // tMax & tMin
    printTemperature (display, (String)currentWeek[i].tempMax, (i*28) + 14, 177);
    printTemperature (display, (String)currentWeek[i].tempMin, (i*28) + 14, 195);
  }

  drawLine(display, 1, 133, 199, 133, GxEPD_BLACK, 4); // day bottom
}

void CityWeather::drawWatchFace()
{
  display.fillScreen(GxEPD_WHITE);

  cityWeatherService.updateWifiData();

  drawStatusBar();
  drawCity();
  drawCalendar();
};