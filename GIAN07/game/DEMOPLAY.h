/*                                                                           */
/*   DemoPlay.h   デモプレイ処理                                             */
/*                                                                           */
/*                                                                           */

#pragma once

#include <string>
#include <vector>
#include "game/input.h"

struct CONFIG_DATA;

///// [ 定数 ] /////
inline constexpr auto DEMOBUF_MAX = (60 * 60 * 30); // ３０分のデータ格納ＯＫ
static constexpr auto REPLAY_STAGE_MAX = 6;

///// Replay-specific config option subset /////
// The original code simply reused CONFIG_DATA, which we can't do in this fork
// due to the additional fields we add to the structure.
struct DEMOPLAY_CONFIG_DATA {
  uint8_t GameLevel;
  uint8_t PlayerStock;
  uint8_t BombStock;
  uint8_t Padding1[5] = {0};
  uint8_t InputFlags;
  uint8_t Padding2[15] = {0};
};
static_assert(sizeof(DEMOPLAY_CONFIG_DATA) == 24);

///// [構造体] /////
struct DemoPlayState {
  uint32_t RndSeed;            // 乱数のたね
  uint32_t FrameCount;         // Not data size! Including the terminating ESC.
  DEMOPLAY_CONFIG_DATA CfgDat; // コンフィグの情報(Load時に一部を参照する)
  uint8_t Exp;                 // 初期パワーアップ
  uint8_t Weapon;              // 初期装備
};
using DEMOPLAY_INFO = DemoPlayState;

// Multi-stage replay header
struct MULTI_REPLAY_INFO {
  uint32_t RndSeed;
  uint8_t StageCount;          // Number of stages recorded (1-6)
  DEMOPLAY_CONFIG_DATA CfgDat; // 24 bytes
  uint8_t Exp;
  uint8_t Weapon;
  uint8_t Stages[REPLAY_STAGE_MAX];       // Stage numbers
  uint32_t FrameCounts[REPLAY_STAGE_MAX]; // Per-stage input frame count
};

///// [ 関数 ] /////
void DemoplayInit(void); // デモプレイデータの準備

// デモプレイデータを保存する
bool DemoplayRecord(INPUT_BITS key);

void DemoplaySaveDemo(void); // デモプレイデータをファイルに書き込む

bool DemoplayLoadDemo(int stage); // デモプレイデータをロードする
INPUT_BITS DemoplayMove(void);    // Key_Data を返す
void DemoplayCleanup(void);       // デモプレイロードの事後処理

// Multi-stage replay
void DemoplayFlushStage(void);
void DemoplaySaveReplayAll(bool exstg = false);
bool DemoplayLoadReplayAll(const char8_t *fn);
bool DemoplayHasRecordedStages(void);

///// [ 変数 ] /////
extern bool DemoplayLoadEnable;    // リプレイのロードが動作しているか
extern bool DemoplaySaveAllEnable; // Multi-stage recording active
extern bool DemoplayLoadAllEnable; // Multi-stage playback active
extern MULTI_REPLAY_INFO MultiPlayInfo;
extern uint8_t PlaybackMaxStage;
extern std::u8string PendingReplayFile;
