#include "ScreenSafeShutdown.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenSafeShutdown_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"SAFE STATE", "OUTPUT DISABLE", "SAVE & OFF", "REBOOT"};
  UiDrawListHeader(renderer, "SAFE SHUTDOWN");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_SAFE_SHUTDOWN];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    UiDrawListRow(renderer, row, labels[idx], "", idx == ctx.focus[SCREEN_SAFE_SHUTDOWN], ctx.editMode[SCREEN_SAFE_SHUTDOWN]);
  }
}
