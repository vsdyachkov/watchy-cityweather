#pragma once
#include <Watchy.h>
class CityWeatherService;

class CityWeather : public Watchy{

    using Watchy::Watchy;
    public:
        void drawWatchFace();
        void drawTime();
        void drawBattery();
        void drawSky();
        void drawCity();
        virtual void handleButtonPress(); 
};




