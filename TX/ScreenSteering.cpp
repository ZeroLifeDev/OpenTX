#include "ScreenSteering.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenSteering_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"DEADZONE", "CENTER", "EXPO", "ENDPTS"};
  UiDrawListHeader(renderer, "STEERING SETUP");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_STEERING];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%u", state.steerDeadzone); value = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%d", state.steerCenter); value = buf; }
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%u", state.steerExpo); value = buf; }
    else if (idx == 3) { snprintf(buf, sizeof(buf), "%u", state.steerEndpoint); value = buf; }
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_STEERING], ctx.editMode[SCREEN_STEERING]);
  }
}
