#include "ScreenLogging.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenLogging_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"PEAK TEMP", "MAX SPEED", "DRIVE TIME", "ERRORS"};
  UiDrawListHeader(renderer, "LOGGING");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_LOGGING];
  char buf[12];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%uC", state.peakTemp); value = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%u", state.maxSpeed); value = buf; }
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%lum", static_cast<unsigned long>(state.driveTimeSec / 60)); value = buf; }
    else if (idx == 3) { snprintf(buf, sizeof(buf), "%u", state.errorCount); value = buf; }
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_LOGGING], ctx.editMode[SCREEN_LOGGING]);
  }
}
