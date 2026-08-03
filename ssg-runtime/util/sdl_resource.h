///
/// RAII wrapper for SDL objects that are released by a free function.
///

#pragma once

#include <utility>

namespace util {

// [Destroy] is a `void Destroy(T *)` function such as SDL_DestroyWindow.
// Implicitly converts to `T *` for direct use with the SDL C API.
template <typename T, auto Destroy>
class SdlResource {
public:
  constexpr SdlResource() noexcept = default;
  explicit constexpr SdlResource(T *ptr) noexcept : ptr_(ptr) {}
  ~SdlResource() { Reset(); }

  SdlResource(const SdlResource &) = delete;
  SdlResource &operator=(const SdlResource &) = delete;

  SdlResource(SdlResource &&other) noexcept : ptr_(other.Release()) {}
  SdlResource &operator=(SdlResource &&other) noexcept {
    if (this != &other) {
      Reset();
      ptr_ = other.Release();
    }
    return *this;
  }

  [[nodiscard]] T *get() const noexcept { return ptr_; }
  T *operator->() const noexcept { return ptr_; }
  T &operator*() const noexcept { return *ptr_; }
  operator T *() const noexcept { return ptr_; }

  SdlResource &operator=(T *ptr) noexcept {
    Reset(ptr);
    return *this;
  }

  // Releases the current object and adopts [ptr]. Returns the new pointer.
  T *Reset(T *ptr = nullptr) noexcept {
    if (ptr_ != ptr) {
      if (ptr_ != nullptr) {
        Destroy(ptr_);
      }
      ptr_ = ptr;
    }
    return ptr_;
  }

  T *Release() noexcept { return std::exchange(ptr_, nullptr); }

private:
  T *ptr_ = nullptr;
};

} // namespace util
