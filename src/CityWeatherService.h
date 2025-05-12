#pragma once
#include <Arduino.h>
class CityWeather;

struct LocationData
{
    String regionName;
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


class CityWeatherService
{

public:
    explicit CityWeatherService(CityWeather &cw);

    bool updateWifiData();
    void getCurrentWeekForecast(DailyForecast weekDay[7]);

private:
    CityWeather &cityWeather;

    bool getLocationData();
    String weatherNameFromCode(int code);
    bool getWeatherData();
    void sortForecasts(DailyForecast *forecastArray, size_t size);
};

