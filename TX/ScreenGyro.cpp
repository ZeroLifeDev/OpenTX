#include "ScreenGyro.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenGyro_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"GYRO", "GAIN", "MODE", "RESPONSE"};
  UiDrawListHeader(renderer, "GYRO CONTROL");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_GYRO];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) value = UiStrings::onOffLabel(state.gyroOn);
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%u", state.gyroGain); value = buf; }
    else if (idx == 2) value = UiStrings::gyroModeLabel(state.gyroMode);
    else if (idx == 3) { snprintf(buf, sizeof(buf), "%u", state.gyroResponse); value = buf; }
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_GYRO], ctx.editMode[SCREEN_GYRO]);
  }
  if (visible > 4) {
    int16_t barY = UiLayout::ListY + (4 * UiLayout::ItemH) + 4;
    renderer.drawText(6, barY, "RESP", 1, TEXT_MUTED);
    renderer.drawHBar(44, barY + 2, 74, 8, state.gyroResponse, ACCENT_CYAN, BG_PANEL);
  }
}
