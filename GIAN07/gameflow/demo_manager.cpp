/*
 *   DemoManager — centralized demo/replay system state
 */

#include "demo_manager.h"

// --- グローバルインスタンス ---
DemoManager Demos;

// --- 後方互換用参照ラッパー ---
// クロスモジュール参照（stage/ で使用）
bool& DemoplayLoadEnable = Demos.load_enable;
bool& DemoplaySaveAllEnable = Demos.save_all_enable;
bool& DemoplayLoadAllEnable = Demos.load_all_enable;
uint8_t& PlaybackMaxStage = Demos.playback_max_stage;
std::u8string& PendingReplayFile = Demos.pending_replay_file;
// MultiPlayInfo, DemoInfo, DemoBuffer → 各 .cpp は Demos.xxx を直接使用
