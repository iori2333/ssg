/*
 *   SCL Stage Command executor — extracted from SCROLL.cpp
 *
 *   Processes SCL (Stage Control Language) bytecodes for enemy spawning,
 *   boss encounters, BGM changes, messages, and stage flow control.
 */

#pragma once

#include <cstdint>

// Execute SCL commands starting from *scl_now until a blocking command
// (SCL_KEY, SCL_TIME, SCL_WAITEX) is reached. Returns true if all commands
// were processed without blocking.
bool SclExecute(uint8_t *&scl_now);
