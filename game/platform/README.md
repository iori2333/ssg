Code that genuinely differs between platforms. Currently only contains the text rendering backend and its OS-specific implementations (GDI on Windows, PangoCairo on Linux).

All cross-platform abstractions (SDL3, miniaudio, TinySoundFont) live in [`game/`](../game/).
