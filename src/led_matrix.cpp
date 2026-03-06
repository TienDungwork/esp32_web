#include "led_matrix.h"
#include <Adafruit_GFX.h>

MatrixPanel_I2S_DMA* dma_display = nullptr;

// Wrapper để mirror xen kẽ các bảng lẻ (1,3,5...) khi vẽ chữ
class MirrorMatrixWrapper : public Adafruit_GFX {
 public:
  MirrorMatrixWrapper(int16_t w, int16_t h, MatrixPanel_I2S_DMA* real, int panelW)
    : Adafruit_GFX(w, h), _real(real), _panelW(panelW) {}
  void drawPixel(int16_t x, int16_t y, uint16_t color) override {
    if (!_real || x < 0 || x >= width() || y < 0 || y >= height()) return;
    int p = x / _panelW;
    int16_t x2 = (p % 2 == 1)
      ? (p * _panelW + (_panelW - 1 - (x % _panelW)))
      : x;
    _real->drawPixel(x2, y, color);
  }
  void fillScreen(uint16_t color) override {
    if (_real) _real->fillScreen(color);
  }
 private:
  MatrixPanel_I2S_DMA* _real;
  int _panelW;
};

static MirrorMatrixWrapper* mirror_display = nullptr;

uint16_t LED_COLOR_BLACK;
uint16_t LED_COLOR_WHITE;
uint16_t LED_COLOR_RED;
uint16_t LED_COLOR_GREEN;
uint16_t LED_COLOR_BLUE;
uint16_t LED_COLOR_YELLOW;
uint16_t LED_COLOR_CYAN;

void ledMatrixInit() {
  Serial.println("Initializing LED Matrix (HUB75)...");

  HUB75_I2S_CFG::i2s_pins _pins = {
    R1_PIN, G1_PIN, B1_PIN,
    R2_PIN, G2_PIN, B2_PIN,
    A_PIN, B_PIN, C_PIN,
    D_PIN, E_PIN,
    LAT_PIN, OE_PIN, CLK_PIN
  };

  HUB75_I2S_CFG mxconfig(PANEL_RES_X, PANEL_RES_Y, PANEL_CHAIN, _pins);

  mxconfig.i2sspeed = HUB75_I2S_CFG::HZ_8M;
  mxconfig.latch_blanking = 2;
  mxconfig.clkphase = true;
  mxconfig.double_buff = false;

  dma_display = new MatrixPanel_I2S_DMA(mxconfig);
  if (!dma_display) {
    Serial.println("FAILED to create LED Matrix display!");
    return;
  }

  dma_display->begin();
  dma_display->setRotation(PANEL_ROTATION);

  LED_COLOR_BLACK = dma_display->color565(0, 0, 0);
  LED_COLOR_WHITE = dma_display->color565(255, 255, 255);
  LED_COLOR_RED = dma_display->color565(255, 0, 0);
  LED_COLOR_GREEN = dma_display->color565(0, 255, 0);
  LED_COLOR_BLUE = dma_display->color565(0, 0, 255);
  LED_COLOR_YELLOW = dma_display->color565(255, 255, 0);
  LED_COLOR_CYAN = dma_display->color565(0, 255, 255);

  dma_display->setBrightness8(180);
  dma_display->clearScreen();

  int totalW = PANEL_RES_X * PANEL_CHAIN;
  mirror_display = new MirrorMatrixWrapper(totalW, PANEL_RES_Y, dma_display, PANEL_RES_X);

  Serial.println("LED Matrix initialized");
}

void ledMatrixShowCenterText(const String& text, uint16_t color, int y) {
  if (!dma_display) return;

  dma_display->clearScreen();
  dma_display->setTextSize(1.6);
  dma_display->setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;
  dma_display->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int totalWidth = PANEL_RES_X * PANEL_CHAIN;
  int x = (totalWidth - w) / 2;

  dma_display->setCursor(x, y);
  dma_display->print(text);
}

