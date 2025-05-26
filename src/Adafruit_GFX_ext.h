#include <Watchy.h>

void drawOutlinedBitmap(Adafruit_GFX &d, int16_t x, int16_t y, const unsigned char* bitmap , int16_t w, int16_t h, uint16_t color) {
  int borderColor = color == GxEPD_BLACK ? GxEPD_WHITE : GxEPD_BLACK;
  
  d.setTextColor(borderColor);
  for (int8_t dx = -2; dx <= 2; dx++) {
    for (int8_t dy = -2; dy <= 2; dy++) {
      if (dx != 0 || dy != 0) {
        d.drawBitmap(x + dx, y + dy, bitmap, w, h, borderColor);
      }
    }
  }

  d.drawBitmap(x, y, bitmap, w, h, color);
}

void drawOutlinedText(Adafruit_GFX &d, int16_t x, int16_t y, const String &text, int textColor) {

  int borderColor = textColor == GxEPD_BLACK ? GxEPD_WHITE : GxEPD_BLACK;
  d.setTextColor(borderColor);
  for (int8_t dx = -2; dx <= 2; dx++) {
    for (int8_t dy = -2; dy <= 2; dy++) {
      if (dx != 0 || dy != 0) {
        d.setCursor(x + dx, y + dy);
        d.print(text);
      }
    }
  }
  d.setTextColor(textColor);
  d.setCursor(x, y);
  d.print(text);
}

void printCentered(Adafruit_GFX &d, const String &text, int16_t centerX, int16_t y) {
  int16_t x1, y1;
  uint16_t w, h;
  d.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int16_t x = centerX - w/2;
  drawOutlinedText(d, x, y, text, GxEPD_BLACK);
}

void drawTextRightAligned(Adafruit_GFX &d, int xRight, int y, const String &text) {
  int16_t x1, y1;
  uint16_t w, h;
  d.getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int xStart = xRight - w;
  d.setCursor(xStart, y);
  d.print(text);
}

String clipStringToWidth(Adafruit_GFX &display, const GFXfont *f, const char *input, int16_t maxWidth) {
  if (!f) return String();

  uint8_t first = f->first;
  uint8_t last = f->last;
  int16_t w = 0;
  String result = "";

  for (const char *p = input; *p; p++) {
    char c = *p;
    if (c < first || c > last) continue;

    int16_t adv = f->glyph[c - first].xAdvance;
    if (w + adv > maxWidth) break;

    w += adv;
    result += c;
  }

  return result;
}
void drawLine(Adafruit_GFX &d, int x0, int y0, int x1, int y1, uint16_t c = GxEPD_BLACK, int dithering = 2) {
    int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, count = 0;
  
    while (true) {
      if (count % dithering == 0) d.drawPixel(x0, y0, c);
      if (x0 == x1 && y0 == y1) break;
      int e2 = 2 * err;
      if (e2 >= dy) { err += dy; x0 += sx; }
      if (e2 <= dx) { err += dx; y0 += sy; }
      count++;
    }
}

void fillRect(Adafruit_GFX &d,  int x, int y, int w, int h, uint16_t c = GxEPD_BLACK, int dithering = 2) {
  for (int row = 0; row < h; row = row + dithering) {
    drawLine(d, x, y + row, x + w - 1, y + row, c, dithering);
  }
}