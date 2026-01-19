#include "ScreenProfileSelect.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenProfileSelect_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"PROFILE 1", "PROFILE 2", "PROFILE 3", "PROFILE 4"};
  UiDrawListHeader(renderer, "PROFILE SELECT");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_PROFILE_SELECT];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    snprintf(buf, sizeof(buf), "%s", idx == state.activeProfile ? "ACTIVE" : "");
    UiDrawListRow(renderer, row, labels[idx], buf, idx == ctx.focus[SCREEN_PROFILE_SELECT], ctx.editMode[SCREEN_PROFILE_SELECT]);
  }
}
