///
/// SfxManager — sound effect loading + SfxId enum
///
#pragma once

#include <cstdint>

enum class SfxId : uint8_t {
  Kebari    = 0x00, Tame      = 0x01, Laser     = 0x02, Laser2    = 0x03,
  Bomb      = 0x04, Select    = 0x05, Hit       = 0x06, Cancel    = 0x07,
  Warning   = 0x08, Sblaser   = 0x09, Buzz      = 0x0a, Missile   = 0x0b,
  Joint     = 0x0c, Dead      = 0x0d, Sbbomb    = 0x0e, Bossbomb  = 0x0f,
  Enemyshot = 0x10, Hlaser    = 0x11, Tamefast  = 0x12, Warp      = 0x13,
};

struct PACKFILE_READ;

class SfxManager {
public:
  bool LoadAll();

private:
  friend class PackManager;
  static bool LoadAllFromPack(const PACKFILE_READ &in);
};

inline SfxManager sfx;
