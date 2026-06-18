///
/// Keyboard and pad input interface
///

#pragma once

#include "game/input.h"
#include <optional>

// Function prototype declarations
bool Key_Start(void); // Start key input
void Key_End(void);   // End key input

void Key_Read(void);

// Returns:
// • ≥1: ID of the single gamepad button that is being pressed
// •  0: More than one gamepad button is being pressed
// • std::nullopt: No gamepad button is being pressed
std::optional<INPUT_PAD_BUTTON> Key_PadSingle(void);
