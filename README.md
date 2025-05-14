# watchy-cityweather
CityWeather watchface for Watchy

![screenshot](./screenshot.png)

For install watchface you need VSCode + PlatformIO

Watchface displays sunrise/sunset and weather for the current week

This Watchface uses a WiFi connection to automatically determine the approximate location, synchronize the date/time (NTP) and download the weather forecast. No manual settings or API keys are required.

Services are used:

- http://ipwho.is
- https://api.open-meteo.com
