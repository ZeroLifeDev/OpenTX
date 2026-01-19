#pragma once

#include "InputManager.h"
#include "Renderer.h"
#include "UiState.h"

struct UiContext;

void ScreenCalibration_Draw(Renderer &renderer, const UiState &state, const UiContext &ctx);
void ScreenCalibration_HandleInput(const InputActions &actions, UiState &state, UiContext &ctx);
