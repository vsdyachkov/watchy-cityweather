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
    long date;
    int temp_max;
    int temp_min;
    int weather_code;
};


class CityWeatherService
{

public:
    explicit CityWeatherService(CityWeather &cw);

    bool updateWifiData();
    void getCurrentWeekForecast();
    void printWeekTable();

private:
    CityWeather &cityWeather;

    bool getLocationData();
    String weatherNameFromCode(int code);
    bool getWeatherData();
    void sortForecasts(DailyForecast *forecastArray, size_t size);
};

