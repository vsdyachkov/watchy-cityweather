#include "CityWeatherService.h"
#include "CityWeather.h"
#include "Images.h"
#include <Preferences.h>
#include <stddef.h>
#include <string.h>

#define IP_WHO_URL "http://ipwho.is/?fields=city,country,latitude,longitude,timezone.offset"
#define OPEN_METEO_URL "https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}&daily=temperature_2m_max,temperature_2m_min,weather_code&past_days=7&forecast_days=16&timezone=auto"
#define OPEN_METEO_UPDATE_INTERVAL 60
#define WEATHER_UPDATE_INTERVAL_SECONDS (OPEN_METEO_UPDATE_INTERVAL * 60)
#define NETWORK_RETRY_INTERVAL_SECONDS (60 * 60)

#define NUM_DAYS 23

namespace
{
constexpr uint32_t WEATHER_CACHE_STORAGE_MAGIC = 0x43575758;
constexpr uint16_t WEATHER_CACHE_STORAGE_VERSION = 1;
constexpr const char *WEATHER_CACHE_STORAGE_NAMESPACE = "cw-weather";
constexpr const char *WEATHER_CACHE_STORAGE_KEY = "cache";

struct StoredDailyForecast
{
    uint32_t date;
    int16_t tempMax;
    int16_t tempMin;
    int16_t weatherCode;
};

struct WeatherCacheSnapshot
{
    uint32_t magic;
    uint16_t version;
    uint8_t numDays;
    bool forecastReady;
    LocationData location;
    time_t savedTime;
    StoredDailyForecast forecast[NUM_DAYS];
    uint32_t checksum;
};
}

