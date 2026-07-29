#pragma once
#include <Watchy.h>
class CityWeather;

inline uint32_t tmToYYYYMMDD(const tmElements_t &tm) {
    return (tm.Year + 1970) * 10000UL + tm.Month * 100UL + tm.Day;
}

inline uint32_t ymdToYYYYMMDD(int year, int month, int day) {
    return year * 10000UL + month * 100UL + day;
}

struct LocationData
{
    char city[20];
    char lat[16];
    char lon[16];
    char offset[12];
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

void resetCityWeatherNetworkCache();

class CityWeatherService
{

public:
    explicit CityWeatherService(CityWeather &cw);

    bool updateWifiData();
    bool hasForecastData() const;
    bool hasCurrentWeekForecastData() const;
    void getCurrentWeekForecast(DailyForecast weekDay[7]);
    const unsigned char* weatherNameFromCode(int code);

private:
    CityWeather &cityWeather;

    bool getLocationData();
    bool getWeatherData();
    void sortForecasts(DailyForecast *forecastArray, size_t size);
};
