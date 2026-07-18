///
/// Application lifecycle — init, cleanup, main loop
///
#pragma once

[[nodiscard]] bool XInit();
void XCleanup();
[[nodiscard]] bool GameFrame();
