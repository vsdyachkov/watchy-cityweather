#include <vector>
#include "Adafruit_GFX_ext.h"
#include "Images.h"
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

  drawOutlinedText(display, -1, 17, timeStr, &FreeSansBold12pt7b, GxEPD_YELLOW);
}

void CityWeather::drawBattery()
{
  display.drawBitmap(175, 1, battery, 24, 16, 1);
  display.setFont();
  display.setTextColor(GxEPD_WHITE);
  display.setCursor(177, 5);
  float voltage = getBatteryVoltage();
  int batteryPercent = constrain((voltage - 3.3) * 111.11, 0, 100);
  String batteryStr = String(batteryPercent);
  if (batteryPercent < 100)
    batteryStr = batteryStr + "%";
  display.print(batteryStr);
}

void CityWeather::drawSky()
{
  drawStuckiDitherRect(display, 0, 0, 200, 94, ditheringValue);
}

void CityWeather::drawCity()
{
  display.drawBitmap(0, 28, city, 200, 65, 1);
  display.drawBitmap(-1, 29, city, 200, 65, 1);
  display.drawBitmap(1, 29, city, 200, 65, 1);
  display.drawBitmap(0, 29, city, 200, 65, 0);
}

void CityWeather::drawWatchFace()
{
  display.fillScreen(GxEPD_WHITE);

  drawSky();
  drawCity();

  drawTime();
  drawBattery();

  display.setTextColor(0);
  display.setTextWrap(false);
  display.setFont(&FreeSansBold9pt7b);


  display.drawLine(1, 130, 198, 130, 0);

  drawLine (display, 29, 95, 29, 212, 0);
  
  drawLine (display, 57, 95, 57, 212, 0);
  drawLine (display, 85, 95, 85, 212, 0);
  drawLine (display, 85, 95, 85, 212, 0);
  drawLine (display, 141, 95, 141, 212, 0);
  drawLine (display, 169, 95, 169, 212, 0);

  display.fillRect (1, 94, 200, 13, GxEPD_BLACK);

  // drawStuckiDitherRectConst (display, 1, 95, 199, 108, 0.5);


  display.drawLine(0, 95, 0, 211, 0);
  display.drawLine(199, 95, 199, 212, 0);
  display.drawLine(198, 199, 1, 199, 0);

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
     display.setFont();
     display.setTextColor(GxEPD_WHITE);

    // weekday
    display.setCursor(11+i*28, 98);
    display.print(currentWeek[i].weekDay);

    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);

    // day
    display.setCursor(4+i*28, 123);
    int day = currentWeek[i].date % 100;
    display.print(day);

    // weather
    display.drawBitmap(8+i*28, 135, image_download__copy__bits, 15, 16, 0);

    // t max
    display.setCursor(7+i*28, 170);
    display.print(currentWeek[i].tempMax);

    // t min
    display.setCursor(10+i*28, 194);
    display.print(currentWeek[i].tempMin);
  }

};

//     void CityWeatherService::printWeekTable()
// {
//     // 1) Заголовок: дни недели
//     for (int i = 0; i < 7; i++)
//     {
//         Serial.print(wdayNames[i]);
//         if (i < 6)
//             Serial.print(" ");
//     }
//     Serial.println();

//     // 2) Число месяца (из поля date = YYYYMMDD)
//     for (int i = 0; i < 7; i++) {
//         int day = currentWeek[i].date % 100;  // отбрасываем YYYYMM
//         Serial.print(day);
//         if (i < 6) Serial.print(" ");
//     }
//     Serial.println();

//     // 3) Коды погоды
//     for (int i = 0; i < 7; i++)
//     {
//         Serial.print(weatherNameFromCode(currentWeek[i].weather_code));
//         if (i < 6)
//             Serial.print(" ");
//     }
//     Serial.println();

//     // 4) Максимумы
//     for (int i = 0; i < 7; i++)
//     {
//         Serial.print(currentWeek[i].temp_max);
//         if (i < 6)
//             Serial.print(" ");
//     }
//     Serial.println();

//     // 5) Минимумы
//     for (int i = 0; i < 7; i++)
//     {
//         Serial.print(currentWeek[i].temp_min);
//         if (i < 6)
//             Serial.print(" ");
//     }
//     Serial.println();
// };
