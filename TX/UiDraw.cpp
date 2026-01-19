#include "UiDraw.h"

#include "UiLayout.h"

namespace {
ColorToken tempColor(float temp) {
  if (temp >= 85.0f) return STATE_CRIT;
  if (temp >= 65.0f) return STATE_WARN;
  return STATE_OK;
}

static const char *kContextItems[] = {"ADVANCED", "RESET", "CLOSE"};
static const uint8_t kContextCount = 3;
}  // namespace

void UiDrawOverlay(Renderer &renderer, const UiState &state) {
  UiDrawStatusBar(renderer, state);
  UiDrawTempStrip(renderer, state);
}

void UiDrawStatusBar(Renderer &renderer, const UiState &state) {
  renderer.fillRect(0, 0, UiLayout::ScreenW, UiLayout::StatusH, BG_STATUS);
  renderer.drawHLine(0, UiLayout::StatusH - 1, UiLayout::ScreenW, GRID_LINE);

  renderer.drawText(2, 3, "RX", 1, TEXT_PRIMARY);
  renderer.fillRect(16, 4, 6, 6, state.rxConnected ? STATE_OK : STATE_CRIT);

  renderer.drawText(28, 3, "GY", 1, TEXT_PRIMARY);
  renderer.fillRect(42, 4, 6, 6, state.gyroOn ? STATE_OK : STATE_WARN);

  char buf[8];
  if (state.rxVoltageAvailable) {
    snprintf(buf, sizeof(buf), "R%.1f", state.rxVoltage);
    renderer.drawTextRight(90, 3, buf, 1, TEXT_PRIMARY);
  }
  snprintf(buf, sizeof(buf), "T%.1f", state.txVoltage);
  renderer.drawTextRight(126, 3, buf, 1, TEXT_PRIMARY);
}

void UiDrawTempStrip(Renderer &renderer, const UiState &state) {
  int16_t segW = UiLayout::ScreenW / 3;
  int16_t y = UiLayout::StatusH;

  renderer.fillRect(0, y, segW, UiLayout::TempH, tempColor(state.tempMotor));
  renderer.fillRect(segW, y, segW, UiLayout::TempH, tempColor(state.tempEsc));
  renderer.fillRect(segW * 2, y, UiLayout::ScreenW - (segW * 2), UiLayout::TempH, tempColor(state.tempBoard));

  char buf[8];
  snprintf(buf, sizeof(buf), "M%d", static_cast<int>(state.tempMotor));
  renderer.drawText(2, y + 2, buf, 1, TEXT_INVERT);
  snprintf(buf, sizeof(buf), "E%d", static_cast<int>(state.tempEsc));
  renderer.drawText(segW + 2, y + 2, buf, 1, TEXT_INVERT);
  snprintf(buf, sizeof(buf), "B%d", static_cast<int>(state.tempBoard));
  renderer.drawText(segW * 2 + 2, y + 2, buf, 1, TEXT_INVERT);
}

void UiDrawListHeader(Renderer &renderer, const char *title) {
  renderer.fillRect(0, UiLayout::ContentY, UiLayout::ScreenW, UiLayout::HeaderH, BG_PANEL_ALT);
  renderer.drawText(6, UiLayout::ContentY + 3, title, 1, TEXT_PRIMARY);
  renderer.drawHLine(0, UiLayout::ContentY + UiLayout::HeaderH - 1, UiLayout::ScreenW, ACCENT_CYAN);
}

void UiDrawListRow(Renderer &renderer, int row, const char *label, const char *value, bool focused, bool editing) {
  int16_t y = UiLayout::ListY + (row * UiLayout::ItemH);
  ColorToken bg = (row % 2 == 0) ? BG_PANEL : BG_PANEL_ALT;
  if (focused) {
    bg = FOCUS_BG;
  }
  renderer.fillRect(0, y, UiLayout::ScreenW, UiLayout::ItemH, bg);
  ColorToken border = focused ? (editing ? ACCENT_AMBER : FOCUS_BORDER) : GRID_LINE;
  renderer.drawRect(0, y, UiLayout::ScreenW, UiLayout::ItemH, border);
  if (focused) {
    renderer.fillRect(0, y, 3, UiLayout::ItemH, border);
  }
  int16_t textY = y + (UiLayout::ItemH - renderer.textHeight(1)) / 2;
  renderer.drawText(6, textY, label, 1, focused ? TEXT_INVERT : TEXT_PRIMARY);
  if (value) {
    renderer.drawTextRight(124, textY, value, 1, focused ? TEXT_INVERT : TEXT_MUTED);
  }
}

void UiDrawContextMenu(Renderer &renderer, const UiContext &ctx) {
  int16_t w = 96;
  int16_t h = (kContextCount * 16) + 8;
  int16_t x = (UiLayout::ScreenW - w) / 2;
  int16_t y = UiLayout::ContentY + 10;
  renderer.fillRect(x, y, w, h, BG_PANEL);
  renderer.drawRect(x, y, w, h, ACCENT_CYAN);

  for (uint8_t i = 0; i < kContextCount; ++i) {
    int16_t rowY = y + 6 + (i * 16);
    bool focused = (i == ctx.contextIndex);
    if (focused) {
      renderer.fillRect(x + 2, rowY - 2, w - 4, 14, FOCUS_BG);
    }
    renderer.drawText(x + 6, rowY, kContextItems[i], 1, focused ? TEXT_INVERT : TEXT_PRIMARY);
  }
}
