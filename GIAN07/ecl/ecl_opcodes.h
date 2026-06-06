/*
 *   ECL opcode and register constants — type-safe enum classes
 *
 *   Replaces the old #define-style constants from ECL.h with C++23 enum
 *   classes for improved type safety and IDE support.
 */

#pragma once

#include <cstdint>

// ============================================================
// ECL opcodes — the bytecode instruction set
// ============================================================
enum class EclOp : uint8_t {
  // 0x0? : 制御用コマンド
  SETUP = 0x00, // 敵データ初期化
  END = 0x01,   // 敵強制消滅
  JMP = 0x02,   // 強制ジャンプ
  LOOP = 0x03,  // ループ(２重は不可)
  CALL = 0x04,  // サブルーチンを呼ぶ
  RET = 0x05,   // サブルーチンから復帰する
  JHPL = 0x06,  // ＨＰが指定値より大きければジャンプ
  JHPS = 0x07,  // ＨＰが指定値より小さければジャンプ
  JDIF = 0x08,  // 難易度によるswitch
  JDSB = 0x09,  // 進行角度がサボテンと一致したらジャンプ
  JFCL = 0x0A,  // フレームカウンタが大きければジャンプ
  JFCS = 0x0B,  // フレームカウンタが小さければジャンプ
  STI = 0x0C,   // 割り込みベクタをセットする
  CLI = 0x0D,   // 割り込みを無効にする

  // 0x1? : 移動用コマンド
  NOP = 0x10,    // 何もしない
  NOPSC = 0x11,  // スクロールに流される
  MOV = 0x12,    // 移動する
  ROL = 0x13,    // 回転移動
  LROL = 0x14,   // 直進＆回転移動
  WAVX = 0x15,   // 波移動Ｘ
  WAVY = 0x16,   // 波移動Ｙ
  MXA = 0x17,    // Ｘ絶対移動
  MYA = 0x18,    // Ｙ絶対移動
  MXYA = 0x19,   // ＸＹ絶対移動
  MXS = 0x1A,    // Ｘサボテンセット移動
  MYS = 0x1B,    // Ｙサボテンセット移動
  MXYS = 0x1C,   // ＸＹサボテンセット移動
  ACC = 0x1D,    // 加速or減速つき移動
  ACCXYA = 0x1E, // 減速付きＸＹ絶対セット
  GRAX = 0x1F,   // 重力付きＸ反射移動

  // 0x2? : 数値セット用コマンド
  DEGA = 0x20,  // 角度絶対セット
  DEGR = 0x21,  // 角度相対セット
  DEGX = 0x22,  // 角度ランダムセット
  DEGS = 0x23,  // 角度サボテンセット
  SPDA = 0x24,  // 速度絶対セット
  SPDR = 0x25,  // 速度相対セット
  XYA = 0x26,   // 座標絶対セット
  XYR = 0x27,   // 座標相対セット
  DEGXU = 0x28, // 角度ランダムセット(上)
  DEGXD = 0x29, // 角度ランダムセット(下)
  DEGEX = 0x2A, // 角度特殊セット
  XYS = 0x2B,   // 座標サボテンセット
  DEGX2 = 0x2C, // 制限付き角度ランダム
  XYRND = 0x2D, // 制限付き座標ランダム
  XYL = 0x2E,   // 長さ指定座標相対

  // 0x4? : 弾発射用コマンド
  TAMA = 0x40,   // 弾発射
  TAUTO = 0x41,  // 弾発射間隔をセットする
  TXYR = 0x42,   // 弾発射位置の相対ずらし
  TCMD = 0x43,   // 弾コマンド
  TDEGA = 0x44,  // 弾発射角絶対指定
  TDEGR = 0x45,  // 弾発射角相対指定
  TNUMA = 0x46,  // 弾数絶対指定
  TNUMR = 0x47,  // 弾数相対指定
  TSPDA = 0x48,  // 弾初速度絶対指定
  TSPDR = 0x49,  // 弾初速度相対指定
  TOPT = 0x4a,   // 弾オプション指定
  TTYPE = 0x4b,  // 弾の種類指定
  TCOL = 0x4c,   // 弾の色または形状指定
  TVDEG = 0x4d,  // 弾の角速度指定
  TREP = 0x4e,   // 弾の繰り返し用
  TDEGS = 0x4f,  // 弾発射角サボテンセット
  TDEGE = 0x50,  // 弾発射角を自分の向きにセット
  TAMA2 = 0x51,  // 難易度変化なし弾発射
  TCLR = 0x52,   // 全ての弾を消去する
  TAMAL = 0x53,  // 弾をライン状に発射する
  T2ITEM = 0x54, // 弾の何割かをアイテム化する
  TAMAEX = 0x55, // エキストラボス用弾幕発射コマンド

