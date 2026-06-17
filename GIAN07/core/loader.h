/*                                                                           */
/*   LOADER.h   グラフィック、サウンド等のロード                             */
/*                                                                           */
/*                                                                           */

#pragma once

#include "constants.h"
#include "game/coords.h"
#include "game/graphics.h"
#include "game/hash.h"

struct SURFACE_DDRAW;

///// [ 定数 ] /////
inline constexpr auto FACE_NUMX = 6; // 顔グラの横の個数

// 特殊グラフィックID (For LoadGraph() ) //
inline constexpr auto GRAPH_ID_MUSICROOM =
    128; // 音楽室用ＢＭＰのＩＤ(数値は1-6 で無ければ良い)
inline constexpr auto GRAPH_ID_TITLE = (128 + 1); // タイトル画面のＢＭＰのＩＤ
inline constexpr auto GRAPH_ID_NAMEREGIST =
    (128 + 2); // お名前登録画面のＢＭＰのＩＤ
inline constexpr auto GRAPH_ID_EXSTAGE =
    (128 + 3); // エキストラステージ・システム
inline constexpr auto GRAPH_ID_EXBOSS1 =
    (128 + 4); // エキストラステージ・ボス１
inline constexpr auto GRAPH_ID_EXBOSS2 =
    (128 + 5); // エキストラステージ・ボス２
inline constexpr auto GRAPH_ID_SPROJECT = (128 + 6); // 西方Ｐｒｏｊｅｃｔの表示
inline constexpr auto GRAPH_ID_ENDING = (128 + 7); // エンディングのロードを行う

// サウンド(効果音番号) //
inline constexpr auto SOUND_ID_KEBARI = 0x00;
inline constexpr auto SOUND_ID_TAME = 0x01;
inline constexpr auto SOUND_ID_LASER = 0x02;
inline constexpr auto SOUND_ID_LASER2 = 0x03;
inline constexpr auto SOUND_ID_BOMB = 0x04;
inline constexpr auto SOUND_ID_SELECT = 0x05;
inline constexpr auto SOUND_ID_HIT = 0x06;
inline constexpr auto SOUND_ID_CANCEL = 0x07;
inline constexpr auto SOUND_ID_WARNING = 0x08;
inline constexpr auto SOUND_ID_SBLASER = 0x09;
inline constexpr auto SOUND_ID_BUZZ = 0x0a;
inline constexpr auto SOUND_ID_MISSILE = 0x0b;
inline constexpr auto SOUND_ID_JOINT = 0x0c;
inline constexpr auto SOUND_ID_DEAD = 0x0d;
inline constexpr auto SOUND_ID_SBBOMB = 0x0e;
inline constexpr auto SOUND_ID_BOSSBOMB = 0x0f;
inline constexpr auto SOUND_ID_ENEMYSHOT = 0x10;
inline constexpr auto SOUND_ID_HLASER = 0x11;
inline constexpr auto SOUND_ID_TAMEFAST = 0x12;
inline constexpr auto SOUND_ID_WARP = 0x13;

// サウンド(最大数) //
inline constexpr auto SNDMAX_KEBARI = 5;
inline constexpr auto SNDMAX_TAME = 5;
inline constexpr auto SNDMAX_LASER = 1;
inline constexpr auto SNDMAX_LASER2 = 1;
inline constexpr auto SNDMAX_BOMB = 1; // 5
inline constexpr auto SNDMAX_SELECT = 1;
inline constexpr auto SNDMAX_HIT = 1; // 5
inline constexpr auto SNDMAX_CANCEL = 1;
inline constexpr auto SNDMAX_WARNING = 1;
inline constexpr auto SNDMAX_SBLASER = 1;
inline constexpr auto SNDMAX_BUZZ = 2; // 2
inline constexpr auto SNDMAX_MISSILE = 5;
inline constexpr auto SNDMAX_JOINT = 1;
inline constexpr auto SNDMAX_DEAD = 1;
inline constexpr auto SNDMAX_SBBOMB = 1;
inline constexpr auto SNDMAX_BOSSBOMB = 1;
inline constexpr auto SNDMAX_ENEMYSHOT = 5;
inline constexpr auto SNDMAX_HLASER = 1;
inline constexpr auto SNDMAX_TAMEFAST = 5;
inline constexpr auto SNDMAX_WARP = 1;

struct FaceData {
  PALETTE pal; // 顔グラ用パレット
};
// (FACE_DATA alias removed — use FaceData directly)

// エンディングのグラフィック管理用 //
struct EndingGrp {
  PIXEL_LTRB rcTarget; // 矩形の範囲
  PALETTE pal;         // パレット
};
// (ENDING_GRP alias removed — use EndingGrp directly)

///// [ 関数 ] /////
void LoaderInit();
void LoaderCleanup();
[[nodiscard]] bool LoadStageData(
    uint8_t stage);        // ＥＣＬ&ＳＣＬデータ列をメモリ上にロードする
[[nodiscard]] bool LoadGraph(int stage); // あるステージのグラフィックをロードする
[[nodiscard]] bool LoadFace(uint8_t FaceID, uint8_t FileNo); // 顔グラフィックをロードする
[[nodiscard]] bool LoadMusic(unsigned int id);               // ｎ番目の曲をロードする
[[nodiscard]] bool LoadMusicByHash(const HASH &hash);
[[nodiscard]] bool LoadMIDIBuffer(BYTE_BUFFER_OWNED);
bool LoadSound(void); // 全ての Sound データをロードする

// MusicRoom のコメントをロードする
BYTE_BUFFER_OWNED LoadMusicRoomComment(int no);

BYTE_BUFFER_OWNED LoadDemo(int stage);

void LoadPaletteFromEnemy(); // 敵のパレットにする

// Reloads the last stage loaded with LoadGraph().
void ReloadGraph();

//// [ 変数 ] ////

// 顔グラ用
extern const std::reference_wrapper<SURFACE_DDRAW> GrFaces[FACE_MAX];

extern const std::reference_wrapper<SURFACE_DDRAW> GrEndingPic[ENDING_PIC_MAX];

extern FaceData face_data[FACE_MAX]; // 顔グラ用

extern uint32_t MusicNum; // 曲数

extern PALETTE SProjectPalette;

extern EndingGrp ending_pic[ENDING_PIC_MAX];
