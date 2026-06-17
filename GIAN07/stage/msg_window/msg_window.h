/*                                                                           */
/*   msg_window.h   メッセージウィンドウ処理                                  */
/*                                                                           */
/*                                                                           */

#pragma once

#include "window_sys.h" // MSG_WINDOW_FLAGS, WINDOW_LTRB, FONT_ID, etc.

#include <optional>

// メッセージウィンドウ管理用クラス //
// 元は [MSG_WINDOW] 構造体 + ファイル静的グローバル [MsgWindow] だったが、
// 状態をカプセル化するためクラス化した。[MWin*] 自由関数はこのグローバル
// インスタンスへの薄い転調として残されている。
class MsgWindow {
public:
  void Init(const WINDOW_LTRB &rc,
            MSG_WINDOW_FLAGS flags = MSG_WINDOW_FLAGS::NONE);
  void Open();       // メッセージウィンドウをオープンする
  void Close();      // メッセージウィンドウをクローズする
  void ForceClose(); // メッセージウィンドウを強制クローズする
  void Tick();       // メッセージウィンドウを動作させる
  void Draw();       // メッセージウィンドウを描画する
  void Msg(Narrow::string_view str); // メッセージ文字列を送る
  void Face(uint8_t faceID);         // 顔をセットする
  void Cmd(uint8_t cmd);             // コマンドを送る
  void Help(WINDOW_SYSTEM *ws);      // メッセージウィンドウにヘルプ文字列を送る

private:
  void MsgBlank(); // 文字列をクリアし、最初の行へ戻す

  WINDOW_LTRB max_size{}; // ウィンドウの最終的な大きさ
  WINDOW_LTRB now_size{}; // ウィンドウの現在のサイズ
  PIXEL_POINT text_topleft{};

  MSG_WINDOW_FLAGS flags{};
  FONT_ID font_id{};   // 使用するフォント
  uint8_t font_dy{};   // フォントのＹ増量値
  uint8_t state{};     // 状態
  uint8_t max_line{};  // 最大表示可能行数
  uint8_t line{};      // 次に挿入する行

  uint8_t face_id{};    // 使用する顔番号
  uint8_t next_face{};  // 次に表示する顔番号
  uint8_t face_state{}; // 顔の状態
  uint8_t face_time{};  // 顔表示用カウンタ

  Narrow::string_view msg[MSG_HEIGHT]{}; // 表示するメッセージへのポインタ

  // Contains all text from [msg], concatenated with '\n'.
  Narrow::string text;

  std::optional<TEXTRENDER_RECT_ID> trr;
};

// 移行用グローバルインスタンス。最終的には UIManager 等に集約する。
extern MsgWindow MsgWin;
