///
/// Platform-specific thread interface via SDL
///

#pragma once

#include <atomic>
#include <concepts>
#include <memory>
#include <utility>
struct SDL_Thread;

using THREAD_STOP = std::atomic<bool>;

template <std::invocable<const THREAD_STOP &> F> struct THREAD_META {
  F f;
  THREAD_STOP &st;
};

class THREAD {
  SDL_Thread *sdl_thread = nullptr;
  std::unique_ptr<THREAD_STOP> st;

public:
  bool Joinable() const noexcept;
  void Join() noexcept;
  void Abort() noexcept;

  THREAD() noexcept = default;
  THREAD(SDL_Thread *sdl_thread, std::unique_ptr<THREAD_STOP> st);
  THREAD(THREAD &&other) noexcept;
  THREAD(const THREAD &) = delete;
  THREAD &operator=(THREAD &&other) noexcept;
  ~THREAD();
};

SDL_Thread *HelpCreateThread(int (*fn)(void *), void *data);

template <std::invocable<const THREAD_STOP &> F> int ThreadFunc(void *data) {
  const std::unique_ptr<THREAD_META<F>> meta(
      static_cast<THREAD_META<F> *>(data));

  meta.get()->f(meta.get()->st);
  return 0;
}

template <std::invocable<const THREAD_STOP &> F>
[[nodiscard]] THREAD ThreadStart(F &&f) {
  auto st = std::make_unique<THREAD_STOP>();
  auto meta = std::make_unique<THREAD_META<F>>(std::forward<F>(f), *st.get());
  auto sdl_thread = HelpCreateThread(ThreadFunc<F>, meta.get());
  if (!sdl_thread) {
    return {};
  }
  meta.release();
  return THREAD{sdl_thread, std::move(st)};
}
