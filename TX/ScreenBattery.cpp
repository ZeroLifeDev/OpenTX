#include "ScreenBattery.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenBattery_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  UiDrawListHeader(renderer, "BATTERY & POWER");
  char buf[10];
  if (state.rxVoltageAvailable) {
    snprintf(buf, sizeof(buf), "R%.1f", state.rxVoltage);
    renderer.drawTextRight(86, UiLayout::ContentY + 3, buf, 1, TEXT_MUTED);
  }
  snprintf(buf, sizeof(buf), "T%.1f", state.txVoltage);
  renderer.drawTextRight(124, UiLayout::ContentY + 3, buf, 1, TEXT_PRIMARY);

  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_BATTERY];
  static const char *labels[] = {"TX WARN", "RX WARN", "SAG DETECT"};
  char val[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 3) break;
    const char *value = "";
    if (idx == 0) { snprintf(val, sizeof(val), "%.1f", state.txVoltageWarn); value = val; }
    else if (idx == 1) { snprintf(val, sizeof(val), "%.1f", state.rxVoltageWarn); value = val; }
    else if (idx == 2) value = UiStrings::onOffLabel(state.voltageSagDetect);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_BATTERY], ctx.editMode[SCREEN_BATTERY]);
  }
}
