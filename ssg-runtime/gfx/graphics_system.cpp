///
/// The active graphics system instance.
///

#include "graphics_system.h"

namespace gfx {

GraphicsSystem &ActiveGraphics() {
  static GraphicsSystem instance;
  return instance;
}

} // namespace gfx
