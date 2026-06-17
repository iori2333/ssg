/*                                                                           */
/*   SCL.h   ＳＣＬ用定義ファイル                                            */
/*                                                                           */
/*                                                                           */

#pragma once

///// [更新履歴] /////

// 2000/03/14 : WAITEX,STAGECLEAR 命令を追加
// 2000/02/28 : ＢＯＳＳ命令を変更
// 2000/02/24 : ミディ関連の関数を追加
// 2000/02/18 : システムのアップデート開始

///// 特殊な命令の仕様について /////

// WAITEX <待ち条件(BYTE)>,<オプション(DWORD)>
// 待ち条件の BOSSHP
// は、主に背景エフェクトチェンジ等に用いること(状態推移には使用しない)

///// [ 定数 ] /////

// SCL 命令 //
inline constexpr uint8_t SCL_TIME  = 0x00; // 次のイベントの発動時間
inline constexpr uint8_t SCL_ENEMY = 0x01; // 敵イベント
inline constexpr uint8_t SCL_SSP   = 0x02; // スクロールスピードチェンジ
inline constexpr uint8_t SCL_EFC   = 0x03; // エフェクトセット
inline constexpr uint8_t SCL_END   = 0x04; // ＳＣＬ終了
inline constexpr uint8_t SCL_BOSS  = 0x05; // ボス発生(引数は X(16),Y(16),BossID(8))

// SCL レベル２命令 //
inline constexpr uint8_t SCL_MWOPEN       = 0x06; // メッセージウィンドウを開く
inline constexpr uint8_t SCL_MWCLOSE      = 0x07; // メッセージウィンドウを閉じる
inline constexpr uint8_t SCL_MSG          = 0x08; // メッセージを出力する
inline constexpr uint8_t SCL_KEY          = 0x09; // キー入力待ち
inline constexpr uint8_t SCL_NPG          = 0x0a; // 新しいページに変更する
inline constexpr uint8_t SCL_FACE         = 0x0b; // 顔を表示する
inline constexpr uint8_t SCL_MUSIC        = 0x0c; // 曲データをロードする
inline constexpr uint8_t SCL_BOSSDEAD     = 0x0d; // ボス強制破壊(すなわち時間切れ)
inline constexpr uint8_t SCL_LOADFACE     = 0x0e; // 顔グラをロードする(引数は、SurfaceID(BYTE),FileNo(BYTE))
inline constexpr uint8_t SCL_WAITEX       = 0x0f; // ある条件が起こるまでＳＣＬをストップする
inline constexpr uint8_t SCL_STAGECLEAR   = 0x10; // そのステージが終了することを意味する。次のステージへGO!
inline constexpr uint8_t SCL_MAPPALETTE   = 0x11; // パレットをマップパーツ用のもので初期化する(For 8BitMode)
inline constexpr uint8_t SCL_GAMECLEAR    = 0x12; // タイトルに戻る(ネームレジスト有)
inline constexpr uint8_t SCL_DELENEMY     = 0x13; // 敵を強制消去(インデックス配列そのものを)する
inline constexpr uint8_t SCL_ENEMYPALETTE = 0x14; // 敵のパレットにする
inline constexpr uint8_t SCL_STAFF        = 0x15; // スタッフＩＤセット
inline constexpr uint8_t SCL_EXTRACLEAR   = 0x16; // エキストラステージクリア

// EFC 命令の引数 //
inline constexpr uint8_t SEFC_WARN        = 0x00; // ワーニング音・開始
inline constexpr uint8_t SEFC_WARNSTOP    = 0x01; // ワーニング音・停止
inline constexpr uint8_t SEFC_MUSICFADE   = 0x02; // 曲フェードアウト実行(Level2)
inline constexpr uint8_t SEFC_STG2BOSS    = 0x03; // ステージ２ボスのスクロール発動！！
inline constexpr uint8_t SEFC_RASTERON    = 0x04; // ラスタースクロール開始(砂漠とか海底都市とかに使えるかも)
inline constexpr uint8_t SEFC_RASTEROFF   = 0x05; // ラスタースクロール終了
inline constexpr uint8_t SEFC_CFADEIN     = 0x06; // 円形フェードイン
inline constexpr uint8_t SEFC_CFADEOUT    = 0x07; // 円形フェードアウト
inline constexpr uint8_t SEFC_STG3BOSS    = 0x08; // ３面ボス雲
inline constexpr uint8_t SEFC_STG3RESET   = 0x09; // ３面ボス雲リセット
inline constexpr uint8_t SEFC_STG6CUBE    = 0x0a; // ６面ボス３Ｄキューヴ
inline constexpr uint8_t SEFC_STG6RNDECL  = 0x0b; // ６面ボス偽ＥＣＬ羅列
inline constexpr uint8_t SEFC_STG4ROCK    = 0x0c; // ４面岩
inline constexpr uint8_t SEFC_STG4LEAVE   = 0x0d; // ４面岩を画面外に吐き出す
inline constexpr uint8_t SEFC_WHITEIN     = 0x0e; // ホワイトイン
inline constexpr uint8_t SEFC_WHITEOUT    = 0x0f; // ホワイトアウト
inline constexpr uint8_t SEFC_LOADEX01    = 0x10; // エキストラボス１用画像をロード
inline constexpr uint8_t SEFC_LOADEX02    = 0x11; // エキストラボス２用画像をロード
inline constexpr uint8_t SEFC_STG6RASTER  = 0x12; // ６面ラスター

// WAITEX 命令の引数(Level2) //
inline constexpr uint8_t SWAIT_BOSSLEFT = 0x00; // ボスの残り数(OPT:ボスの数)
inline constexpr uint8_t SWAIT_BOSSHP   = 0x01; // ボスのＨＰ総和が指定値より小さい(OPT:残りＨＰ)
