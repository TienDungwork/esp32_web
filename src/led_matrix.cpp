#include "led_matrix.h"
#include <ESP32-HUB75-VirtualMatrixPanel_T.hpp>

MatrixPanel_I2S_DMA* dma_display = nullptr;

// Dùng VirtualMatrixPanel_T để map đúng layout 3 hàng x 2 cột.
// Với mô tả của bạn (mỗi hàng đi trái->phải), thường wiring sẽ hợp với *_ZZ.
// Layout bạn mô tả là serpentine theo hàng: → ← →  (hàng 2 bị đảo chiều)
static VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>* virtual_display = nullptr;

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

  if (!virtual_display) {
    virtual_display = new VirtualMatrixPanel_T<CHAIN_TOP_LEFT_DOWN>(
      VDISP_NUM_ROWS, VDISP_NUM_COLS, PANEL_RES_X, PANEL_RES_Y
    );
    virtual_display->setDisplay(*dma_display);
    virtual_display->setRotation(PANEL_ROTATION);
  }

  Serial.println("LED Matrix initialized");
}

void ledMatrixShowCenterText(const String& text, uint16_t color, int y) {
  if (!dma_display) return;
  auto* draw = (virtual_display != nullptr) ? (Adafruit_GFX*)virtual_display : (Adafruit_GFX*)dma_display;

  dma_display->clearScreen();
  draw->setTextSize(1.6);
  draw->setTextColor(color);

  int16_t x1, y1;
  uint16_t w, h;
  draw->getTextBounds(text, 0, 0, &x1, &y1, &w, &h);
  int totalWidth = draw->width();
  int x = (totalWidth - (int)w) / 2;

  draw->setCursor(x, y);
  draw->print(text);
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
  return VDISP_NUM_ROWS * VDISP_NUM_COLS;
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

uint16_t ledMatrixParseColor(const String& hex, uint16_t fallback) {
  String s = hex;
  s.trim();
  if (s.startsWith("#")) s.remove(0, 1);
  if (s.length() != 6) return fallback;

  char buf[7];
  s.toCharArray(buf, sizeof(buf));
  char* endPtr = nullptr;
  uint32_t rgb = strtoul(buf, &endPtr, 16);
  if (endPtr == buf) return fallback;

  uint8_t r = (rgb >> 16) & 0xFF;
  uint8_t g = (rgb >> 8) & 0xFF;
  uint8_t b = rgb & 0xFF;

  // Tự tính 565, không phụ thuộc dma_display
  uint16_t c = ((r & 0xF8) << 8) |
               ((g & 0xFC) << 3) |
               (b >> 3);
  return c;
}

void ledMatrixShowMultiLine(const String lines[], const float fontSizes[], const uint16_t colors[], int lineCount, int boardCount) {
  if (!dma_display || lineCount <= 0) return;
  Adafruit_GFX* draw = (virtual_display != nullptr) ? (Adafruit_GFX*)virtual_display : (Adafruit_GFX*)dma_display;

  const int safeLineCount = clampInt(lineCount, 1, 5);
  (void)boardCount; // dùng toàn bộ layout 6 bảng (2 cột x 3 hàng)
  const int totalWidth = draw->width();
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
  int textHeightSum = 0;

  for (int i = 0; i < visibleCount; i++) {
    const int idx = lineIndexes[i];
    draw->setTextSize(clampFloat(fontSizes[idx], 0.8f, 8.0f));

    int16_t x1, y1;
    uint16_t w, h;
    draw->getTextBounds(lines[idx], 0, 0, &x1, &y1, &w, &h);
    heights[i] = static_cast<int>(h);
    textHeightSum += heights[i];
  }

  // Tự tính khoảng cách dòng theo chiều cao chữ để nhìn rõ hơn
  const int avgH = (visibleCount > 0) ? max(1, textHeightSum / visibleCount) : 1;
  const int lineSpacing = clampInt(avgH / 4, 2, 12); // ~25% chiều cao chữ, min 2px
  int totalTextHeight = textHeightSum + (visibleCount - 1) * lineSpacing;

  float scale = 1.0f;
  const int availableH = draw->height();
  if (totalTextHeight > availableH && totalTextHeight > 0) {
    scale = (float)availableH / (float)totalTextHeight;
    if (scale < 0.5f) scale = 0.5f;
  }

  int scaledTotalHeight = (int)((float)totalTextHeight * scale);
  int top = (availableH - scaledTotalHeight) / 2;
  if (top < 0) top = 0;

  dma_display->clearScreen();

  int cursorTop = top;
  for (int i = 0; i < visibleCount; i++) {
    const int idx = lineIndexes[i];
    float sz = clampFloat(fontSizes[idx], 0.8f, 8.0f) * scale;
    if (sz < 0.8f) sz = 0.8f;
    draw->setTextSize(sz);

    int16_t x1, y1;
    uint16_t w, h;
    draw->getTextBounds(lines[idx], 0, 0, &x1, &y1, &w, &h);

    // Căn trái với một chút margin, không căn giữa nữa
    const int xMargin = 4;
    int x = xMargin;

    int baselineY = cursorTop - y1;
    if (baselineY < 0) baselineY = 0;

    uint16_t c = colors ? colors[idx] : LED_COLOR_GREEN;
    draw->setTextColor(c);
    draw->setCursor(x, baselineY);
    draw->print(lines[idx]);

    cursorTop += (int)h + (int)((float)lineSpacing * scale);
  }
}
