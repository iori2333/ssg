/*                                                                           */
/*   ECL.h   敵コントロール言語用の定数                                      */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////

// 2000/11/27 : STG4EFC を追加・それに関する定数を追加
// 2000/10/16 : JEQ を追加
// 2000/09/05 : HLASER を追加
// 2000/04/26 : LASER2 を追加
// 2000/03/22 : LLaser 命令を追加
// 2000/03/15 : 命令を大幅に追加 (割り込み系、弾消去、レジスタ比較)
// 2000/02/18 : システムのアップデート開始

///// [ 定数 ] /////

// 0x0? : 制御用コマンド //
inline constexpr auto ECL_CMD0 = 14;    //
inline constexpr auto ECL_SETUP = 0x00; // 敵データ初期化
inline constexpr auto ECL_END = 0x01;   // 敵強制消滅
inline constexpr auto ECL_JMP = 0x02;   // 強制ジャンプ
inline constexpr auto ECL_LOOP = 0x03;  // ループ(２重は不可、CX は使わないの....)
inline constexpr auto ECL_CALL = 0x04;  // サブルーチンを呼ぶ
inline constexpr auto ECL_RET = 0x05;   // サブルーチンから復帰する
inline constexpr auto ECL_JHPL = 0x06;  // ＨＰが指定値より大きければジャンプ
inline constexpr auto ECL_JHPS = 0x07;  // ＨＰが指定値より小さければジャンプ
inline constexpr auto ECL_JDIF = 0x08;  // 難易度によるswitch
inline constexpr auto ECL_JDSB = 0x09; // 進行角度がサボテンと一致したらジャンプ(誤差±４まで有効)
inline constexpr auto ECL_JFCL = 0x0A; // フレームカウンタが大きければジャンプ
inline constexpr auto ECL_JFCS = 0x0B; // フレームカウンタが小さければジャンプ
inline constexpr auto ECL_STI = 0x0C;  // 割り込みベクタをセットする(SeTInterrupt flag)
inline constexpr auto ECL_CLI = 0x0D;  // 割り込みを無効にする(CLearInterrupt flag)

// 0x1? : 移動用コマンド //
inline constexpr auto ECL_CMD1 = 16;     //
inline constexpr auto ECL_NOP = 0x10;    // 何もしない
inline constexpr auto ECL_NOPSC = 0x11;  // スクロールに流される
inline constexpr auto ECL_MOV = 0x12;    // 移動する
inline constexpr auto ECL_ROL = 0x13;    // 回転移動
inline constexpr auto ECL_LROL = 0x14;   // 直進＆回転移動
inline constexpr auto ECL_WAVX = 0x15;   // 波移動Ｘ
inline constexpr auto ECL_WAVY = 0x16;   // 波移動Ｙ
inline constexpr auto ECL_MXA = 0x17;    // Ｘ絶対移動
inline constexpr auto ECL_MYA = 0x18;    // Ｙ絶対移動
inline constexpr auto ECL_MXYA = 0x19;   // ＸＹ絶対移動
inline constexpr auto ECL_MXS = 0x1A;    // Ｘサボテンセット移動
inline constexpr auto ECL_MYS = 0x1B;    // Ｙサボテンセット移動
inline constexpr auto ECL_MXYS = 0x1C;   // ＸＹサボテンセット移動
inline constexpr auto ECL_ACC = 0x1D;    // 加速or減速つき移動
inline constexpr auto ECL_ACCXYA = 0x1E; // 減速付きＸＹ絶対セット
inline constexpr auto ECL_GRAX = 0x1F;   // 重力付きＸ反射移動(Ｙ>=GY_MAX ならば自動消滅)

// 0x2? : 数値セット用コマンド //
inline constexpr auto ECL_CMD2 = 15;    //
inline constexpr auto ECL_DEGA = 0x20;  // 角度絶対セット
inline constexpr auto ECL_DEGR = 0x21;  // 角度相対セット
inline constexpr auto ECL_DEGX = 0x22;  // 角度ランダムセット
inline constexpr auto ECL_DEGS = 0x23;  // 角度サボテンセット
inline constexpr auto ECL_SPDA = 0x24;  // 速度絶対セット
inline constexpr auto ECL_SPDR = 0x25;  // 速度相対セット
inline constexpr auto ECL_XYA = 0x26;   // 座標絶対セット
inline constexpr auto ECL_XYR = 0x27;   // 座標相対セット
inline constexpr auto ECL_DEGXU = 0x28; // 角度ランダムセット(上１２８度)
inline constexpr auto ECL_DEGXD = 0x29; // 角度ランダムセット(下１２８度)
inline constexpr auto ECL_DEGEX = 0x2A; // 角度特殊セット(EXDEGDと併用する)
inline constexpr auto ECL_XYS = 0x2B;   // 座標サボテンセット
inline constexpr auto ECL_DEGX2 = 0x2C; // 制限付き角度ランダム
inline constexpr auto ECL_XYRND = 0x2D; // 制限付き座標ランダム
inline constexpr auto ECL_XYL = 0x2E;   // 長さ指定座標相対(極座標的に指定)

