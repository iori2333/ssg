///
/// SfxManager — sound effect loading
///
#pragma once

#include <cstdint>

#include "core/constants.h"

class PackFile;

class SfxManager {
public:
  bool LoadAll();

private:
  friend class PackManager;
  static bool LoadAllFromPack(const PackFile &in);
};

inline SfxManager sfx;
