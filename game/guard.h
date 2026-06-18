#pragma once

#include <type_traits>
#include <utility>

namespace detail {
struct Nil {};
} // namespace detail

template <typename T, typename F>
class Guard {
    T value_;
    [[no_unique_address]] F deleter_;
    bool engaged_ = true;

    static constexpr bool has_value = std::is_invocable_v<F, T>;

public:
    Guard(T v, F d) : value_(std::move(v)), deleter_(std::move(d)) {}

    ~Guard() {
        if (engaged_) {
            if constexpr (has_value) {
                deleter_(value_);
            } else {
                deleter_();
            }
        }
    }

    Guard(const Guard &) = delete;
    Guard &operator=(const Guard &) = delete;

    Guard(Guard &&other) noexcept
        : value_(std::move(other.value_)),
          deleter_(std::move(other.deleter_)),
          engaged_(std::exchange(other.engaged_, false)) {}

    Guard &operator=(Guard &&other) noexcept {
        if (this != &other) {
            if (engaged_) {
                if constexpr (has_value) {
                    deleter_(value_);
                } else {
                    deleter_();
                }
            }
            value_ = std::move(other.value_);
            deleter_ = std::move(other.deleter_);
            engaged_ = std::exchange(other.engaged_, false);
        }
        return *this;
    }

    [[nodiscard]] const T &get() const { return value_; }
    [[nodiscard]] T &get() { return value_; }

    void release() { engaged_ = false; }

    void reset(T v) {
        if (engaged_) {
            if constexpr (has_value) {
                deleter_(value_);
            } else {
                deleter_();
            }
        }
        value_ = std::move(v);
        engaged_ = true;
    }
};

template <typename T, typename F>
Guard<T, F> make_guard(T v, F d) {
    return Guard<T, F>(std::move(v), std::move(d));
}

template <typename F>
Guard<detail::Nil, F> make_guard(F f) {
    return Guard<detail::Nil, F>(detail::Nil{}, std::move(f));
}
