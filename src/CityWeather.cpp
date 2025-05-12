#include <vector>
#include "Adafruit_GFX_ext.h"
#include "Images.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include "CityWeather.h"
#include "CityWeatherService.h"

RTC_DATA_ATTR float ditheringValue = 0.0;

/*
#define STEPSGOAL 5000

const uint8_t WEATHER_ICON_WIDTH = 48;
const uint8_t WEATHER_ICON_HEIGHT = 32;

RTC_DATA_ATTR uint8_t vaultBoyNum;

void WatchyPipBoy::drawWatchFace(){
    //top menu bar
    display.fillScreen(DARKMODE ? GxEPD_BLACK : GxEPD_WHITE);
    display.setFont(&monofonto8pt7b);
    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.setCursor(22, 14);
    display.print("STAT  INV  DATA  MAP");
    display.drawBitmap(0, 10, menubar, 200, 9, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);

    //bottom text
    display.setFont(&monofonto8pt7b);
    display.setCursor(10, 195);
    display.println("PIP-BOY 3000 ROBCO IND.");

    drawTime();
    drawDate();
    drawSteps();
    drawWeather();
    drawBattery();
    // display.drawBitmap(120, 77, WIFI_CONFIGURED ? wifi : wifioff, 26, 18, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    // if(BLE_CONFIGURED){
    //     display.drawBitmap(100, 75, bluetooth, 13, 21, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    // }
}

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

void WatchyPipBoy::drawWeather(){

    weatherData currentWeather = getWeatherData();

    int8_t temperature = currentWeather.temperature;
    int16_t weatherConditionCode = currentWeather.weatherConditionCode;

    display.setFont(&monofonto10pt7b);
    display.setTextColor(DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
    display.setCursor(12, 133);


    display.print(temperature);
    display.print(currentWeather.isMetric ? "C" : "F");
    const unsigned char* weatherIcon;

    //https://openweathermap.org/weather-conditions
    if(weatherConditionCode > 801){//Cloudy
    weatherIcon = cloudy;
    }else if(weatherConditionCode == 801){//Few Clouds
    weatherIcon = cloudsun;
    }else if(weatherConditionCode == 800){//Clear
    weatherIcon = sunny;
    }else if(weatherConditionCode >=700){//Atmosphere
    weatherIcon = atmosphere;
    }else if(weatherConditionCode >=600){//Snow
    weatherIcon = snow;
    }else if(weatherConditionCode >=500){//Rain
    weatherIcon = rain;
    }else if(weatherConditionCode >=300){//Drizzle
    weatherIcon = drizzle;
    }else if(weatherConditionCode >=200){//Thunderstorm
    weatherIcon = thunderstorm;
    }else
    return;
    display.drawBitmap(5, 85, weatherIcon, WEATHER_ICON_WIDTH, WEATHER_ICON_HEIGHT, DARKMODE ? GxEPD_WHITE : GxEPD_BLACK);
}
    */

static const unsigned char PROGMEM image_download_bits[] = {0x00, 0x00, 0x00, 0x07, 0xc0, 0x00, 0x08, 0x20, 0x00, 0x10, 0x10, 0x00, 0x30, 0x08, 0x00, 0x40, 0x0e, 0x00, 0x80, 0x01, 0x00, 0x80, 0x00, 0x80, 0x40, 0x00, 0x80, 0x3f, 0xff, 0x00, 0x01, 0x10, 0x00, 0x22, 0x22, 0x00, 0x44, 0x84, 0x00, 0x91, 0x28, 0x00, 0x22, 0x40, 0x00, 0x00, 0x80, 0x00};

static const unsigned char PROGMEM image_download_1_bits[] = {0x01, 0xf0, 0x00, 0x06, 0x0c, 0x00, 0x18, 0x03, 0x00, 0x21, 0xf0, 0x80, 0x46, 0x0c, 0x40, 0x88, 0x02, 0x20, 0x10, 0xe1, 0x00, 0x23, 0x18, 0x80, 0x04, 0x04, 0x00, 0x08, 0x42, 0x00, 0x01, 0xb0, 0x00, 0x02, 0x08, 0x00, 0x00, 0x40, 0x00, 0x00, 0xa0, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00};

static const unsigned char PROGMEM image_download__copy__bits[] = {0x01, 0x00, 0x21, 0x08, 0x10, 0x10, 0x03, 0x80, 0x8c, 0x62, 0x48, 0x24, 0x10, 0x10, 0x10, 0x10, 0x10, 0x10, 0x48, 0x24, 0x8c, 0x62, 0x03, 0x80, 0x10, 0x10, 0x21, 0x08, 0x01, 0x00, 0x00, 0x00};

