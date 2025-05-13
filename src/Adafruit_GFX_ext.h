#include <Watchy.h>

void drawOutlinedBitmap(Adafruit_GFX &d, int16_t x, int16_t y, const uint8_t bitmap[], int16_t w, int16_t h, uint16_t color) {
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

void drawStuckiDitherRect(Adafruit_GFX &d, int x, int y, int w, int h, float dithering) {
  // Буфер яркости (0.0 - 1.0) — градиент по вертикали
  float** buffer = new float*[h];
  for (int i = 0; i < h; i++) {
    buffer[i] = new float[w];
    float b = dithering + (float)i / (h - 1); // вертикальная заливка
    for (int j = 0; j < w; j++) buffer[i][j] = b;
  }

  // Алгоритм Stucki: распределение ошибки
  int diffusionX[] = { 1, 2, -2, -1, 0, 1, 2 };
  int diffusionY[] = { 0, 0, 1, 1, 1, 1, 1 };
  float weights[]   = { 8, 4, 2, 4, 8, 4, 2 }; // делить на 42

  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      float old = buffer[i][j];
      float newPixel = old < 0.5 ? 0 : 1;
      float error = old - newPixel;

      d.drawPixel(x + j, y + i, newPixel == 0 ? GxEPD_BLACK : GxEPD_WHITE);

      // распространение ошибки
      for (int k = 0; k < 7; k++) {
        int nx = j + diffusionX[k];
        int ny = i + diffusionY[k];
        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
          buffer[ny][nx] += error * (weights[k] / 42.0f);
        }
      }
    }
  }

  // очистка
  for (int i = 0; i < h; i++) delete[] buffer[i];
  delete[] buffer;
}

void drawStuckiDitherRectConst(Adafruit_GFX &d, int x, int y, int w, int h, float level) {
  // Буфер яркости (0.0 - 1.0) — теперь константный
  float** buffer = new float*[h];
  for (int i = 0; i < h; i++) {
    buffer[i] = new float[w];
    for (int j = 0; j < w; j++) {
      buffer[i][j] = level;  // вся заливка одним уровнем
    }
  }

  // Распределение ошибки по Stucki
  const int diffusionX[7] = { 1,  2, -2, -1, 0, 1, 2 };
  const int diffusionY[7] = { 0,  0,  1,  1, 1, 1, 1 };
  const float weights[7] = { 8, 4, 2, 4, 8, 4, 2 }; // делим на 42

  for (int i = 0; i < h; i++) {
    for (int j = 0; j < w; j++) {
      float oldVal = buffer[i][j];
      // решающее пороговое значение — 0.5
      float newVal = oldVal < 0.5f ? 0.0f : 1.0f;
      float err    = oldVal - newVal;

      // рисуем пиксель: чёрный для 0, белый для 1
      d.drawPixel(x + j, y + i, newVal == 0.0f ? GxEPD_BLACK : GxEPD_WHITE);

      // распространяем ошибку
      for (int k = 0; k < 7; k++) {
        int nx = j + diffusionX[k];
        int ny = i + diffusionY[k];
        if (nx >= 0 && nx < w && ny >= 0 && ny < h) {
          buffer[ny][nx] += err * (weights[k] / 42.0f);
        }
      }
    }
  }

  // освобождаем память
  for (int i = 0; i < h; i++) {
    delete[] buffer[i];
  }
  delete[] buffer;
}

// --- интерполяция Catmull-Rom ---
float catmull(float p0, float p1, float p2, float p3, float t) {
float t2 = t * t, t3 = t2 * t;
return 0.5 * ((2*p1) + (-p0+p2)*t + (2*p0 - 5*p1 + 4*p2 - p3)*t2 + (-p0 + 3*p1 - 3*p2 + p3)*t3);
}

void drawSpline(Adafruit_GFX &d, std::vector<int> values, int originX, int originY, float scaleX, float scaleY, uint16_t color = 0, int dithering = 2, int steps = 100)
{
  std::vector<int> x;
  std::vector<int> y;

  for (int i = 0; i < values.size(); i++)
  {
      x.push_back(int(originX + (i * scaleX)));
      y.push_back(int(originY - (values[i] * scaleY)));
  }
  x.insert(x.begin(), x.front());
  y.insert(y.begin(), y.front());
  x.push_back(x.back());
  y.push_back(y.back());

  int count = x.size();

  if (dithering < 1 || count < 4)
      return;
  int xt = x[1], yt = y[1];

  for (int i = 1; i < count - 2; i++)
  {
      for (int j = 0; j <= steps; j++)
      {
          float t = (float)j / steps;
          int xt1 = int(catmull(x[i-1], x[i], x[i+1], x[i+2], t));
          int yt1 = int(catmull(y[i-1], y[i], y[i+1], y[i+2], t));

          if ((int)xt1 % dithering == 0)
              drawLine(d, xt1, originY, xt1, yt1, color, dithering);
          if (j > 0)
              d.drawLine(xt, yt, xt1, yt1, color);
          xt = xt1;
          yt = yt1;
      }
  }
}

void drawTwoSplines(Adafruit_GFX &d, std::vector<int> values1, std::vector<int> values2, int originX, int originY, float scaleX, float scaleY, uint16_t color = 0, int dithering = 2, int steps = 100)
{
    std::vector<int> x;
    std::vector<int> y1;
    std::vector<int> y2;

    for (int i = 0; i < values1.size(); i++)
    {
        x.push_back(int(originX + (i * scaleX)));
        y1.push_back(int(originY - (values1[i] * scaleY)));
        y2.push_back(int(originY - (values2[i] * scaleY)));
    }
    x.insert(x.begin(), x.front());
    x.push_back(x.back());
    y1.insert(y1.begin(), y1.front());
    y1.push_back(y1.back());
    y2.insert(y2.begin(), y2.front());
    y2.push_back(y2.back());

    int count = x.size();

    if (dithering < 1 || count < 4)
        return;

    std::vector<bool> filled(d.width(), false);

    int xt_prev = x[1];
    int yt1_prev = y1[1];
    int yt2_prev = y2[1];

    for (int i = 1; i < count - 2; i++)
    {
        for (int j = 0; j <= steps; j++)
        {
            float t = (float)j / steps;
            int xt1 = int(catmull(x[i-1], x[i], x[i+1], x[i+2], t));
            int yt1 = int(catmull(y1[i-1], y1[i], y1[i+1], y1[i+2], t));
            int yt2 = int(catmull(y2[i-1], y2[i], y2[i+1], y2[i+2], t));

            if (xt1% dithering == 0 && xt1 >= 0 && xt1 < (int)filled.size() && !filled[xt1]) {
                int ya = std::min(yt1, yt2),
                    yb = std::max(yt1, yt2);
                drawLine(d, xt1, ya, xt1, yb, color, dithering);
                filled[xt1] = true;
            }

            if (j > 0) {
                d.drawLine(xt_prev, yt1_prev, xt1, yt1, color);
                d.drawLine(xt_prev, yt2_prev, xt1, yt2, color);
            }
            
            xt_prev = xt1;
            yt1_prev = yt1;
            yt2_prev = yt2;
        }
    }
}
