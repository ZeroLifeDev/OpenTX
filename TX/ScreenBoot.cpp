#include "ScreenBoot.h"

#include "UiLayout.h"

void ScreenBoot_Draw(Renderer &renderer) {
  renderer.clear(BG_PRIMARY);
  renderer.drawTextCentered(34, "OPENTX", 2, TEXT_PRIMARY);
  renderer.drawTextCentered(62, "FIRMWARE V1.0", 1, TEXT_MUTED);
  renderer.drawTextCentered(82, "HARDWARE CHECK", 1, TEXT_MUTED);
  renderer.drawTextCentered(98, "DMA READY", 1, TEXT_MUTED);
}
