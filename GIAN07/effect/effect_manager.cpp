/*
 *   EffectManager — centralized visual effects system state
 */

#include "effect_manager.h"

// --- グローバルインスタンス ---
EffectManager Effects;

// 後方互換用参照ラッパーは全て削除 — 各 .cpp は Effects.xxx を直接使用