// 0x4? : 弾発射用コマンド //
inline constexpr auto ECL_CMD45 = 22;    //
inline constexpr auto ECL_TAMA = 0x40;   // 弾発射
inline constexpr auto ECL_TAUTO = 0x41;  // 弾発射間隔をセットする(０：自動発射しない)
inline constexpr auto ECL_TXYR = 0x42;   // 弾発射位置の相対ずらし
inline constexpr auto ECL_TCMD = 0x43;   // 弾コマンド
inline constexpr auto ECL_TDEGA = 0x44;  // 弾発射角絶対指定
inline constexpr auto ECL_TDEGR = 0x45;  // 弾発射角相対指定
inline constexpr auto ECL_TNUMA = 0x46;  // 弾数絶対指定
inline constexpr auto ECL_TNUMR = 0x47;  // 弾数相対指定
inline constexpr auto ECL_TSPDA = 0x48;  // 弾初速度絶対指定
inline constexpr auto ECL_TSPDR = 0x49;  // 弾初速度相対指定
inline constexpr auto ECL_TOPT = 0x4a;   // 弾オプション指定
inline constexpr auto ECL_TTYPE = 0x4b;  // 弾の種類指定
inline constexpr auto ECL_TCOL = 0x4c;   // 弾の色または形状指定
inline constexpr auto ECL_TVDEG = 0x4d;  // 弾の角速度指定
inline constexpr auto ECL_TREP = 0x4e;   // 弾の繰り返し用
inline constexpr auto ECL_TDEGS = 0x4f;  // 弾発射角サボテンセット
inline constexpr auto ECL_TDEGE = 0x50;  // 弾発射角を自分の向きにセット
inline constexpr auto ECL_TAMA2 = 0x51;  // 難易度変化なし弾発射
inline constexpr auto ECL_TCLR = 0x52;   // 全ての弾を消去する
inline constexpr auto ECL_TAMAL = 0x53;  // 弾をライン状に発射する
inline constexpr auto ECL_T2ITEM = 0x54; // 弾の何割かをアイテム化する
inline constexpr auto ECL_TAMAEX = 0x55; //	エキストラボス用弾幕発射コマンド

// 0x6? : レーザー発射用コマンド //
inline constexpr auto ECL_CMD67 = 18;    //
inline constexpr auto ECL_LASER = 0x60;  // レーザー発射
inline constexpr auto ECL_LCMD = 0x61;   // レーザーコマンド
inline constexpr auto ECL_LLA = 0x62;    // レーザー長・絶対指定
inline constexpr auto ECL_LLR = 0x63;    // レーザー長・相対指定
inline constexpr auto ECL_LL2 = 0x64;    // レーザー発射位置
inline constexpr auto ECL_LDEGA = 0x65;  // レーザー発射角絶対指定
inline constexpr auto ECL_LDEGR = 0x66;  // レーザー発射角相対指定
inline constexpr auto ECL_LNUMA = 0x67;  // レーザーの本数絶対指定
inline constexpr auto ECL_LNUMR = 0x68;  // レーザーの本数相対指定
inline constexpr auto ECL_LSPDA = 0x69;  // レーザーの速さ絶対指定
inline constexpr auto ECL_LSPDR = 0x6a;  // レーザーの速さ相対指定
inline constexpr auto ECL_LCOL = 0x6b;   // レーザーの色
inline constexpr auto ECL_LTYPE = 0x6c;  // レーザーの種類
inline constexpr auto ECL_LWA = 0x6d;    // レーザーの太さ絶対指定
inline constexpr auto ECL_LDEGS = 0x6e;  // レーザー発射角サボテンセット
inline constexpr auto ECL_LDEGE = 0x6f;  // レーザー発射角を自分の向きにセット
inline constexpr auto ECL_LXY = 0x70;    // レーザーの発射座標セット(太レーザー用？)
inline constexpr auto ECL_LASER2 = 0x71; // レーザー発射

