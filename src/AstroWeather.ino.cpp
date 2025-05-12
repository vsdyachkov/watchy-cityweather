#include "CityWeather.h"
#include "settings.h"

CityWeather watchy(settings);

void setup(){
  Serial.begin(115200);
  while (!Serial);
  watchy.init();
}

void loop(){}