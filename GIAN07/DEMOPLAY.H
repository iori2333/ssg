/*                                                                           */
/*   DemoPlay.h   デモプレイ処理                                             */
/*                                                                           */
/*                                                                           */

#ifndef PBGWIN_DEMOPLAY_H
#define PBGWIN_DEMOPLAY_H "DEMOPLAY : Version 0.02 : Update 2000/04/01"
// #pragma message(PBGWIN_DEMOPLAY_H)

#include "game/input.h"
#include <string>
#include <vector>

struct CONFIG_DATA;

///// [ 定数 ] /////
#define DEMOBUF_MAX (60 * 60 * 30) // ３０分のデータ格納ＯＫ
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
typedef struct tagDEMOPLAY_INFO {
  uint32_t RndSeed;            // 乱数のたね
  uint32_t FrameCount;         // Not data size! Including the terminating ESC.
  DEMOPLAY_CONFIG_DATA CfgDat; // コンフィグの情報(Load時に一部を参照する)
  uint8_t Exp;                 // 初期パワーアップ
  uint8_t Weapon;              // 初期装備
} DEMOPLAY_INFO;

// Multi-stage replay header
struct MULTI_REPLAY_INFO {
  uint32_t RndSeed;
  uint8_t StageCount;            // Number of stages recorded (1-6)
  DEMOPLAY_CONFIG_DATA CfgDat;   // 24 bytes
  uint8_t Exp;
  uint8_t Weapon;
  uint8_t Stages[REPLAY_STAGE_MAX];       // Stage numbers
  uint32_t FrameCounts[REPLAY_STAGE_MAX]; // Per-stage input frame count
};

///// [ 関数 ] /////
void DemoplayInit(void); // デモプレイデータの準備

// デモプレイデータを保存する
// Returns `true` if the replay is over and should be saved via either
// DemoplaySave() or ReplaySave().
bool DemoplayRecord(INPUT_BITS key);

void DemoplaySaveDemo(void);   // デモプレイデータをファイルに書き込む
void DemoplaySaveReplay(void); // リプレイデータをファイルに書き込む

bool DemoplayLoadDemo(int stage);   // デモプレイデータをロードする
bool DemoplayLoadReplay(int stage); // リプレイデータをロードする
INPUT_BITS DemoplayMove(void);      // Key_Data を返す
void DemoplayCleanup(void);         // デモプレイロードの事後処理

// Multi-stage replay
void DemoplayFlushStage(void);  // Save current stage's recorded frames
void DemoplaySaveReplayAll(bool exstg = false); // Write multi-stage replay file
bool DemoplayLoadReplayAll(const char8_t *fn);  // Load multi-stage replay file
INPUT_BITS DemoplayMoveAll(void);               // Get next frame for multi-stage playback
bool DemoplayHasRecordedStages(void);           // Returns true if at least one stage recorded

// Replay file scanning
std::u8string ReplayAllFN(bool exstg = false); // Generate replay filename with timestamp

///// [ 変数 ] /////
extern bool DemoplaySaveEnable;    // デモプレイのセーブが動作しているか(obsolete single-stage)
extern bool DemoplayLoadEnable;    // デモプレイのロードが動作しているか
extern bool DemoplaySaveAllEnable; // Multi-stage recording active
extern bool DemoplayLoadAllEnable; // Multi-stage playback active
extern MULTI_REPLAY_INFO MultiPlayInfo;   // Currently loaded multi-stage replay info
extern uint8_t PlaybackMaxStage;          // Max stage number in loaded replay
extern std::u8string PendingReplayFile;   // Deferred replay start from menu

#endif