RTC_DATA_ATTR LocationData locationData;
RTC_DATA_ATTR const char *wdayNames[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

RTC_DATA_ATTR DailyForecast forecast[NUM_DAYS];

RTC_DATA_ATTR time_t savedTime = 0;
RTC_DATA_ATTR time_t nextWeatherRetryTime = 0;
RTC_DATA_ATTR bool forecastReady = false;
RTC_DATA_ATTR uint32_t weatherCacheLoadedMagic = 0;

CityWeatherService::CityWeatherService(CityWeather &cw) : cityWeather(cw) {}

static void copyStringToBuffer(char *target, size_t targetSize, const String &value)
{
    if (targetSize == 0) {
        return;
    }
    strncpy(target, value.c_str(), targetSize - 1);
    target[targetSize - 1] = '\0';
}

static void copyCStringToBuffer(char *target, size_t targetSize, const char *value)
{
    if (targetSize == 0) {
        return;
    }
    if (value == nullptr) {
        target[0] = '\0';
        return;
    }
    strncpy(target, value, targetSize - 1);
    target[targetSize - 1] = '\0';
}

static uint32_t checksumBytes(const uint8_t *bytes, size_t length)
{
    uint32_t checksum = 2166136261UL;
    for (size_t i = 0; i < length; i++)
    {
        checksum ^= bytes[i];
        checksum *= 16777619UL;
    }
    return checksum;
}

static uint32_t weatherCacheSnapshotChecksum(const WeatherCacheSnapshot &snapshot)
{
    return checksumBytes(
        reinterpret_cast<const uint8_t *>(&snapshot),
        offsetof(WeatherCacheSnapshot, checksum)
    );
}

static bool hasLocationData()
{
    return locationData.lat[0] != '\0' &&
        locationData.lon[0] != '\0' &&
        locationData.offset[0] != '\0';
}

static bool forecastArrayLooksValid()
{
    if (!forecastReady)
    {
        return true;
    }

    bool hasForecast = false;
    uint32_t previousDate = 0;
    for (int i = 0; i < NUM_DAYS; i++)
    {
        if (forecast[i].date <= 20200101)
        {
            continue;
        }
        if (forecast[i].tempMax < -100 || forecast[i].tempMax > 100)
        {
            return false;
        }
        if (forecast[i].tempMin < -100 || forecast[i].tempMin > 100)
        {
            return false;
        }
        if (previousDate != 0 && static_cast<uint32_t>(forecast[i].date) < previousDate)
        {
            return false;
        }
        previousDate = static_cast<uint32_t>(forecast[i].date);
        hasForecast = true;
    }

    return hasForecast;
}

static bool forecastEntryLooksUsable(const DailyForecast &dailyForecast)
{
    if (dailyForecast.date <= 20200101)
    {
        return false;
    }
    if (dailyForecast.tempMax < -100 || dailyForecast.tempMax > 100)
    {
        return false;
    }
    if (dailyForecast.tempMin < -100 || dailyForecast.tempMin > 100)
    {
        return false;
    }
    if (dailyForecast.weatherCode < 0 || dailyForecast.weatherCode > 99)
    {
        return false;
    }
    return true;
}

static bool forecastContainsDate(uint32_t targetDate)
{
    if (!forecastReady)
    {
        return false;
    }

    for (int i = 0; i < NUM_DAYS; i++)
    {
        if (
            static_cast<uint32_t>(forecast[i].date) == targetDate &&
            forecastEntryLooksUsable(forecast[i])
        )
        {
            return true;
        }
    }
    return false;
}

static bool weatherCacheInMemoryLooksValid()
{
    locationData.city[sizeof(locationData.city) - 1] = '\0';
    locationData.lat[sizeof(locationData.lat) - 1] = '\0';
    locationData.lon[sizeof(locationData.lon) - 1] = '\0';
    locationData.offset[sizeof(locationData.offset) - 1] = '\0';
    return (!forecastReady || hasLocationData()) && forecastArrayLooksValid();
}

static void saveWeatherCacheToStorage()
{
    WeatherCacheSnapshot snapshot = {};
    snapshot.magic = WEATHER_CACHE_STORAGE_MAGIC;
    snapshot.version = WEATHER_CACHE_STORAGE_VERSION;
    snapshot.numDays = NUM_DAYS;
    snapshot.forecastReady = forecastReady;
    snapshot.location = locationData;
    snapshot.savedTime = savedTime;
    for (int i = 0; i < NUM_DAYS; i++)
    {
        snapshot.forecast[i].date = static_cast<uint32_t>(forecast[i].date);
        snapshot.forecast[i].tempMax = static_cast<int16_t>(forecast[i].tempMax);
        snapshot.forecast[i].tempMin = static_cast<int16_t>(forecast[i].tempMin);
        snapshot.forecast[i].weatherCode = static_cast<int16_t>(forecast[i].weatherCode);
    }
    snapshot.checksum = weatherCacheSnapshotChecksum(snapshot);

    Preferences preferences;
    if (!preferences.begin(WEATHER_CACHE_STORAGE_NAMESPACE, false))
    {
        return;
    }
    preferences.putBytes(WEATHER_CACHE_STORAGE_KEY, &snapshot, sizeof(snapshot));
    preferences.end();
}

static bool loadWeatherCacheFromStorage()
{
    Preferences preferences;
    if (!preferences.begin(WEATHER_CACHE_STORAGE_NAMESPACE, true))
    {
        return false;
    }

    size_t storedLength = preferences.getBytesLength(WEATHER_CACHE_STORAGE_KEY);
    if (storedLength != sizeof(WeatherCacheSnapshot))
    {
        preferences.end();
        return false;
    }

    WeatherCacheSnapshot snapshot = {};
    size_t readLength = preferences.getBytes(WEATHER_CACHE_STORAGE_KEY, &snapshot, sizeof(snapshot));
    preferences.end();

    if (readLength != sizeof(snapshot))
    {
        return false;
    }
    if (
        snapshot.magic != WEATHER_CACHE_STORAGE_MAGIC ||
        snapshot.version != WEATHER_CACHE_STORAGE_VERSION ||
        snapshot.numDays != NUM_DAYS ||
        snapshot.checksum != weatherCacheSnapshotChecksum(snapshot)
    )
    {
        return false;
    }

    locationData = snapshot.location;
    locationData.city[sizeof(locationData.city) - 1] = '\0';
    locationData.lat[sizeof(locationData.lat) - 1] = '\0';
    locationData.lon[sizeof(locationData.lon) - 1] = '\0';
    locationData.offset[sizeof(locationData.offset) - 1] = '\0';
    savedTime = snapshot.savedTime;
    nextWeatherRetryTime = 0;
    forecastReady = snapshot.forecastReady;
    for (int i = 0; i < NUM_DAYS; i++)
    {
        forecast[i].weekDay = nullptr;
        forecast[i].date = snapshot.forecast[i].date;
        forecast[i].tempMax = snapshot.forecast[i].tempMax;
        forecast[i].tempMin = snapshot.forecast[i].tempMin;
        forecast[i].weatherCode = snapshot.forecast[i].weatherCode;
    }

    return weatherCacheInMemoryLooksValid();
}

static void clearWeatherCacheStorage()
{
    Preferences preferences;
    if (!preferences.begin(WEATHER_CACHE_STORAGE_NAMESPACE, false))
    {
        return;
    }
    preferences.remove(WEATHER_CACHE_STORAGE_KEY);
    preferences.end();
}

static void ensureWeatherCacheLoaded()
{
    if (
        weatherCacheLoadedMagic == WEATHER_CACHE_STORAGE_MAGIC &&
        weatherCacheInMemoryLooksValid()
    )
    {
        return;
    }

    if (loadWeatherCacheFromStorage())
    {
        weatherCacheLoadedMagic = WEATHER_CACHE_STORAGE_MAGIC;
        return;
    }

    if (weatherCacheInMemoryLooksValid() && (forecastReady || hasLocationData()))
    {
        weatherCacheLoadedMagic = WEATHER_CACHE_STORAGE_MAGIC;
        saveWeatherCacheToStorage();
        return;
    }

    weatherCacheLoadedMagic = WEATHER_CACHE_STORAGE_MAGIC;
}

void resetCityWeatherNetworkCache()
{
    locationData.city[0] = '\0';
    locationData.lat[0] = '\0';
    locationData.lon[0] = '\0';
    locationData.offset[0] = '\0';
    savedTime = 0;
    nextWeatherRetryTime = 0;
    forecastReady = false;
    weatherCacheLoadedMagic = WEATHER_CACHE_STORAGE_MAGIC;
    clearWeatherCacheStorage();
}

template<typename F>
bool retry(F f, int maxAttempts) {
  for(int attempt = 1; attempt <= maxAttempts; ++attempt) {
    if (f()) {
      return true;
    }
  }
  return false;
}

bool CityWeatherService::getLocationData()
{
    ensureWeatherCacheLoaded();
    HTTPClient http;
    http.setConnectTimeout(20000);
    http.begin(IP_WHO_URL);
    int httpCode = http.GET();
    bool success = false;

    if (httpCode == 200)
    {
        String payload = http.getString();
        JSONVar responseObject = JSON.parse(payload);
        if (JSON.typeof(responseObject) == "undefined")
        {
            Serial.println("Failed to parse location JSON");
        }
        else
        {
            auto cityVar = responseObject["city"];
            if (JSON.typeof(cityVar) == "string") {
                copyCStringToBuffer(
                    locationData.city,
                    sizeof(locationData.city),
                    (const char*)cityVar
                );
            }

            copyStringToBuffer(
                locationData.lat,
                sizeof(locationData.lat),
                String((double)responseObject["latitude"], 6)
            );
            copyStringToBuffer(
                locationData.lon,
                sizeof(locationData.lon),
                String((double)responseObject["longitude"], 6)
            );
            copyStringToBuffer(
                locationData.offset,
                sizeof(locationData.offset),
                String((int)responseObject["timezone"]["offset"])
            );
            Serial.println("OK");
            saveWeatherCacheToStorage();
            success = true;
        }
    }
    else
    {
        Serial.println("Error on location HTTP request");
    }

    http.end();
    return success;
}

bool CityWeatherService::getWeatherData()
{
    ensureWeatherCacheLoaded();
    HTTPClient http;
    http.setConnectTimeout(20000);
    String weatherQueryURL = OPEN_METEO_URL;
    if (locationData.lat[0] != '\0' && locationData.lon[0] != '\0')
    {
        weatherQueryURL.replace("{lat}", locationData.lat);
        weatherQueryURL.replace("{lon}", locationData.lon);
    }
    else
    {
        Serial.println("Error: locationData in null");
        return false;
    }

    http.begin(weatherQueryURL);
    int httpCode = http.GET();
    bool success = false;

    if (httpCode == 200)
    {
        String payload = http.getString();
        JSONVar responseObject = JSON.parse(payload);
        if (JSON.typeof(responseObject) == "undefined")
        {
            Serial.println("Failed to parse weather JSON");
        }
        else
        {
            JSONVar times = responseObject["daily"]["time"];
            JSONVar temps_max = responseObject["daily"]["temperature_2m_max"];
            JSONVar temps_min = responseObject["daily"]["temperature_2m_min"];
            JSONVar codes = responseObject["daily"]["weather_code"];

            for (int i = 0; i < NUM_DAYS; i++)
            {
                if (
                    JSON.typeof(times[i]) != "string" ||
                    JSON.typeof(temps_max[i]) == "undefined" ||
                    JSON.typeof(temps_min[i]) == "undefined" ||
                    JSON.typeof(codes[i]) == "undefined"
                )
                {
                    Serial.println("Incomplete weather JSON");
                    http.end();
                    return false;
                }

                const char *dateStr = (const char *)times[i];
                if (dateStr == nullptr || strlen(dateStr) < 10)
                {
                    Serial.println("Invalid weather date");
                    http.end();
                    return false;
                }
                int year = atoi(String(dateStr).substring(0, 4).c_str());
                int month = atoi(String(dateStr).substring(5, 7).c_str());
                int day = atoi(String(dateStr).substring(8, 10).c_str());
                uint32_t dateNum = year * 10000 + month * 100 + day;

                forecast[i].date = dateNum;
                forecast[i].tempMax = static_cast<int>(round((double)temps_max[i]));
                forecast[i].tempMin = static_cast<int>(round((double)temps_min[i]));
                forecast[i].weatherCode = (int)codes[i];
            }
            sortForecasts(forecast, NUM_DAYS);
            forecastReady = true;
            if (forecastArrayLooksValid())
            {
                Serial.println("OK");
                success = true;
            }
            else
            {
                forecastReady = false;
                Serial.println("Invalid weather data");
            }
        }
    }
    else
    {
        Serial.println("Error on weather HTTP request");
    }

    http.end();
    return success;
}

bool CityWeatherService::updateWifiData()
{
    ensureWeatherCacheLoaded();
    tmElements_t tm;
    Watchy::RTC.read(tm);
    time_t now = makeTime(tm);
    long diff = now - savedTime;

    if (nextWeatherRetryTime != 0 && now < nextWeatherRetryTime) {
        Serial.println("Skip update until next retry window");
        return false;
    }

    if (hasCurrentWeekForecastData() && savedTime != 0 && diff <= WEATHER_UPDATE_INTERVAL_SECONDS) {
        Serial.println("Don't need update");
        return false;
    }

    Serial.println("Update data...");

    bool success = false;
    bool locationReady = hasLocationData();

    if (cityWeather.connectWiFi())
    {
        Serial.println("#1. Wifi connected");

        if (!locationReady)
        {
            Serial.print("#2. getLocationData... ");
            locationReady = retry([&]() { return getLocationData(); }, 3);
        }
        else
        {
            Serial.println("#2. use cached location data");
        }

        if (locationReady)
        {
            Serial.print("#3. syncNTP GMT: ");
            Serial.print(locationData.offset);
            Serial.print("... ");
            if (retry([&]() { return cityWeather.syncNTP(atol(locationData.offset)); }, 3))
            {
                Serial.println("OK");
                Serial.print("#4. getWeatherData...");
                success = retry([&]() { return getWeatherData(); }, 3);
                if (success)
                {
                    Watchy::RTC.read(tm);
                    savedTime = makeTime(tm);
                    nextWeatherRetryTime = 0;
                    saveWeatherCacheToStorage();
                }
            }
            else
            {
                Serial.println("failed");
            }
        }
    }
    else
    {
        Serial.println("#1. Wifi not connected");
    }

    WiFi.mode(WIFI_OFF);

    if (!success)
    {
        nextWeatherRetryTime = now + NETWORK_RETRY_INTERVAL_SECONDS;
    }

    return success;
}

bool CityWeatherService::hasForecastData() const
{
    ensureWeatherCacheLoaded();
    if (!forecastReady) {
        return false;
    }

    for (int i = 0; i < NUM_DAYS; i++)
    {
        if (forecastEntryLooksUsable(forecast[i]))
        {
            return true;
        }
    }

    return false;
}

bool CityWeatherService::hasCurrentWeekForecastData() const
{
    ensureWeatherCacheLoaded();
    if (!forecastReady)
    {
        return false;
    }

    tmElements_t tmNow;
    Watchy::RTC.read(tmNow);
    time_t nowT = makeTime(tmNow);
    if (nowT <= 0)
    {
        return false;
    }

    int daysBack = (tmNow.Wday + 5) % 7;
    time_t mondayT = nowT - daysBack * SECS_PER_DAY;
    for (int weekIndex = 0; weekIndex < 7; weekIndex++)
    {
        time_t dayT = mondayT + weekIndex * SECS_PER_DAY;
        tmElements_t tmDay;
        breakTime(dayT, tmDay);
        uint32_t targetDate =
            (tmDay.Year + 1970) * 10000 + tmDay.Month * 100 + tmDay.Day;
        if (!forecastContainsDate(targetDate))
        {
            return false;
        }
    }
    return true;
}

void CityWeatherService::sortForecasts(DailyForecast *forecastArray, size_t size)
{
    for (size_t i = 0; i < size - 1; ++i)
    {
        for (size_t j = i + 1; j < size; ++j)
        {
            if (forecastArray[i].date > forecastArray[j].date)
            {
                DailyForecast temp = forecastArray[i];
                forecastArray[i] = forecastArray[j];
                forecastArray[j] = temp;
            }
        }
    }
}

void CityWeatherService::getCurrentWeekForecast(DailyForecast currentWeek[7])
{
    ensureWeatherCacheLoaded();
    tmElements_t tmNow;
    Watchy::RTC.read(tmNow);
    time_t nowT = makeTime(tmNow);
    int wday = tmNow.Wday;
    int daysBack = (wday + 5) % 7; // пн→0, вт→1,… вс→6
    time_t mondayT = nowT - daysBack * SECS_PER_DAY;

    for (int weekIndex = 0; weekIndex < 7; weekIndex++)
    {
        time_t dayT = mondayT + weekIndex * SECS_PER_DAY;
        tmElements_t tmDay;
        breakTime(dayT, tmDay);
        uint32_t targetDate = (tmDay.Year + 1970) * 10000 + tmDay.Month * 100 + tmDay.Day;

        bool found = false;
        for (int forecastIndex = 0; forecastIndex < NUM_DAYS; forecastIndex++)
        {
            if (forecast[forecastIndex].date == targetDate)
            {
                currentWeek[weekIndex] = forecast[forecastIndex];
                found = true;
                break;
            }
        }

        if (!found)
        {
            currentWeek[weekIndex].date = targetDate;
            currentWeek[weekIndex].tempMax = 0;
            currentWeek[weekIndex].tempMin = 0;
            currentWeek[weekIndex].weatherCode = 0;
        }
        
        
        currentWeek[weekIndex].weekDay = wdayNames[weekIndex];
    }
}

const unsigned char* CityWeatherService::weatherNameFromCode(int code)
{
    switch (code)
    {
    case 0:
        return sun;
    case 1:
    case 2:
    case 3:
        return cloudSun;
    case 45:
    case 8:
        return fog;
    case 51:
    case 53:
    case 55:
    case 61:
    case 63:
    case 65:
    case 80:
    case 81:
    case 82:
        return rain;
    case 66:
    case 67:
        return freezingRain;
    case 56:
    case 57:
    case 71:
    case 73:
    case 75:
    case 77:
    case 85:
    case 86:
        return snow;
    case 95:
    case 96:
    case 99:
        return thunderstorm;
    default:
        return blank;
    }
}
