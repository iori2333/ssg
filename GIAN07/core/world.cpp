///
/// world - GameWorld instance definition.
///

#include "world.h"

GameWorld &gWorld() {
  static GameWorld instance;
  return instance;
}