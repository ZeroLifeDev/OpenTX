#include "ScreenThrottle.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenThrottle_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"DEADZONE", "CENTER", "CURVE", "BRAKE", "REVERSE"};
  UiDrawListHeader(renderer, "THROTTLE SETUP");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_THROTTLE];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 5) break;
    const char *value = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%u", state.throttleDeadzone); value = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%d", state.throttleCenter); value = buf; }
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%u", state.throttleCurve); value = buf; }
    else if (idx == 3) { snprintf(buf, sizeof(buf), "%u", state.brakeStrength); value = buf; }
    else if (idx == 4) value = UiStrings::onOffLabel(state.reverseLogic);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_THROTTLE], ctx.editMode[SCREEN_THROTTLE]);
  }
}
