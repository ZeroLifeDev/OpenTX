#include "ScreenProfileEdit.h"

#include "UiDraw.h"
#include "UiLayout.h"

void ScreenProfileEdit_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"RENAME", "DUPLICATE", "DELETE", "TUNE"};
  UiDrawListHeader(renderer, "PROFILE EDIT");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_PROFILE_EDIT];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    UiDrawListRow(renderer, row, labels[idx], "", idx == ctx.focus[SCREEN_PROFILE_EDIT], ctx.editMode[SCREEN_PROFILE_EDIT]);
  }
}