void ledMatrixTestBasic() {
  if (!dma_display) return;

  dma_display->fillScreen(LED_COLOR_WHITE);
  delay(500);
  dma_display->fillScreen(LED_COLOR_RED);
  delay(500);
  dma_display->fillScreen(LED_COLOR_BLUE);
  delay(500);
  dma_display->clearScreen();

  ledMatrixShowCenterText("TEST OK", LED_COLOR_YELLOW, 8);
}

int ledMatrixGetMaxBoardCount() {
  return PANEL_CHAIN;
}

static int clampInt(int value, int low, int high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

static float clampFloat(float value, float low, float high) {
  if (value < low) return low;
  if (value > high) return high;
  return value;
}

void ledMatrixShowMultiLine(const String lines[], const float fontSizes[], int lineCount, int boardCount) {
  if (!dma_display || lineCount <= 0) return;
  Adafruit_GFX* draw = (mirror_display != nullptr) ? (Adafruit_GFX*)mirror_display : (Adafruit_GFX*)dma_display;

  const int safeLineCount = clampInt(lineCount, 1, 5);
  const int safeBoardCount = clampInt(boardCount, 1, PANEL_CHAIN);
  const int totalWidth = PANEL_RES_X * safeBoardCount;
  const int maxVisibleLines = 5;

  int lineIndexes[maxVisibleLines] = {0, 0, 0, 0, 0};
  int visibleCount = 0;
  for (int i = 0; i < safeLineCount && visibleCount < maxVisibleLines; i++) {
    if (lines[i].length() > 0) {
      lineIndexes[visibleCount++] = i;
    }
  }

  if (visibleCount == 0) {
    dma_display->clearScreen();
    return;
  }

  int heights[maxVisibleLines] = {0, 0, 0, 0, 0};
  int yOffsets[maxVisibleLines] = {0, 0, 0, 0, 0};
  int totalTextHeight = 0;
  const int lineSpacing = 1;

  for (int i = 0; i < visibleCount; i++) {
    const int idx = lineIndexes[i];
    draw->setTextSize(clampFloat(fontSizes[idx], 0.8f, 8.0f));

    int16_t x1, y1;
    uint16_t w, h;
    draw->getTextBounds(lines[idx], 0, 0, &x1, &y1, &w, &h);
    heights[i] = static_cast<int>(h);
    yOffsets[i] = static_cast<int>(y1);
    totalTextHeight += heights[i];
  }

  totalTextHeight += (visibleCount - 1) * lineSpacing;

  float scale = 1.0f;
  if (totalTextHeight > PANEL_RES_Y && totalTextHeight > 0) {
    scale = (float)PANEL_RES_Y / (float)totalTextHeight;
    if (scale < 0.5f) scale = 0.5f;
  }

  int scaledTotalHeight = (int)((float)totalTextHeight * scale);
  int top = (PANEL_RES_Y - scaledTotalHeight) / 2;
  if (top < 0) top = 0;

  dma_display->clearScreen();
  draw->setTextColor(LED_COLOR_GREEN);

  int cursorTop = top;
  for (int i = 0; i < visibleCount; i++) {
    const int idx = lineIndexes[i];
    float sz = clampFloat(fontSizes[idx], 0.8f, 8.0f) * scale;
    if (sz < 0.8f) sz = 0.8f;
    draw->setTextSize(sz);

    int16_t x1, y1;
    uint16_t w, h;
    draw->getTextBounds(lines[idx], 0, 0, &x1, &y1, &w, &h);

    int x = (totalWidth - static_cast<int>(w)) / 2;
    if (x < 0) x = 0;

    int baselineY = cursorTop - y1;
    if (baselineY < 0) baselineY = 0;

    draw->setCursor(x, baselineY);
    draw->print(lines[idx]);

    cursorTop += (int)h + lineSpacing;
  }
}
