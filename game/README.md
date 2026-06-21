Core game functionality and all cross-platform backend code (SDL3, miniaudio, TinySoundFont). Only platform-specific rendering backends with no cross-platform equivalent are deferred to the [`platform` subdirectory](../platform/).

May include SDL3 and other cross-platform library abstractions, but must not directly use OS-specific APIs (Win32, X11, etc.).
