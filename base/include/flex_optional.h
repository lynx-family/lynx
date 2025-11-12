// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FLEX_OPTIONAL_H_
#define BASE_INCLUDE_FLEX_OPTIONAL_H_

#include <cstdlib>
#include <memory>
#include <optional>
#include <type_traits>
#include <utility>

namespace lynx {
namespace base {

namespace optional {

// Heap allocated storage
template <class T>
struct flex_optional_mem_save {
  using value_type = T;
  ~flex_optional_mem_save() = default;
  flex_optional_mem_save() noexcept = default;
  flex_optional_mem_save(const flex_optional_mem_save& other) {
    if (other.val_) {
      val_ = std::unique_ptr<value_type>(new value_type(*other.val_));
    }
  }
  flex_optional_mem_save(flex_optional_mem_save&& other) noexcept
      : val_(std::move(other.val_)) {}
  flex_optional_mem_save(std::nullopt_t) noexcept {}

  template <class... Args>
  explicit flex_optional_mem_save(std::in_place_t, Args&&... args)
      : val_(new value_type(std::forward<Args>(args)...)) {}

  template <class U, class... Args>
  explicit flex_optional_mem_save(std::in_place_t,
                                  std::initializer_list<U> list, Args&&... args)
      : val_(new value_type(list, std::forward<Args>(args)...)) {}

  template <class U = std::remove_cv_t<value_type>,
            class = std::enable_if_t<std::is_constructible_v<value_type, U>>>
  flex_optional_mem_save(U&& value)
      : val_(new value_type(std::forward<U>(value))) {}

  template <class U>
  explicit flex_optional_mem_save(const flex_optional_mem_save<U>& other)
      : val_(other ? new value_type(*other.val_) : nullptr) {}

  template <class U>
  explicit flex_optional_mem_save(flex_optional_mem_save<U>&& other)
      : val_(other ? new value_type(*other.val_) : nullptr) {}

  flex_optional_mem_save& operator=(std::nullopt_t) noexcept {
    reset();
    return *this;
  }

  // same value_type
  flex_optional_mem_save& operator=(const flex_optional_mem_save& other) {
    this->val_ = std::unique_ptr<value_type>(new value_type(*other.val_));
    return *this;
  }

  flex_optional_mem_save& operator=(flex_optional_mem_save&& other) noexcept {
    this->val_ = std::move(other.val_);
    return *this;
  }

  template <typename>
  friend struct flex_optional_mem_save;
  // value_type can be constructed from U
  template <class U,
            class = std::enable_if_t<std::is_constructible_v<value_type, U>>>
  flex_optional_mem_save& operator=(const flex_optional_mem_save<U>& other) {
    this->val_ = std::unique_ptr<value_type>(new value_type(*other.val_));
    return *this;
  }

  template <class U,
            class = std::enable_if_t<std::is_constructible_v<value_type, U>>>
  flex_optional_mem_save& operator=(flex_optional_mem_save<U>&& other) {
    this->val_ =
        std::unique_ptr<value_type>(new value_type(std::move(*other.val_)));
    return *this;
  }

  template <class U = std::remove_cv_t<value_type>,
            class = std::enable_if_t<std::is_constructible_v<value_type, U>>>
  flex_optional_mem_save& operator=(U&& value) {
    val_ = std::unique_ptr<value_type>(new value_type(std::forward<U>(value)));
    return *this;
  }

  template <class... Args>
  value_type& emplace(Args&&... args) {
    val_ = std::unique_ptr<value_type>(
        new value_type(std::forward<Args>(args)...));
    return *val_;
  }

  template <class U, class... Args>
  value_type& emplace(std::initializer_list<U> list, Args&&... args) {
    val_ = std::unique_ptr<value_type>(
        new value_type(list, std::forward<Args>(args)...));
    return *val_;
  }

  void swap(flex_optional_mem_save<value_type>& other) noexcept {
    std::swap(this->val_, other.val_);
  }

  const value_type* operator->() const noexcept { return val_.get(); }
  value_type* operator->() noexcept { return val_.get(); }
  const value_type& operator*() const& noexcept { return *val_; }
  value_type& operator*() & noexcept { return *val_; }
  const value_type&& operator*() const&& noexcept { return *std::move(val_); }
  value_type&& operator*() && noexcept { return *std::move(val_); }

  bool has_value() const noexcept { return val_ != nullptr; }

