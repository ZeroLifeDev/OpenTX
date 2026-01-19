#include "ScreenAlerts.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenAlerts_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"TEMP ALERT", "VOLT ALERT", "VOLUME", "MUTE"};
  UiDrawListHeader(renderer, "ALERTS & BUZZER");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_ALERTS];
  char buf[8];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) value = UiStrings::onOffLabel(state.alertTemp);
    else if (idx == 1) value = UiStrings::onOffLabel(state.alertVoltage);
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%u", state.buzzerVolume); value = buf; }
    else if (idx == 3) value = UiStrings::onOffLabel(state.buzzerMute);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_ALERTS], ctx.editMode[SCREEN_ALERTS]);
  }
}