static const unsigned char PROGMEM image_download__copy__1_bits[] = {0x00, 0x20, 0x00, 0x02, 0x02, 0x00, 0x00, 0x70, 0x00, 0x01, 0x8c, 0x00, 0x09, 0x04, 0x80, 0x02, 0x02, 0x00, 0x02, 0x02, 0x00, 0x07, 0x82, 0x00, 0x08, 0x44, 0x80, 0x10, 0x2c, 0x00, 0x30, 0x30, 0x00, 0x60, 0x1e, 0x00, 0x80, 0x03, 0x00, 0x80, 0x01, 0x00, 0x80, 0x01, 0x00, 0x7f, 0xfe, 0x00};

void CityWeather::handleButtonPress()
{
  if (guiState == WATCHFACE_STATE)
  {
    // Up and Down switch watch faces
    uint64_t wakeupBit = esp_sleep_get_ext1_wakeup_status();
    if (wakeupBit & UP_BTN_MASK)
    {
      ditheringValue += 0.1;
      if (ditheringValue > 1.0)
      {
        ditheringValue = 1.0;
      }
      RTC.read(currentTime);
      showWatchFace(true);
      return;
    }
    if (wakeupBit & DOWN_BTN_MASK)
    {
      ditheringValue -= 0.1;
      if (ditheringValue < -1.0)
      {
        ditheringValue = -1.0;
      }
      RTC.read(currentTime);
      showWatchFace(true);
      return;
    }
    if (wakeupBit & BACK_BTN_MASK)
    {
      // light = !light;
      RTC.read(currentTime);
      showWatchFace(true);
      return;
    }
    if (wakeupBit & MENU_BTN_MASK)
    {
      Watchy::handleButtonPress();
      return;
    }
  }
  else
  {
    Watchy::handleButtonPress();
  }
  return;
}

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

void CityWeather::drawBattery()
{
  display.drawBitmap(143, 1, battery, 9, 15, GxEPD_BLACK);
  display.setTextColor(GxEPD_BLACK);
  display.setFont(&FreeSansBold9pt7b);
  float voltage = getBatteryVoltage();
  int batteryPercent = constrain((voltage - 3.3) * 111.11, 0, 100);
  String batteryStr = String(batteryPercent) + "%";
  drawTextRightAligned (display, 198, 14, batteryStr);
}

void CityWeather::drawSky()
{
  drawStuckiDitherRect(display, 0, 19, 200, 88, ditheringValue);
}

void CityWeather::drawCity()
{
  int y = 40;
  display.drawBitmap(0, y-1, city, 200, 65, 1);
  display.drawBitmap(-1, y, city, 200, 65, 1);
  display.drawBitmap(1, y, city, 200, 65, 1);
  display.drawBitmap(0, y, city, 200, 65, 0);
}

void CityWeather::drawWatchFace()
{
  display.fillScreen(GxEPD_WHITE);

  drawSky();
  drawCity();

  drawTime();
  drawBattery();

  // int x = 1;
  // int y = 172;
  // float xScale = 33;
  // float yScale = 1.2;

  // std::vector<int> valuesDay = {4, 6, 10, 5, 14, 14, 10};
  // std::vector<int> valuesNight = {-3, -8, -3, 2, 3, 2, 2};
  // drawTwoSplines(display, valuesDay, valuesNight, x, y, xScale, yScale);

  CityWeatherService cityWeatherService(*this);
  cityWeatherService.updateWifiData();

  DailyForecast currentWeek[7];
  cityWeatherService.getCurrentWeekForecast(currentWeek);
  
  for (int i = 0; i < 7; i++)
  {
    // fill current day
    tmElements_t tmNow;
    Watchy::RTC.read(tmNow);
    uint32_t today = uint32_t(tmNow.Year + 1970) * 10000 + uint32_t(tmNow.Month) * 100 + uint32_t(tmNow.Day);
    if (currentWeek[i].date == today) {
      fillRect(display, 1+i*28, 105, 28, 200-105);
    }

    if (i < 7) {
      drawLine (display, 1+i*28, 95, 1+i*28, 200);
    }
    
    // weekday
    display.setFont();
    display.setTextColor(GxEPD_BLACK);
    printCentered (display, currentWeek[i].weekDay, (i * 28) + 17, 108);

    // day
    display.setTextColor(GxEPD_BLACK);
    display.setFont(&FreeSansBold9pt7b);
    int day = currentWeek[i].date % 100;
    printCentered (display, (String)day, (i * 28) + 14, 132);

    // weather
    display.drawBitmap(8+i*28, 140, image_download__copy__bits, 15, 16, GxEPD_BLACK);

    // tMax & tMin
    display.setFont(&FreeSans9pt7b);
    printCentered (display, (String)currentWeek[i].tempMax, (i * 28) + 14, 175);
    printCentered (display, (String)currentWeek[i].tempMin, (i * 28) + 14, 195);
  }

  drawLine (display, 0, 135, 200, 135, 0, 4); // day bottom

  display.drawLine(0, 105, 0, 200, GxEPD_BLACK);
  display.drawLine(199, 105, 199, 200, GxEPD_BLACK);
  display.drawLine(1, 199, 200, 199, GxEPD_BLACK);

};