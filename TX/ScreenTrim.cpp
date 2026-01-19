#include "ScreenTrim.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenTrim_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"STEER TRIM", "THR TRIM", "RESET", "PER PROFILE"};
  UiDrawListHeader(renderer, "TRIM MANAGEMENT");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_TRIM];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%d", state.steerTrim); value = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%d", state.throttleTrim); value = buf; }
    else if (idx == 2) value = "HOLD SET";
    else if (idx == 3) value = UiStrings::onOffLabel(state.trimPerProfile);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_TRIM], ctx.editMode[SCREEN_TRIM]);
  }
}
