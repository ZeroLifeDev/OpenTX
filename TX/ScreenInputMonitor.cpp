#include "ScreenInputMonitor.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenInputMonitor_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  UiDrawListHeader(renderer, "INPUT MONITOR");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_INPUT_MON];
  char buf[20];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 6) break;
    const char *label = "";
    const char *value = "";
    if (idx == 0) { label = "RAW STEER"; snprintf(buf, sizeof(buf), "%u", state.rawSteer); value = buf; }
    else if (idx == 1) { label = "RAW THR"; snprintf(buf, sizeof(buf), "%u", state.rawThrottle); value = buf; }
    else if (idx == 2) { label = "RAW SUSP"; snprintf(buf, sizeof(buf), "%u", state.rawSuspension); value = buf; }
    else if (idx == 3) { label = "STEER %"; snprintf(buf, sizeof(buf), "%d", static_cast<int>(state.steerPct)); value = buf; }
    else if (idx == 4) { label = "THR %"; snprintf(buf, sizeof(buf), "%d", static_cast<int>(state.throttlePct)); value = buf; }
    else if (idx == 5) {
      label = "BUTTONS";
      snprintf(buf, sizeof(buf), "M%d S%d +%d -%d G%d",
        state.btnMenu ? 1 : 0,
        state.btnSet ? 1 : 0,
        state.btnTrimPlus ? 1 : 0,
        state.btnTrimMinus ? 1 : 0,
        state.gyroOn ? 1 : 0);
      value = buf;
    }
    UiDrawListRow(renderer, row, label, value, idx == ctx.focus[SCREEN_INPUT_MON], ctx.editMode[SCREEN_INPUT_MON]);
  }
}
