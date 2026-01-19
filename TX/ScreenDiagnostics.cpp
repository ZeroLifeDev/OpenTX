#include "ScreenDiagnostics.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenDiagnostics_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"SENSORS", "ADC CHECK", "GYRO VALID"};
  UiDrawListHeader(renderer, "DIAGNOSTICS");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_DIAGNOSTICS];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 3) break;
    const char *value = "";
    if (idx == 0) value = UiStrings::onOffLabel(state.sensorsHealthy);
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%u", state.adcSanity); value = buf; }
    else if (idx == 2) value = UiStrings::onOffLabel(state.gyroValid);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_DIAGNOSTICS], ctx.editMode[SCREEN_DIAGNOSTICS]);
  }
}