// 0x8? : 太レーザー&ホーミング発射用コマンド(構造体セットは上の命令を使用する)
// //
inline constexpr auto ECL_CMD8 = 6;        //
inline constexpr auto ECL_LLSET = 0x80;    // 太レーザーセット
inline constexpr auto ECL_LLOPEN = 0x81;   // 太レーザーオープン
inline constexpr auto ECL_LLCLOSE = 0x82;  // 太レーザークローズ(消去＆参照カウント減少)
inline constexpr auto ECL_LLCLOSEL = 0x83; // 太レーザーライン状態へ
inline constexpr auto ECL_LLDEGR = 0x84;   // 太レーザー角度相対変更
inline constexpr auto ECL_HLASER = 0x85;   // ホーミングレーザー発動！！

// 0x9? : フラグセット用コマンド //
inline constexpr auto ECL_CMD9 = 10;         //
inline constexpr auto ECL_DRAW_ON = 0x90;    // 描画する
inline constexpr auto ECL_DRAW_OFF = 0x91;   // 描画しない
inline constexpr auto ECL_CLIP_ON = 0x92;    // 画面外に出ても消さない
inline constexpr auto ECL_CLIP_OFF = 0x93;   // 画面外に出たら消す
inline constexpr auto ECL_DAMAGE_ON = 0x94;  // 無敵にする
inline constexpr auto ECL_DAMAGE_OFF = 0x95; // 無敵にしない
inline constexpr auto ECL_HITSB_ON = 0x96;   // 自機に当たる
inline constexpr auto ECL_HITSB_OFF = 0x97;  // 自機に当たらない
inline constexpr auto ECL_RLCHG_ON = 0x98;   // 左右反転を有効にする
inline constexpr auto ECL_RLCHG_OFF = 0x99;  // 左右反転を無効にする

// 0xA? : 特殊コマンド //
inline constexpr auto ECL_CMDA = 16;        //
inline constexpr auto ECL_ANM = 0xA0;       // アニメーションを変更する
inline constexpr auto ECL_PSE = 0xA1;       // 効果音を鳴らす
inline constexpr auto ECL_INT = 0xA2;       // ボス用割り込みを発生させる...
inline constexpr auto ECL_EXDEGD = 0xA3;    // 特殊角セット初期化
inline constexpr auto ECL_ENEMYSET = 0xA4;  // 敵を雑魚指定でセットする
inline constexpr auto ECL_ENEMYSETD = 0xA5; // 敵セット(角度指定有り)
inline constexpr auto ECL_HITXY = 0xA6;     // 敵の当たり判定を変更する
inline constexpr auto ECL_ITEM = 0xA7;      // アイテムの種類をセットする
inline constexpr auto ECL_STG4EFC = 0xA8;   // ４面ボス用同期エフェクト管理
inline constexpr auto ECL_ANMEX = 0xA9;     // ダメージ中のアニメーションを設定
inline constexpr auto ECL_BITLASER = 0xAA;  // ビットによるレーザーコマンド指定
inline constexpr auto ECL_BITATTACK = 0xAB; // ビットによる攻撃指定
inline constexpr auto ECL_BITCMD = 0xAC;    // ビットコマンド送信
inline constexpr auto ECL_BOSSSET = 0xAD;   // ボスを発生させる
inline constexpr auto ECL_CEFC = 0xAE;      // 円エフェクトを発生させる
inline constexpr auto ECL_STG3EFC = 0xAF;   // ３面星エフェクト発動

// 0xB? : レジスタ使用コマンド([80x86命令ちっく]に) //
inline constexpr auto ECL_CMDB = 15;   //
inline constexpr auto ECL_MOVR = 0xB0; // レジスタ<->構造体変数の代入
inline constexpr auto ECL_MOVC = 0xB1; // レジスタ<- 定数(即値)の代入
inline constexpr auto ECL_ADD = 0xB2;  // 加算命令
inline constexpr auto ECL_SUB = 0xB3;  // 減算命令
inline constexpr auto ECL_SINL = 0xB4; // sinl(Gr0,Gr1)
inline constexpr auto ECL_COSL = 0xB5; // cosl(Gr0,Gr1)
inline constexpr auto ECL_MOD = 0xB6;  // Gr0 = Gr0 % Const
inline constexpr auto ECL_RND = 0xB7;  // Gr0 = rnd()
inline constexpr auto ECL_CMPR = 0xB8; // Gr0,Gr1 の比較
inline constexpr auto ECL_CMPC = 0xB9; // Gr0,Const の比較
inline constexpr auto ECL_JL = 0xBA;   // 比較結果が > ならばジャンプ
inline constexpr auto ECL_JS = 0xBB;   // 比較結果が < ならばジャンプ
inline constexpr auto ECL_INC = 0xBC;  // レジスタ＋１
inline constexpr auto ECL_DEC = 0xBD;  // レジスタ－１
inline constexpr auto ECL_JEQ = 0xBE;  // 比較結果が = ならばジャンプ

