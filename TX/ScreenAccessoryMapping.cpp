#include "ScreenAccessoryMapping.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenAccessoryMapping_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"HEADLIGHTS", "TAILLIGHTS", "TURN SIG", "BRAKE", "AUX OUT"};
  UiDrawListHeader(renderer, "ACCESSORY MAP");
  bool conflict[5] = {false, false, false, false, false};
  for (int i = 0; i < 5; ++i) {
    for (int j = i + 1; j < 5; ++j) {
      if (state.accessoryMap[i] == state.accessoryMap[j] && state.accessoryMap[i] != 5) {
        conflict[i] = true;
        conflict[j] = true;
      }
    }
  }

  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_ACCESSORY_MAP];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 5) break;
    const char *value = "";
    char buf[10];
    const char *base = UiStrings::mapLabel(state.accessoryMap[idx]);
    if (conflict[idx]) {
      snprintf(buf, sizeof(buf), "%s!", base);
      value = buf;
    } else {
      value = base;
    }
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_ACCESSORY_MAP], ctx.editMode[SCREEN_ACCESSORY_MAP]);
  }
  if (visible > 5) {
    renderer.drawText(6, UiLayout::ListY + (5 * UiLayout::ItemH) + 4, "PRIO M>S>T>G", 1, TEXT_MUTED);
  }
}
