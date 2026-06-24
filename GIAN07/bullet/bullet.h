///
/// bullet.h - Legacy shim redirecting to the new `bullets::` namespace.
///
/// Non-bullet callers continue to write `Bullet`, `BulletCommand`, etc.
/// unqualified.  This header pulls the new types out of `bullet_data.h`
/// and exposes them with `using` declarations.
///

#pragma once

#include "bullet/bullet_data.h"

// Unqualified legacy aliases.
using Bullet = bullets::Bullet;
using BulletCommand = bullets::BulletCommand;

// Bullet constants — too numerous to re-export individually; expose the
// entire `bullets::` enum namespace via using-directive so callers can
// keep referencing TAMA_MAX / T_NORM / TC_WAY etc. unqualified.
using namespace bullets;

// Free-function helpers relocated to bullets::; expose identical names.
using bullets::GetBulletEvadeRadius;
using bullets::GetBulletHitRadius;