// ECL 定数 //

// 割り込み命令は、数字が小さいほど優先順位が高い //
inline constexpr auto ECLVECT_MAX = 4;         // 割り込みベクタ最大数
inline constexpr auto ECLVECT_BOSSLEFT = 0x00; // ボス残り数割り込み
inline constexpr auto ECLVECT_HP = 0x01;       // 体力が指定値より小さいときに割り込み
inline constexpr auto ECLVECT_TIMER = 0x02;    // タイマー割り込み
inline constexpr auto ECLVECT_BITLEFT = 0x03;  // 残りビット数割り込み

inline constexpr auto ECLREG_MAX = 8; // レジスタの本数
inline constexpr auto ECLCST_GR0 = 0; // ０番レジスタ
inline constexpr auto ECLCST_GR1 = 1; // １番レジスタ
inline constexpr auto ECLCST_GR2 = 2; // ２番レジスタ
inline constexpr auto ECLCST_GR3 = 3; // ３番レジスタ
inline constexpr auto ECLCST_GR4 = 4; // ４番レジスタ
inline constexpr auto ECLCST_GR5 = 5; // ５番レジスタ
inline constexpr auto ECLCST_GR6 = 6; // ６番レジスタ
inline constexpr auto ECLCST_GR7 = 7; // ７番レジスタ

inline constexpr auto ECLCST_LCMD_D = (128 + 0);  // レーザーコマンド(角度)
inline constexpr auto ECLCST_LCMD_DW = (128 + 1); // レーザーコマンド(角度差)
inline constexpr auto ECLCST_LCMD_N = (128 + 2);  // レーザーコマンド(本数)
inline constexpr auto ECLCST_LCMD_C = (128 + 3);  // レーザーコマンド(色)
inline constexpr auto ECLCST_LCMD_L = (128 + 4);  // レーザーコマンド(長さ)
inline constexpr auto ECLCST_LCMD_V = (128 + 5);  // レーザーコマンド(速度)

inline constexpr auto ECLCST_TCMD_D = (128 + 6);    // 弾コマンド(角度)
inline constexpr auto ECLCST_TCMD_DW = (128 + 7);   // 弾コマンド(角度差)
inline constexpr auto ECLCST_TCMD_N = (128 + 8);    // 弾コマンド(個数)
inline constexpr auto ECLCST_TCMD_NS = (128 + 9);   // 弾コマンド(連射数)
inline constexpr auto ECLCST_TCMD_V = (128 + 10);   // 弾コマンド(速度)
inline constexpr auto ECLCST_TCMD_C = (128 + 11);   // 弾コマンド(色)
inline constexpr auto ECLCST_TCMD_A = (128 + 12);   // 弾コマンド(加速度)
inline constexpr auto ECLCST_TCMD_REP = (128 + 13); // 弾コマンド(繰り返し)
inline constexpr auto ECLCST_TCMD_VD = (128 + 14);  // 弾コマンド(角速度)

inline constexpr auto ECLCST_ENEMY_X = (128 + 15); // 敵のＸ座標
inline constexpr auto ECLCST_ENEMY_Y = (128 + 16); // 敵のＹ座標
inline constexpr auto ECLCST_ENEMY_D = (128 + 17); // 敵の角度

inline constexpr auto ECLCST_LLASERALL = 0xff; // 全てのレーザーに適用する場合に指定する数

inline constexpr auto ECLINT_SNAKEON = 0x00;  // 蛇型セット
inline constexpr auto ECLINT_LBWING01 = 0x01; // ラスボスの蝶の羽モード
inline constexpr auto ECLINT_LBWING02 = 0x02; // ラスボスの鳥の羽モード
inline constexpr auto ECLINT_BITON5 = 0x03;   // ビット装着(５つ)
inline constexpr auto ECLINT_BITON6 = 0x04;   // ビット装着(６つ)
inline constexpr auto ECLINT_SHILD1 = 0x05;   // ボム回避１
inline constexpr auto ECLINT_SHILD2 = 0x06;   // ボム回避１

// ＥＣＬコマンド最大数 (must be after all ECL_CMD* group sizes)
inline constexpr auto ECL_CMDMAX = (ECL_CMD0 + ECL_CMD1 + ECL_CMD2 + ECL_CMD45 + ECL_CMD67 + ECL_CMD8 + ECL_CMD9 + ECL_CMDA + ECL_CMDB);

