#include "ScreenPerformance.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenPerformance_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"LOOP US", "FPS", "CPU", "MEM"};
  UiDrawListHeader(renderer, "PERFORMANCE");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_PERF];
  char buf[12];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(state.loopTimeUs)); value = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%u", state.fps); value = buf; }
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%u%%", state.cpuLoad); value = buf; }
    else if (idx == 3) { snprintf(buf, sizeof(buf), "%lu", static_cast<unsigned long>(state.memFree)); value = buf; }
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_PERF], ctx.editMode[SCREEN_PERF]);
  }
}
