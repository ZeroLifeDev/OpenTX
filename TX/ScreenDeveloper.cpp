#include "ScreenDeveloper.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenDeveloper_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"EXPERT MODE", "EXPERIMENT", "PERF OVERRIDE"};
  UiDrawListHeader(renderer, "DEVELOPER");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_DEVELOPER];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 3) break;
    const char *value = "";
    if (idx == 0) value = UiStrings::onOffLabel(state.expertMode);
    else if (idx == 1) value = UiStrings::onOffLabel(state.experimental);
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%u", state.perfOverride); value = buf; }
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_DEVELOPER], ctx.editMode[SCREEN_DEVELOPER]);
  }
}
