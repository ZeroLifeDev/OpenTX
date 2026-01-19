#include "ScreenAccessoryControl.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenAccessoryControl_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"HEADLIGHTS", "TAILLIGHTS", "TURN SIG", "BRAKE", "AUX OUT"};
  UiDrawListHeader(renderer, "ACCESSORY CTRL");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_ACCESSORY_CTRL];

  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 5) break;
    const char *value = "";
    char buf[8];
    if (idx == 0) value = UiStrings::triStateLabel(state.headlights);
    else if (idx == 1) value = UiStrings::onOffLabel(state.taillights == ACC_ON);
    else if (idx == 2) value = UiStrings::turnLabel(state.turnSignals);
    else if (idx == 3) value = UiStrings::brakeLabel(state.brakeLights);
    else if (idx == 4) { snprintf(buf, sizeof(buf), "%u", state.auxOutput); value = buf; }
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_ACCESSORY_CTRL], false);
  }
  if (visible > 5 && ctx.confirmUntilMs > millis()) {
    renderer.drawText(6, UiLayout::ListY + (5 * UiLayout::ItemH) + 4, "APPLIED", 1, STATE_OK);
  }
}
