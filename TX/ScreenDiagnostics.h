#pragma once

#include "Renderer.h"
#include "UiState.h"

struct UiContext;

void ScreenDiagnostics_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx);
