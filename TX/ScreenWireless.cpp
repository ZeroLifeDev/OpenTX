#include "ScreenWireless.h"

#include "UiDraw.h"
#include "UiLayout.h"
#include "UiStrings.h"

void ScreenWireless_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  static const char *labels[] = {"LINK QUALITY", "PACKET LOSS", "UPDATE RATE", "RECONNECT"};
  UiDrawListHeader(renderer, "WIRELESS");
  int visible = UiLayout::ListH / UiLayout::ItemH;
  int start = ctx.scroll[SCREEN_WIRELESS];
  char buf[12];
  for (int row = 0; row < visible; ++row) {
    int idx = start + row;
    if (idx >= 4) break;
    const char *value = "";
    if (idx == 0) { snprintf(buf, sizeof(buf), "%u%%", state.linkQuality); value = buf; }
    else if (idx == 1) { snprintf(buf, sizeof(buf), "%u%%", state.packetLoss); value = buf; }
    else if (idx == 2) { snprintf(buf, sizeof(buf), "%uHZ", state.updateRate); value = buf; }
    else if (idx == 3) value = UiStrings::onOffLabel(state.reconnectEnabled);
    UiDrawListRow(renderer, row, labels[idx], value, idx == ctx.focus[SCREEN_WIRELESS], ctx.editMode[SCREEN_WIRELESS]);
  }
}
