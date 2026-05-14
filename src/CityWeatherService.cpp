#include "CityWeatherService.h"
#include "CityWeather.h"
#include "Images.h"

#define IP_WHO_URL "http://ipwho.is/?fields=city,country,latitude,longitude,timezone.offset"
#define OPEN_METEO_URL "https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}&daily=temperature_2m_max,temperature_2m_min,weather_code&past_days=7&forecast_days=16&timezone=auto"
#define OPEN_METEO_UPDATE_INTERVAL 60
#define WEATHER_UPDATE_INTERVAL_SECONDS (OPEN_METEO_UPDATE_INTERVAL * 60)
#define NETWORK_RETRY_INTERVAL_SECONDS (60 * 60)

#define NUM_DAYS 23

RTC_DATA_ATTR LocationData locationData;
RTC_DATA_ATTR const char *wdayNames[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

RTC_DATA_ATTR DailyForecast forecast[NUM_DAYS];

RTC_DATA_ATTR time_t savedTime = 0;
RTC_DATA_ATTR time_t nextWeatherRetryTime = 0;
RTC_DATA_ATTR bool forecastReady = false;

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

void resetCityWeatherNetworkCache()
{
    locationData.city[0] = '\0';
    locationData.lat[0] = '\0';
    locationData.lon[0] = '\0';
    locationData.offset[0] = '\0';
    savedTime = 0;
    nextWeatherRetryTime = 0;
    forecastReady = false;
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
                const char *dateStr = (const char *)times[i];
                int year = atoi(String(dateStr).substring(0, 4).c_str());
                int month = atoi(String(dateStr).substring(5, 7).c_str());
                int day = atoi(String(dateStr).substring(8, 10).c_str());
                uint32_t dateNum = year * 10000 + month * 100 + day;

                forecast[i].date = dateNum;
                forecast[i].tempMax = round((int)temps_max[i]);
                forecast[i].tempMin = round((int)temps_min[i]);
                forecast[i].weatherCode = (int)codes[i];
            }
            sortForecasts(forecast, NUM_DAYS);
            forecastReady = true;
            Serial.println("OK");
            success = true;
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
    tmElements_t tm;
    Watchy::RTC.read(tm);
    time_t now = makeTime(tm);
    long diff = now - savedTime;

    if (nextWeatherRetryTime != 0 && now < nextWeatherRetryTime) {
        Serial.println("Skip update until next retry window");
        return false;
    }

    if (hasForecastData() && savedTime != 0 && diff <= WEATHER_UPDATE_INTERVAL_SECONDS) {
        Serial.println("Don't need update");
        return false;
    }

    Serial.println("Update data...");

    bool success = false;
    bool locationReady =
        locationData.lat[0] != '\0' &&
        locationData.lon[0] != '\0' &&
        locationData.offset[0] != '\0';

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
    btStop();

    if (!success)
    {
        nextWeatherRetryTime = now + NETWORK_RETRY_INTERVAL_SECONDS;
    }

    return success;
}

bool CityWeatherService::hasForecastData() const
{
    if (!forecastReady) {
        return false;
    }

    for (int i = 0; i < NUM_DAYS; i++)
    {
        if (forecast[i].date > 20200101)
        {
            return true;
        }
    }

    return false;
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
