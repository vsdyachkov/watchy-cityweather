#pragma once
#include <Arduino.h>
class CityWeather;

struct LocationData
{
    char city[10];
    String lat;
    String lon;
    String offset;
};

struct DailyForecast
{
    const char * weekDay;
    long date;
    int tempMax;
    int tempMin;
    int weatherCode;
};

extern RTC_DATA_ATTR LocationData locationData;

class CityWeatherService
{

public:
    explicit CityWeatherService(CityWeather &cw);

    bool updateWifiData();
    void getCurrentWeekForecast(DailyForecast weekDay[7]);
    // const char * getCurrentLocationName();
    const unsigned char* weatherNameFromCode(int code);

private:
    CityWeather &cityWeather;

    bool getLocationData();
    bool getWeatherData();
    void sortForecasts(DailyForecast *forecastArray, size_t size);
};

