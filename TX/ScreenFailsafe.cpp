#include "ScreenFailsafe.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenFailsafe_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"SIGNAL LOSS", "THROTTLE CUT", "STEER MODE", "ALERT"};
  UiDrawListHeader(renderer, "FAILSAFE");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_FAILSAFE];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) value = UiStrings::failsafeLabel(state.failsafeMode);
    else if (idx == 1) value = UiStrings::onOffLabel(state.throttleCut);
    else if (idx == 2) value = state.steeringCenter ? "CENTER" : "HOLD";
    else if (idx == 3) value = UiStrings::onOffLabel(state.alertSignal);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_FAILSAFE], ctx.editMode[SCREEN_FAILSAFE]);
  }
}
