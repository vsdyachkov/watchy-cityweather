#ifndef SETTINGS_H
#define SETTINGS_H

#define ipWhoUrl "http://ipwho.is/?fields=region,latitude,longitude,timezone.offset"
#define openMeteoUrl "https://api.open-meteo.com/v1/forecast?latitude={lat}&longitude={lon}&daily=temperature_2m_max,temperature_2m_min,weather_code&past_days=7&forecast_days=16&timezone=auto"

//Weather Settings
#define CITY_ID "524894" // legacy Watchy setting, overwritten by IP geolocation
#define OPENWEATHERMAP_APIKEY "" // legacy Watchy setting, unused by CityWeather
#define OPENWEATHERMAP_URL "http://api.openweathermap.org/data/2.5/weather?id={cityID}&lang={lang}&units={units}&appid={apiKey}" //open weather api
#define TEMP_UNIT "metric" //metric = Celsius , imperial = Fahrenheit
#define TEMP_LANG "en"
#define WEATHER_UPDATE_INTERVAL 60 //must be greater than 5, measured in minutes
//NTP Settings
#define NTP_SERVER "pool.ntp.org"
#define GMT_OFFSET_SEC 3600 * 3 // legacy Watchy setting, overwritten by IP geolocation

watchySettings settings{
    .cityID = CITY_ID,
    .lat = "",
    .lon = "",
    .weatherAPIKey = OPENWEATHERMAP_APIKEY,
    .weatherURL = OPENWEATHERMAP_URL,
    .weatherUnit = TEMP_UNIT,
    .weatherLang = TEMP_LANG,
    .weatherUpdateInterval = WEATHER_UPDATE_INTERVAL,
    .ntpServer = NTP_SERVER,
    .gmtOffset = GMT_OFFSET_SEC,
    .vibrateOClock = true,
};

#endif
