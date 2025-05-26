#include "CityWeatherService.h"
#include "CityWeather.h"
#include "Images.h"

#define IP_WHO_URL "http://ipwho.is/199.229.250.5?fields=city,latitude,longitude,timezone.offset"
// #define IP_WHO_URL "http://ipwho.is/?fields=city,country,latitude,longitude,timezone.offset"
#define OPEN_METEO_URL "https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}&daily=temperature_2m_max,temperature_2m_min,weather_code&past_days=7&forecast_days=16&timezone=auto"
#define OPEN_METEO_UPDATE_INTERVAL 60

#define NUM_DAYS 23

RTC_DATA_ATTR LocationData locationData;
RTC_DATA_ATTR const char *wdayNames[7] = {"Su", "Mo", "Tu", "We", "Th", "Fr", "Sa"};

RTC_DATA_ATTR DailyForecast forecast[NUM_DAYS];

RTC_DATA_ATTR int updateMinutes = OPEN_METEO_UPDATE_INTERVAL;

CityWeatherService::CityWeatherService(CityWeather &cw) : cityWeather(cw) {}

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
    // Serial.print(" " + ipWhoUrl + " ");
    http.begin(IP_WHO_URL);
    int httpCode = http.GET();
    if (httpCode == 200)
    {
        String payload = http.getString();
        JSONVar responseObject = JSON.parse(payload);
        if (JSON.typeof(responseObject) == "undefined")
        {
            Serial.println("Failed to parse location JSON");
            return false;
        }
        // Serial.println(payload);

        auto cityVar = responseObject["city"];
        if (JSON.typeof(cityVar) == "string") {
        strncpy(locationData.city,
                (const char*)cityVar,
                sizeof(locationData.city) - 1);
        }
        locationData.city[sizeof(locationData.city)-1] = '\0';

        locationData.lat = String((double)responseObject["latitude"], 6);
        locationData.lon = String((double)responseObject["longitude"], 6);
        locationData.offset = String((int)responseObject["timezone"]["offset"]);
        Serial.println("OK");
        return true;
    }
    else
    {
        return false;
        Serial.println("Error on location HTTP request");
    }
    http.end();
}

bool CityWeatherService::getWeatherData()
{
    HTTPClient http;
    http.setConnectTimeout(20000);
    String weatherQueryURL = OPEN_METEO_URL;
    if (locationData.lat != "" && locationData.lon != "")
    {
        weatherQueryURL.replace("{lat}", locationData.lat);
        weatherQueryURL.replace("{lon}", locationData.lon);
    }
    else
    {
        Serial.println("Error: locationData in null");
        return false;
    }

    // Serial.print(" weatherUrl: " + weatherQueryURL) + " ";
    http.begin(weatherQueryURL);
    int httpCode = http.GET();
    if (httpCode == 200)
    {
        String payload = http.getString();
        JSONVar responseObject = JSON.parse(payload);
        if (JSON.typeof(responseObject) == "undefined")
        {
            Serial.println("Failed to parse weather JSON");
            return false;
        }
        // Serial.println(payload);

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
        Serial.println("OK");
        return true;
    }
    else
    {
        Serial.println("Error on weather HTTP request");
        return false;
    }
    http.end();
}

bool CityWeatherService::updateWifiData()
{

    updateMinutes++;

    Serial.print("#0. updateMinutes: " + String(updateMinutes) + "/" + OPEN_METEO_UPDATE_INTERVAL + " ");

    if (updateMinutes < OPEN_METEO_UPDATE_INTERVAL)
    {
        Serial.println("Don't need update");
        return false;
    }

    updateMinutes = 0;
    Serial.println("Update data...");

    if (cityWeather.connectWiFi())
    {
        Serial.println("#1. Wifi connected");

        Serial.print("#2. getLocationData... ");
        if (retry([&]() { return getLocationData(); }, 3))
        {
            Serial.print("#3. syncNTP GMT: " + locationData.offset + "... ");
            if (retry([&]() { return cityWeather.syncNTP(locationData.offset.toInt()); }, 3))
            {
                Serial.println("OK");
                Serial.print("#4. getWeatherData...");
                if (retry([&]() { return getWeatherData(); }, 3)){
                    // good
                } else {
                    return false;
                }
            }
            else
            {
                Serial.println("failed");
                return false;
            }
        }
        else
        {
            return false;
        }

        WiFi.mode(WIFI_OFF);
        btStop();
        return true;
    }
    Serial.println("#1. Wifi not connected");

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

    sortForecasts(forecast, NUM_DAYS);

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