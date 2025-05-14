# watchy-cityweather
CityWeather watchface for Watchy

![screenshot](./screenshot.png)

[Watchy](https://watchy.sqfmi.com) - Fully Open Source E-Paper Watch 

For install watchface you need [VSCode](https://code.visualstudio.com) + [PlatformIO](https://platformio.org)

Watchface displays schematic image and weather for the current week

This Watchface uses a WiFi connection to automatically determine the approximate location, synchronize the date/time (NTP) and download the weather forecast. No manual IDE settings or API keys are required

Web services are used:

- https://ipwhois.io
- https://open-meteo.com

Tools were used:

- https://watchy.sqfmi.com/docs/create-watchface - Watchy docs
- https://github.com/sqfmi/Watchy  - Watchy SDK
- https://lopaka.app - UI graphics editor for embedded screens
- https://javl.github.io/image2cpp - Image to byte array converter
- https://rop.nl/truetype2gfx - Standard font to Adafruit_GFX font converter
