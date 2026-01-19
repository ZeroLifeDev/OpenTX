#include "ScreenDashboard.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

namespace {
void drawCenterBar(Renderer &renderer, int16_t x, int16_t y, int16_t w, int16_t h, int16_t pct) {
  renderer.fillRect(x, y, w, h, BG_PANEL_ALT);
  renderer.drawRect(x, y, w, h, GRID_LINE);
  int16_t mid = x + w / 2;
  renderer.drawVLine(mid, y, h, GRID_LINE);

  if (pct == 0) return;
  if (pct > 0) {
    int16_t fill = (w / 2) * pct / 100;
    renderer.fillRect(mid, y + 1, fill, h - 2, BAR_POS);
  } else {
    int16_t fill = (w / 2) * (-pct) / 100;
    renderer.fillRect(mid - fill, y + 1, fill, h - 2, BAR_NEG);
  }
}
}  // namespace

void ScreenDashboard_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  int16_t y = UiLayout::ContentY;

  int16_t speedH = 40;
  renderer.fillRect(0, y, UiLayout::ScreenW, speedH, BG_PANEL);
  renderer.drawRect(0, y, UiLayout::ScreenW, speedH, GRID_LINE);
  renderer.drawText(6, y + 4, "SPEED", 1, TEXT_MUTED);
  char buf[8];
  snprintf(buf, sizeof(buf), "%d", static_cast<int>(state.speedKmh));
  renderer.drawTextRight(110, y + 8, buf, 3, TEXT_PRIMARY);
  renderer.drawTextRight(124, y + 22, "KMH", 1, TEXT_MUTED);

  y += speedH;
  int16_t ctlH = 46;
  renderer.fillRect(0, y, UiLayout::ScreenW, ctlH, BG_PANEL_ALT);
  renderer.drawRect(0, y, UiLayout::ScreenW, ctlH, GRID_LINE);
  renderer.drawText(6, y + 4, "THROTTLE", 1, TEXT_MUTED);
  drawCenterBar(renderer, 58, y + 6, 66, 10, static_cast<int>(state.throttlePct));

  renderer.drawText(6, y + 24, "STEER", 1, TEXT_MUTED);
  drawCenterBar(renderer, 58, y + 26, 66, 10, static_cast<int>(state.steerPct));

  y += ctlH;
  int16_t auxH = UiLayout::ScreenH - y;
  renderer.fillRect(0, y, UiLayout::ScreenW, auxH, BG_PANEL);
  renderer.drawRect(0, y, UiLayout::ScreenW, auxH, GRID_LINE);

  renderer.drawText(6, y + 4, "SUSP", 1, TEXT_MUTED);
  renderer.drawVBar(10, y + 16, 12, auxH - 22, static_cast<int>(state.suspensionPct), ACCENT_AMBER, BG_PRIMARY);

  renderer.drawText(28, y + 4, "GYRO", 1, TEXT_MUTED);
  renderer.drawText(62, y + 4, state.gyroOn ? "ON" : "OFF", 1, state.gyroOn ? STATE_OK : STATE_WARN);

  renderer.drawText(28, y + 20, "HL", 1, TEXT_MUTED);
  renderer.drawText(50, y + 20, UiStrings::triStateLabel(state.headlights), 1, TEXT_PRIMARY);
  renderer.drawText(82, y + 20, "TL", 1, TEXT_MUTED);
  renderer.drawText(100, y + 20, UiStrings::onOffLabel(state.taillights == ACC_ON), 1, TEXT_PRIMARY);

  renderer.drawText(28, y + 36, "SIG", 1, TEXT_MUTED);
  renderer.drawText(64, y + 36, UiStrings::turnLabel(state.turnSignals), 1, TEXT_PRIMARY);
}
