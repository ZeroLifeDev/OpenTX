#include "ScreenCalibration.h"

#include "UiLayout.h"

void ScreenCalibration_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx) {
  renderer.fillRect(0, UiLayout::ContentY, UiLayout::ScreenW, UiLayout::ContentH, BG_PANEL);
  renderer.drawRect(0, UiLayout::ContentY, UiLayout::ScreenW, UiLayout::ContentH, GRID_LINE);
  renderer.drawText(6, UiLayout::ContentY + 4, "CALIBRATION", 1, TEXT_PRIMARY);
  char buf[12];
  snprintf(buf, sizeof(buf), "STEP %u/3", state.wizardStep + 1);
  renderer.drawText(6, UiLayout::ContentY + 20, buf, 1, TEXT_MUTED);

  const char *msg = "CENTER STEER";
  if (state.wizardStep == 1) msg = "CENTER THR";
  else if (state.wizardStep == 2) msg = "MOVE TO ENDS";
  else if (state.wizardStep == 3) msg = "SAVE VALUES";
  renderer.drawText(6, UiLayout::ContentY + 38, msg, 1, TEXT_PRIMARY);
  renderer.drawText(6, UiLayout::ContentY + 54, "SET TO ADVANCE", 1, TEXT_MUTED);
}

void ScreenCalibration_HandleInput(const InputActions &actions, UiState &state, UiContext &ctx) {
  if (actions.setShort) {
    state.wizardStep = (state.wizardStep + 1) % 4;
  }
}
