#pragma once

#include <Arduino.h>
#include <Adafruit_GFX.h>

struct OpenSansCondensedGlyph
{
    uint16_t codepoint;
    uint16_t bitmapOffset;
    uint8_t width;
    uint8_t height;
    int8_t xOffset;
    int8_t yOffset;
    uint8_t xAdvance;
};

struct OpenSansCondensedFont
{
    const uint8_t *bitmap;
    const OpenSansCondensedGlyph *glyphs;
    uint16_t glyphCount;
    uint8_t lineHeight;
};

extern const OpenSansCondensedFont OpenSansCondBoldCyrillic9pt;

namespace OpenSansCondensed
{
String normalize(String text);
int16_t textWidth(const OpenSansCondensedFont &font, const String &text);
void drawText(
    Adafruit_GFX &display,
    const OpenSansCondensedFont &font,
    const String &text,
    int16_t x,
    int16_t baselineY,
    uint16_t color = 0
);
void drawOutlinedText(
    Adafruit_GFX &display,
    const OpenSansCondensedFont &font,
    const String &text,
    int16_t x,
    int16_t baselineY,
    uint16_t textColor,
    uint16_t outlineColor
);
uint16_t fittingUtf8Length(
    const OpenSansCondensedFont &font,
    const String &text,
    int16_t maxWidth
);
uint16_t wrappedLineLength(
    const OpenSansCondensedFont &font,
    const String &text,
    int16_t maxWidth
);
uint8_t printBlock(
    Adafruit_GFX &display,
    const OpenSansCondensedFont &font,
    String text,
    int16_t x,
    int16_t baselineY,
    int16_t maxWidth,
    uint8_t maxLines,
    uint16_t color = 0,
    bool heavy = false
);
void printLine(
    Adafruit_GFX &display,
    const OpenSansCondensedFont &font,
    String text,
    int16_t x,
    int16_t baselineY,
    int16_t maxWidth,
    bool rightAlign = false,
    uint16_t color = 0
);
void printCentered(
    Adafruit_GFX &display,
    const OpenSansCondensedFont &font,
    String text,
    int16_t centerX,
    int16_t baselineY,
    int16_t maxWidth,
    uint16_t color = 0
);
void printCenteredOutlined(
    Adafruit_GFX &display,
    const OpenSansCondensedFont &font,
    String text,
    int16_t centerX,
    int16_t baselineY,
    int16_t maxWidth,
    uint16_t textColor,
    uint16_t outlineColor
);
}
