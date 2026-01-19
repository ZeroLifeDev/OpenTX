#include "ScreenSystem.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenSystem_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"BRIGHTNESS", "SLEEP", "LED", "BOOT"};
  UiDrawListHeader(renderer, "SYSTEM");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_SYSTEM];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%u", state.displayBrightness); value = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%us", state.sleepTimeoutSec); value = buf; }
    else if (idx == 2) value = UiStrings::ledModeLabel(state.ledMode);
    else if (idx == 3) value = UiStrings::bootModeLabel(state.bootMode);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_SYSTEM], ctx.editMode[SCREEN_SYSTEM]);
  }
}