  explicit operator bool() const noexcept { return has_value(); }

  value_type& value() & {
    if (!has_value()) {
      abort();
    }
    return *val_;
  }
  const value_type& value() const& {
    if (!has_value()) {
      abort();
    }
    return *val_;
  }

  value_type&& value() && {
    if (!has_value()) {
      abort();
    }
    return std::move(*val_);
  }
  const value_type&& value() const&& {
    if (!has_value()) {
      abort();
    }
    return std::move(*val_);
  }

  template <class U = std::remove_cv_t<value_type>>
  value_type value_or(U&& default_value) const& {
    return has_value()
               ? *val_
               : static_cast<value_type>(std::forward<U>(default_value));
  }

  template <class U = std::remove_cv_t<value_type>>
  value_type value_or(U&& default_value) && {
    return has_value()
               ? std::move(val_)
               : static_cast<value_type>(std::forward<U>(default_value));
  }

  void reset() noexcept { val_ = nullptr; }

 private:
  std::unique_ptr<value_type> val_;
};

template <class T>
flex_optional_mem_save(T) -> flex_optional_mem_save<T>;

// Comparisons between optionals
template <class _Tp, class _Up>
std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() ==
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator==(const flex_optional_mem_save<_Tp>& __x,
           const flex_optional_mem_save<_Up>& __y) {
  if (static_cast<bool>(__x) != static_cast<bool>(__y)) return false;
  if (!static_cast<bool>(__x)) return true;
  return *__x == *__y;
}

template <class _Tp, class _Up>
std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() !=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator!=(const flex_optional_mem_save<_Tp>& __x,
           const flex_optional_mem_save<_Up>& __y) {
  if (static_cast<bool>(__x) != static_cast<bool>(__y)) return true;
  if (!static_cast<bool>(__x)) return false;
  return *__x != *__y;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() <
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator<(const flex_optional_mem_save<_Tp>& __x,
          const flex_optional_mem_save<_Up>& __y) {
  if (!static_cast<bool>(__y)) return false;
  if (!static_cast<bool>(__x)) return true;
  return *__x < *__y;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() >
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator>(const flex_optional_mem_save<_Tp>& __x,
          const flex_optional_mem_save<_Up>& __y) {
  if (!static_cast<bool>(__x)) return false;
  if (!static_cast<bool>(__y)) return true;
  return *__x > *__y;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() <=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator<=(const flex_optional_mem_save<_Tp>& __x,
           const flex_optional_mem_save<_Up>& __y) {
  if (!static_cast<bool>(__x)) return true;
  if (!static_cast<bool>(__y)) return false;
  return *__x <= *__y;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() >=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator>=(const flex_optional_mem_save<_Tp>& __x,
           const flex_optional_mem_save<_Up>& __y) {
  if (!static_cast<bool>(__y)) return true;
  if (!static_cast<bool>(__x)) return false;
  return *__x >= *__y;
}

// Comparisons with nullopt
template <class _Tp>

bool operator==(const flex_optional_mem_save<_Tp>& __x,
                std::nullopt_t) noexcept {
  return !static_cast<bool>(__x);
}

template <class _Tp>

bool operator==(std::nullopt_t,
                const flex_optional_mem_save<_Tp>& __x) noexcept {
  return !static_cast<bool>(__x);
}

template <class _Tp>

bool operator!=(const flex_optional_mem_save<_Tp>& __x,
                std::nullopt_t) noexcept {
  return static_cast<bool>(__x);
}

template <class _Tp>

bool operator!=(std::nullopt_t,
                const flex_optional_mem_save<_Tp>& __x) noexcept {
  return static_cast<bool>(__x);
}

template <class _Tp>

bool operator<(const flex_optional_mem_save<_Tp>&, std::nullopt_t) noexcept {
  return false;
}

template <class _Tp>

bool operator<(std::nullopt_t,
               const flex_optional_mem_save<_Tp>& __x) noexcept {
  return static_cast<bool>(__x);
}

template <class _Tp>

bool operator<=(const flex_optional_mem_save<_Tp>& __x,
                std::nullopt_t) noexcept {
  return !static_cast<bool>(__x);
}

template <class _Tp>

bool operator<=(std::nullopt_t, const flex_optional_mem_save<_Tp>&) noexcept {
  return true;
}

template <class _Tp>

bool operator>(const flex_optional_mem_save<_Tp>& __x,
               std::nullopt_t) noexcept {
  return static_cast<bool>(__x);
}

template <class _Tp>

bool operator>(std::nullopt_t, const flex_optional_mem_save<_Tp>&) noexcept {
  return false;
}

template <class _Tp>

bool operator>=(const flex_optional_mem_save<_Tp>&, std::nullopt_t) noexcept {
  return true;
}

template <class _Tp>

bool operator>=(std::nullopt_t,
                const flex_optional_mem_save<_Tp>& __x) noexcept {
  return !static_cast<bool>(__x);
}

// Comparisons with T
template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() ==
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator==(const flex_optional_mem_save<_Tp>& __x, const _Up& __v) {
  return static_cast<bool>(__x) ? *__x == __v : false;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() ==
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator==(const _Tp& __v, const flex_optional_mem_save<_Up>& __x) {
  return static_cast<bool>(__x) ? __v == *__x : false;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() !=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator!=(const flex_optional_mem_save<_Tp>& __x, const _Up& __v) {
  return static_cast<bool>(__x) ? *__x != __v : true;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() !=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator!=(const _Tp& __v, const flex_optional_mem_save<_Up>& __x) {
  return static_cast<bool>(__x) ? __v != *__x : true;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() <
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator<(const flex_optional_mem_save<_Tp>& __x, const _Up& __v) {
  return static_cast<bool>(__x) ? *__x < __v : true;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() <
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator<(const _Tp& __v, const flex_optional_mem_save<_Up>& __x) {
  return static_cast<bool>(__x) ? __v < *__x : false;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() <=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator<=(const flex_optional_mem_save<_Tp>& __x, const _Up& __v) {
  return static_cast<bool>(__x) ? *__x <= __v : true;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() <=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator<=(const _Tp& __v, const flex_optional_mem_save<_Up>& __x) {
  return static_cast<bool>(__x) ? __v <= *__x : false;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() >
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator>(const flex_optional_mem_save<_Tp>& __x, const _Up& __v) {
  return static_cast<bool>(__x) ? *__x > __v : false;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() >
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator>(const _Tp& __v, const flex_optional_mem_save<_Up>& __x) {
  return static_cast<bool>(__x) ? __v > *__x : true;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() >=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator>=(const flex_optional_mem_save<_Tp>& __x, const _Up& __v) {
  return static_cast<bool>(__x) ? *__x >= __v : false;
}

template <class _Tp, class _Up>

std::enable_if_t<std::is_convertible_v<decltype(std::declval<const _Tp&>() >=
                                                std::declval<const _Up&>()),
                                       bool>,
                 bool>
operator>=(const _Tp& __v, const flex_optional_mem_save<_Up>& __x) {
  return static_cast<bool>(__x) ? __v >= *__x : true;
}

// Check if T has declared flag `AlwaysUseFlexOptionalMemSave`.
template <typename T, typename = void>
struct has_always_use_flex_optional_mem_save : std::false_type {};

template <typename T>
struct has_always_use_flex_optional_mem_save<
    T, std::void_t<typename T::AlwaysUseFlexOptionalMemSave>> : std::true_type {
};

// switching between std::optional and flex_optional_mem_save
template <class T, typename Enable = void>
struct flex_optional_type {
  using Type = flex_optional_mem_save<T>;
};

template <class T>
struct flex_optional_type<
    T, std::enable_if_t<(sizeof(T) <= 32) &&
                        !has_always_use_flex_optional_mem_save<T>::value>> {
  using Type = std::optional<T>;
};

}  // namespace optional

template <class T>
using flex_optional = typename optional::flex_optional_type<T>::Type;

template <class T>
constexpr flex_optional<std::decay_t<T>> make_flex_optional(T&& v) {
  return flex_optional<std::decay_t<T>>(std::forward<T>(v));
}

template <class T, class... Args>
constexpr flex_optional<T> make_flex_optional(Args&&... args) {
  return flex_optional<T>(std::in_place, std::forward<Args>(args)...);
}

template <class T, class _Up, class... Args>
constexpr flex_optional<T> make_flex_optional(std::initializer_list<_Up> list,
                                              Args&&... args) {
  return flex_optional<T>(std::in_place, list, std::forward<Args>(args)...);
}

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_FLEX_OPTIONAL_H_
