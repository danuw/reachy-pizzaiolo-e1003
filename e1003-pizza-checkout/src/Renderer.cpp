#include "Renderer.h"

#include <math.h>
#include <TFT_eSPI.h>

namespace {
TFT_eSPI epaper;
bool g_displayReady = false;

constexpr uint16_t C_WHITE = TFT_WHITE;
constexpr uint16_t C_BLACK = TFT_BLACK;
constexpr uint16_t C_LIGHT = 0xC618;
constexpr uint16_t C_MID = 0x7BEF;

String truncateText(const String& text, size_t maxChars) {
  if (text.length() <= maxChars) {
    return text;
  }
  return text.substring(0, maxChars > 3 ? maxChars - 3 : maxChars) + "...";
}

String formatPrice(const String& currency, float value) {
  char buf[32];
  snprintf(buf, sizeof(buf), "%s%.2f", currency.c_str(), value);
  return String(buf);
}

void drawTextLine(int x, int y, const String& text) {
  epaper.setCursor(x, y);
  epaper.print(text);
}

void drawBasketPanel() {
  const int x = 0;
  const int y = 0;
  const int w = LEFT_W;
  const int h = SCREEN_H;

  epaper.fillRect(x, y, w, h, C_WHITE);
  epaper.drawRect(x, y, w, h, C_BLACK);

  epaper.setTextColor(C_BLACK, C_WHITE);
  epaper.setTextSize(2);

  int cursorY = 40;
  drawTextLine(24, cursorY, truncateText(g_basketState.title, 36));
  cursorY += 44;

  epaper.setTextSize(1);
  if (!g_basketState.status.isEmpty()) {
    drawTextLine(24, cursorY, truncateText(g_basketState.status, 65));
    cursorY += 28;
  }

  if (g_basketState.items.empty()) {
    drawTextLine(24, cursorY + 10, "No items yet.");
    drawTextLine(24, cursorY + 34, "Speak to Reachy to start your order.");
    cursorY += 86;
  } else {
    const int visible = min((int)g_basketState.items.size(), MAX_VISIBLE_ITEMS);
    for (int i = 0; i < visible; ++i) {
      const BasketItem& item = g_basketState.items[i];
      drawTextLine(24, cursorY, String(item.quantity) + " x " + truncateText(item.title, 34));
      cursorY += 24;
      if (!item.subtitle.isEmpty()) {
        drawTextLine(24, cursorY, truncateText(item.subtitle, 52));
        cursorY += 20;
      }

      const String left = formatPrice(g_basketState.currency, item.unitPrice) + " each";
      const String right = formatPrice(g_basketState.currency, item.lineTotal);
      drawTextLine(24, cursorY, left);
      drawTextLine(LEFT_W - 250, cursorY, right);
      cursorY += 32;
    }

    if ((int)g_basketState.items.size() > MAX_VISIBLE_ITEMS) {
      const int hidden = (int)g_basketState.items.size() - MAX_VISIBLE_ITEMS;
      drawTextLine(24, cursorY, "+ " + String(hidden) + " more items");
      cursorY += 28;
    }
  }

  const int totalsY = SCREEN_H - 180;
  epaper.drawLine(24, totalsY - 12, LEFT_W - 24, totalsY - 12, C_MID);

  drawTextLine(24, totalsY + 10, "Subtotal");
  drawTextLine(LEFT_W - 250, totalsY + 10, formatPrice(g_basketState.currency, g_basketState.subtotal));

  if (fabsf(g_basketState.discount) > 0.001f) {
    drawTextLine(24, totalsY + 36, "Discount");
    drawTextLine(LEFT_W - 250, totalsY + 36, formatPrice(g_basketState.currency, g_basketState.discount));
  }

  if (fabsf(g_basketState.delivery) > 0.001f) {
    drawTextLine(24, totalsY + 62, "Delivery");
    drawTextLine(LEFT_W - 250, totalsY + 62, formatPrice(g_basketState.currency, g_basketState.delivery));
  }

  epaper.setTextSize(2);
  drawTextLine(24, totalsY + 96, "Total");
  drawTextLine(LEFT_W - 280, totalsY + 96, formatPrice(g_basketState.currency, g_basketState.total));
  epaper.setTextSize(1);
}

uint16_t buttonFillForStyle(const String& style) {
  if (style == "danger") {
    return 0xBDF7;
  }
  if (style == "secondary") {
    return C_LIGHT;
  }
  return C_WHITE;
}

void drawRightPanel() {
  const int x = RIGHT_X;
  const int y = 0;
  const int w = SCREEN_W - RIGHT_X;
  const int h = SCREEN_H;

  epaper.fillRect(x, y, w, h, C_WHITE);
  epaper.drawRect(x, y, w, h, C_BLACK);

  const int imageX = x + 30;
  const int imageY = 30;
  const int imageW = w - 60;
  const int imageH = (int)(h * 0.38f);

  epaper.fillRect(imageX, imageY, imageW, imageH, C_LIGHT);
  epaper.drawRect(imageX, imageY, imageW, imageH, C_BLACK);
  epaper.setTextSize(2);
  drawTextLine(imageX + 16, imageY + (imageH / 2), truncateText(g_rightPanelState.image.label, 24));

  int cursorY = imageY + imageH + 36;
  epaper.setTextSize(2);
  drawTextLine(x + 30, cursorY, truncateText(g_rightPanelState.title, 35));
  cursorY += 42;

  epaper.setTextSize(1);
  if (!g_rightPanelState.subtitle.isEmpty()) {
    drawTextLine(x + 30, cursorY, truncateText(g_rightPanelState.subtitle, 64));
    cursorY += 34;
  }

  const int visibleButtons = min((int)g_rightPanelState.buttons.size(), MAX_VISIBLE_BUTTONS);
  g_buttonRects.clear();

  if (visibleButtons <= 0) {
    return;
  }

  const int buttonGap = 18;
  const int buttonH = min(130, (h - cursorY - 30 - ((visibleButtons - 1) * buttonGap)) / visibleButtons);
  int buttonY = cursorY;

  for (int i = 0; i < visibleButtons; ++i) {
    const ButtonDef& btn = g_rightPanelState.buttons[i];
    const int bx = x + 30;
    const int bw = w - 60;

    epaper.fillRect(bx, buttonY, bw, buttonH, buttonFillForStyle(btn.style));
    epaper.drawRect(bx, buttonY, bw, buttonH, C_BLACK);
    epaper.setTextColor(C_BLACK, buttonFillForStyle(btn.style));
    epaper.setTextSize(2);
    drawTextLine(bx + 20, buttonY + 36, truncateText(btn.label, 30));

    if (!btn.subtitle.isEmpty()) {
      epaper.setTextSize(1);
      drawTextLine(bx + 20, buttonY + 86, truncateText(btn.subtitle, 50));
    }

    ButtonRect rect;
    rect.id = btn.id;
    rect.label = btn.label;
    rect.x = bx;
    rect.y = buttonY;
    rect.w = bw;
    rect.h = buttonH;
    g_buttonRects.push_back(rect);

    buttonY += buttonH + buttonGap;
  }

  epaper.setTextColor(C_BLACK, C_WHITE);
}

void flush() {
  epaper.update();
}
}  // namespace

namespace Renderer {
bool begin() {
  epaper.init();
  epaper.setRotation(1);
  epaper.fillScreen(C_WHITE);

#if defined(GRAY_LEVEL16)
  epaper.initGrayMode(GRAY_LEVEL16);
#endif

  g_displayReady = true;
  return true;
}

void renderAll() {
  if (!g_displayReady) {
    return;
  }
  drawBasketPanel();
  drawRightPanel();
  flush();
}

void forceRefresh() {
  renderAll();
}
}  // namespace
