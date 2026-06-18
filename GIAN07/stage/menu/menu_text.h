///
/// MenuText - Dynamic menu text
///

#pragma once

#include <format>
#include <string>
#include <string_view>

class MenuText {
public:
  MenuText() = default;
  explicit MenuText(std::string_view s) : storage_(s) {}

  // Format text with std::format
  template <typename... Args>
  void Format(std::format_string<Args...> fmt, Args &&...args) {
    storage_ = std::format(fmt, std::forward<Args>(args)...);
  }

  // Set string directly
  void Set(std::string_view s) { storage_ = s; }

  // Clear text
  void Clear() { storage_.clear(); }

  // Return const char* view of current contents.
  // Validity: until next Format/Set/Clear call.
  const char *Lit() const { return storage_.c_str(); }

  const char *c_str() const { return storage_.c_str(); }
  std::string_view View() const { return storage_; }
  bool empty() const { return storage_.empty(); }

private:
  std::string storage_;
};
