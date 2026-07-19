///
/// bullet_common — Shared utilities for bullet and laser systems
///

#pragma once

#include <cstdint>

namespace bullet_common {

inline constexpr auto kZSet = 0x08;

inline constexpr auto kCmdWay = 0x00;
inline constexpr auto kCmdAll = 0x01;
inline constexpr auto kCmdRnd = 0x02;
inline constexpr auto kCmdMask = 0x03;

// — Direction calculation ————————————————————

// Calculate spread direction for bullet/laser i of n.
// base_deg includes player-aim offset (ZSET) if applicable.
// cmd_type = kCmdWay/kCmdAll/kCmdRnd.
[[nodiscard]] uint8_t CalcSpreadDir(uint16_t i, uint8_t cmd_type, uint8_t n,
                                    uint8_t base_deg, uint8_t dw);

// — Difficulty scaling ——————————————————————

// Common: modifies n and dw based on difficulty level.
// Returns true if the command type was recognized.
bool ApplyEasyCountSpread(uint8_t cmd_type, uint8_t &n, uint8_t &dw);
bool ApplyHardCountSpread(uint8_t cmd_type, uint8_t &n, uint8_t &dw);
bool ApplyLunaticCountSpread(uint8_t cmd_type, uint8_t &n, uint8_t &dw);

// Reflect laser length scaling.
[[nodiscard]] int ScaleLengthEasy(int l);
[[nodiscard]] int ScaleLengthHard(int l);
[[nodiscard]] int ScaleLengthLunatic(int l);

// Bullet rapid-fire count scaling.
void ApplyEasyRapid(uint8_t &ns);
void ApplyHardRapid(uint8_t &ns);
void ApplyLunaticRapid(uint8_t &ns);

// Bullet velocity scaling by rank: (v/2)*rank/32 + v/2
// Only used when type == T_NORM.
[[nodiscard]] int ScaleVelocityByRank(int v, int rank);

} // namespace bullet_common
