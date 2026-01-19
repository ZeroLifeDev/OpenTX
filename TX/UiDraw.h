#pragma once

#include "Renderer.h"
#include "Ui.h"
#include "UiLayout.h"
#include "UiState.h"

void UiDrawOverlay(Renderer &renderer, const UiState &state);
void UiDrawStatusBar(Renderer &renderer, const UiState &state);
void UiDrawTempStrip(Renderer &renderer, const UiState &state);
void UiDrawListHeader(Renderer &renderer, const char *title);
void UiDrawListRow(Renderer &renderer, int row, const char *label, const char *value, bool focused, bool editing);
void UiDrawContextMenu(Renderer &renderer, const UiContext &ctx);
