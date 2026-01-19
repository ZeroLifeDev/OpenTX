#include "ScreenTelemetry.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenTelemetry_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  UiDrawListHeader(renderer, "EXT TELEMETRY");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_TELEMETRY];
  char buf[12];

  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 7) break;
    const char *label = "";
    const char *value = "";
    if (idx == 0) {
      label = "MOTOR TEMP";
      snprintf(buf, sizeof(buf), "%dC", static_cast<int>(state.tempMotor));
      value = buf;
    } else if (idx == 1) {
      label = "ESC TEMP";
      snprintf(buf, sizeof(buf), "%dC", static_cast<int>(state.tempEsc));
      value = buf;
    } else if (idx == 2) {
      label = "BOARD TEMP";
      snprintf(buf, sizeof(buf), "%dC", static_cast<int>(state.tempBoard));
      value = buf;
    } else if (idx == 3) {
      label = "RPM EST";
      snprintf(buf, sizeof(buf), "%d", static_cast<int>(state.rpmEstimate));
      value = buf;
    } else if (idx == 4) {
      label = "CURRENT";
      snprintf(buf, sizeof(buf), "%dA", static_cast<int>(state.currentA));
      value = buf;
    } else if (idx == 5) {
      label = "SIGNAL";
      snprintf(buf, sizeof(buf), "%d%%", static_cast<int>(state.signalStrength));
      value = buf;
    } else if (idx == 6) {
      label = "LATENCY";
      snprintf(buf, sizeof(buf), "%dMS", static_cast<int>(state.latencyMs));
      value = buf;
    }
    bool focused = (idx == ctx.focus[SCREEN_TELEMETRY]);
    UiDrawListRow(renderer, row, label, value, focused, ctx.editMode[SCREEN_TELEMETRY]);
  }
}
