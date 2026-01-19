#include "ScreenSuspension.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenSuspension_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"MODE", "POT MAP", "SPEED LOGIC", "PRESET"};
  UiDrawListHeader(renderer, "SUSPENSION");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_SUSPENSION];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) value = UiStrings::suspModeLabel(state.suspensionMode);
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%u", state.suspensionPotMap); value = buf; }
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%u", state.suspensionSpeedLogic); value = buf; }
    else if (idx == 3) value = UiStrings::suspPresetLabel(state.suspensionPreset);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_SUSPENSION], ctx.editMode[SCREEN_SUSPENSION]);
  }
}
