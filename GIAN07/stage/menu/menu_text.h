/*                                                                           */
/*   menu_text.h   動的メニューテキスト                                       */
/*                                                                           */
/*   std::string を所有し、std::format でフォーマットする。 */
/*   Narrow::literal ビューを提供し、MenuItem::Title への設定を可能にする。   */
/*                                                                           */

#pragma once

#include "game/narrow.h"

#include <format>
#include <string>
#include <string_view>

class MenuText {
public:
  MenuText() = default;
  explicit MenuText(std::string_view s) : storage_(s) {}

  // std::format でテキストをフォーマットする。
  template <typename... Args>
  void Format(std::format_string<Args...> fmt, Args &&...args) {
    storage_ = std::format(fmt, std::forward<Args>(args)...);
  }

  // 文字列を直接設定する。
  void Set(std::string_view s) { storage_ = s; }

  // テキストをクリアする。
  void Clear() { storage_.clear(); }

  // 現在の内容への Narrow::literal ビューを返す。
  // 有効期限: 次の Format/Set/Clear 呼び出しまで。
  Narrow::literal Lit() const { return Narrow::literal{storage_.c_str()}; }

  const char *c_str() const { return storage_.c_str(); }
  std::string_view View() const { return storage_; }
  bool empty() const { return storage_.empty(); }

private:
  std::string storage_;
};