  // 0x6? : レーザー発射用コマンド
  LASER = 0x60,  // レーザー発射
  LCMD = 0x61,   // レーザーコマンド
  LLA = 0x62,    // レーザー長・絶対指定
  LLR = 0x63,    // レーザー長・相対指定
  LL2 = 0x64,    // レーザー発射位置
  LDEGA = 0x65,  // レーザー発射角絶対指定
  LDEGR = 0x66,  // レーザー発射角相対指定
  LNUMA = 0x67,  // レーザーの本数絶対指定
  LNUMR = 0x68,  // レーザーの本数相対指定
  LSPDA = 0x69,  // レーザーの速さ絶対指定
  LSPDR = 0x6a,  // レーザーの速さ相対指定
  LCOL = 0x6b,   // レーザーの色
  LTYPE = 0x6c,  // レーザーの種類
  LWA = 0x6d,    // レーザーの太さ絶対指定
  LDEGS = 0x6e,  // レーザー発射角サボテンセット
  LDEGE = 0x6f,  // レーザー発射角を自分の向きにセット
  LXY = 0x70,    // レーザーの発射座標セット
  LASER2 = 0x71, // レーザー発射

  // 0x8? : 太レーザー&ホーミング発射用コマンド
  LLSET = 0x80,    // 太レーザーセット
  LLOPEN = 0x81,   // 太レーザーオープン
  LLCLOSE = 0x82,  // 太レーザークローズ
  LLCLOSEL = 0x83, // 太レーザーライン状態へ
  LLDEGR = 0x84,   // 太レーザー角度相対変更
  HLASER = 0x85,   // ホーミングレーザー発動

  // 0x9? : フラグセット用コマンド
  DRAW_ON = 0x90,    // 描画する
  DRAW_OFF = 0x91,   // 描画しない
  CLIP_ON = 0x92,    // 画面外に出ても消さない
  CLIP_OFF = 0x93,   // 画面外に出たら消す
  DAMAGE_ON = 0x94,  // 無敵にする
  DAMAGE_OFF = 0x95, // 無敵にしない
  HITSB_ON = 0x96,   // 自機に当たる
  HITSB_OFF = 0x97,  // 自機に当たらない
  RLCHG_ON = 0x98,   // 左右反転を有効にする
  RLCHG_OFF = 0x99,  // 左右反転を無効にする

  // 0xA? : 特殊コマンド
  ANM = 0xA0,       // アニメーションを変更する
  PSE = 0xA1,       // 効果音を鳴らす
  INT = 0xA2,       // ボス用割り込みを発生させる
  EXDEGD = 0xA3,    // 特殊角セット初期化
  ENEMYSET = 0xA4,  // 敵を雑魚指定でセットする
  ENEMYSETD = 0xA5, // 敵セット(角度指定有り)
  HITXY = 0xA6,     // 敵の当たり判定を変更する
  ITEM = 0xA7,      // アイテムの種類をセットする
  STG4EFC = 0xA8,   // ４面ボス用同期エフェクト管理
  ANMEX = 0xA9,     // ダメージ中のアニメーションを設定
  BITLASER = 0xAA,  // ビットによるレーザーコマンド指定
  BITATTACK = 0xAB, // ビットによる攻撃指定
  BITCMD = 0xAC,    // ビットコマンド送信
  BOSSSET = 0xAD,   // ボスを発生させる
  CEFC = 0xAE,      // 円エフェクトを発生させる
  STG3EFC = 0xAF,   // ３面星エフェクト発動

