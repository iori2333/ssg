/*
 *   DemoManager — centralized demo/replay system state
 */

#include "demo_manager.h"

// --- グローバルインスタンス ---
DemoManager Demos;

// --- 後方互換用参照ラッパー ---
bool& DemoplayLoadEnable = Demos.load_enable;
bool& DemoplaySaveAllEnable = Demos.save_all_enable;
bool& DemoplayLoadAllEnable = Demos.load_all_enable;
MULTI_REPLAY_INFO& MultiPlayInfo = Demos.multi_play_info;
uint8_t& PlaybackMaxStage = Demos.playback_max_stage;
std::u8string& PendingReplayFile = Demos.pending_replay_file;
DEMOPLAY_INFO& DemoInfo = Demos.demo_info;
std::array<INPUT_BITS, DEMOBUF_MAX>& DemoBuffer = Demos.demo_buffer;
