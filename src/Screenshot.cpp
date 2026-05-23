#include "Screenshot.h"

#include <string.h>

namespace
{
constexpr uint16_t SCREENSHOT_WIDTH = 200;
constexpr uint16_t SCREENSHOT_HEIGHT = 200;
constexpr uint16_t SCREENSHOT_ROW_BYTES = SCREENSHOT_WIDTH / 8;
constexpr size_t SCREENSHOT_BUFFER_SIZE =
    static_cast<size_t>(SCREENSHOT_ROW_BYTES) * SCREENSHOT_HEIGHT;
constexpr uint32_t SCREENSHOT_MAGIC = 0x43575353;
constexpr uint8_t BYTES_PER_SERIAL_LINE = 64;

RTC_DATA_ATTR uint8_t screenshotBuffer[SCREENSHOT_BUFFER_SIZE];
RTC_DATA_ATTR uint32_t screenshotBufferMagic = 0;

bool isMenuButtonPressed()
{
#ifdef ARDUINO_ESP32S3_DEV
  pinMode(MENU_BTN_PIN, INPUT_PULLUP);
  return digitalRead(MENU_BTN_PIN) == LOW;
#else
  pinMode(MENU_BTN_PIN, INPUT);
  return digitalRead(MENU_BTN_PIN) == HIGH;
#endif
}

void ensureScreenshotBuffer()
{
  if (screenshotBufferMagic == SCREENSHOT_MAGIC)
  {
    return;
  }

  memset(screenshotBuffer, 0xFF, sizeof(screenshotBuffer));
  screenshotBufferMagic = SCREENSHOT_MAGIC;
}

void setScreenshotPixel(uint16_t x, uint16_t y, bool white)
{
  if (x >= SCREENSHOT_WIDTH || y >= SCREENSHOT_HEIGHT)
  {
    return;
  }

  uint8_t &byte = screenshotBuffer[static_cast<size_t>(y) * SCREENSHOT_ROW_BYTES + x / 8];
  uint8_t mask = 0x80 >> (x & 0x07);
  if (white)
  {
    byte |= mask;
  }
  else
  {
    byte &= ~mask;
  }
}

bool sourcePixelIsWhite(const uint8_t *buffer, uint16_t x, uint16_t y)
{
  size_t index = static_cast<size_t>(y) * SCREENSHOT_ROW_BYTES + x / 8;
  uint8_t mask = 0x80 >> (x & 0x07);
  return (buffer[index] & mask) != 0;
}

void waitForMenuReleased(uint32_t timeoutMs)
{
  uint32_t startMs = millis();
  while (isMenuButtonPressed() && millis() - startMs < timeoutMs)
  {
    delay(20);
  }
}
}

extern "C" void cityWeatherScreenshotFrame(
    const uint8_t *buffer,
    size_t size,
    uint16_t x,
    uint16_t y,
    uint16_t w,
    uint16_t h,
    bool fullFrame
)
{
  if (buffer == nullptr || size < SCREENSHOT_BUFFER_SIZE)
  {
    return;
  }

  ensureScreenshotBuffer();

  if (fullFrame || (x == 0 && y == 0 && w >= SCREENSHOT_WIDTH && h >= SCREENSHOT_HEIGHT))
  {
    memcpy(screenshotBuffer, buffer, SCREENSHOT_BUFFER_SIZE);
    return;
  }

  uint16_t right = x + w;
  uint16_t bottom = y + h;
  if (right < x || right > SCREENSHOT_WIDTH)
  {
    right = SCREENSHOT_WIDTH;
  }
  if (bottom < y || bottom > SCREENSHOT_HEIGHT)
  {
    bottom = SCREENSHOT_HEIGHT;
  }
  for (uint16_t yy = y; yy < bottom; yy++)
  {
    for (uint16_t xx = x; xx < right; xx++)
    {
      setScreenshotPixel(xx, yy, sourcePixelIsWhite(buffer, xx, yy));
    }
  }
}

void dumpCityWeatherScreenshotToSerial()
{
  ensureScreenshotBuffer();

  Serial.begin(115200);
  delay(80);

  Serial.println();
  Serial.print("CW_SCREENSHOT_BEGIN ");
  Serial.print(SCREENSHOT_WIDTH);
  Serial.print(' ');
  Serial.print(SCREENSHOT_HEIGHT);
  Serial.print(' ');
  Serial.println(SCREENSHOT_BUFFER_SIZE);

  for (size_t i = 0; i < SCREENSHOT_BUFFER_SIZE; i++)
  {
    if (screenshotBuffer[i] < 0x10)
    {
      Serial.print('0');
    }
    Serial.print(screenshotBuffer[i], HEX);
    if ((i + 1) % BYTES_PER_SERIAL_LINE == 0)
    {
      Serial.println();
    }
  }
  if (SCREENSHOT_BUFFER_SIZE % BYTES_PER_SERIAL_LINE != 0)
  {
    Serial.println();
  }

  Serial.println("CW_SCREENSHOT_END");
  Serial.flush();
}

bool handleCityWeatherScreenshotShortcut(Watchy *watchy, uint16_t holdMs)
{
  if (!isMenuButtonPressed())
  {
    return false;
  }

  uint32_t pressedAtMs = millis();
  while (isMenuButtonPressed())
  {
    if (millis() - pressedAtMs >= holdMs)
    {
      if (watchy != nullptr)
      {
        watchy->vibMotor(35, 2);
      }
      dumpCityWeatherScreenshotToSerial();
      waitForMenuReleased(2000);
      return true;
    }
    delay(20);
  }

  return false;
}