  // 0xB? : レジスタ使用コマンド
  MOVR = 0xB0, // レジスタ<->構造体変数の代入
  MOVC = 0xB1, // レジスタ<- 定数(即値)の代入
  ADD = 0xB2,  // 加算命令
  SUB = 0xB3,  // 減算命令
  SINL = 0xB4, // sinl(Gr0,Gr1)
  COSL = 0xB5, // cosl(Gr0,Gr1)
  MOD = 0xB6,  // Gr0 = Gr0 % Const
  RND = 0xB7,  // Gr0 = rnd()
  CMPR = 0xB8, // Gr0,Gr1 の比較
  CMPC = 0xB9, // Gr0,Const の比較
  JL = 0xBA,   // 比較結果が > ならばジャンプ
  JS = 0xBB,   // 比較結果が < ならばジャンプ
  INC = 0xBC,  // レジスタ＋１
  DEC = 0xBD,  // レジスタ－１
  JEQ = 0xBE,  // 比較結果が = ならばジャンプ
};

// Command group sizes (number of opcodes per prefix)
constexpr uint8_t ECL_CMD0 = 14;
constexpr uint8_t ECL_CMD1 = 16;
constexpr uint8_t ECL_CMD2 = 15;
constexpr uint8_t ECL_CMD45 = 22;
constexpr uint8_t ECL_CMD67 = 18;
constexpr uint8_t ECL_CMD8 = 6;
constexpr uint8_t ECL_CMD9 = 10;
constexpr uint8_t ECL_CMDA = 16;
constexpr uint8_t ECL_CMDB = 15;
constexpr auto ECL_CMDMAX =
    (ECL_CMD0 + ECL_CMD1 + ECL_CMD2 + ECL_CMD45 + ECL_CMD67 + ECL_CMD8 +
     ECL_CMD9 + ECL_CMDA + ECL_CMDB);

// ============================================================
// ECL register / field IDs — used by MOVR, ID2Value, etc.
// ============================================================
enum class EclReg : uint8_t {
  // General-purpose registers
  GR0 = 0,
  GR1 = 1,
  GR2 = 2,
  GR3 = 3,
  GR4 = 4,
  GR5 = 5,
  GR6 = 6,
  GR7 = 7,

  // Laser command fields
  LCMD_D = 128,
  LCMD_DW = 129,
  LCMD_N = 130,
  LCMD_C = 131,
  LCMD_L = 132,
  LCMD_V = 133,

  // Bullet command fields
  TCMD_D = 134,
  TCMD_DW = 135,
  TCMD_N = 136,
  TCMD_NS = 137,
  TCMD_V = 138,
  TCMD_C = 139,
  TCMD_A = 140,
  TCMD_REP = 141,
  TCMD_VD = 142,

  // Enemy fields
  ENEMY_X = 143,
  ENEMY_Y = 144,
  ENEMY_D = 145,
};

constexpr uint8_t ECLREG_MAX = 8;
constexpr uint8_t ECLCST_LLASERALL = 0xff;

// ============================================================
// Interrupt vector types
// ============================================================
enum class EclIntVec : uint8_t {
  BOSSLEFT = 0x00, // ボス残り数割り込み
  HP = 0x01,       // 体力が指定値より小さいときに割り込み
  TIMER = 0x02,    // タイマー割り込み
  BITLEFT = 0x03,  // 残りビット数割り込み
};
constexpr uint8_t ECLVECT_MAX = 4;

// ============================================================
// Interrupt command IDs (for EclOp::INT opcode)
// ============================================================
enum class EclIntType : uint8_t {
  SNAKEON = 0x00,  // 蛇型セット
  LBWING01 = 0x01, // ラスボスの蝶の羽モード
  LBWING02 = 0x02, // ラスボスの鳥の羽モード
  BITON5 = 0x03,   // ビット装着(５つ)
  BITON6 = 0x04,   // ビット装着(６つ)
  SHILD1 = 0x05,   // ボム回避１
  SHILD2 = 0x06,   // ボム回避１
};
