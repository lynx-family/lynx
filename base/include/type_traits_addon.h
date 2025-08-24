// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_TYPE_TRAITS_ADDON_H_
#define BASE_INCLUDE_TYPE_TRAITS_ADDON_H_

#include <type_traits>

namespace lynx {
namespace base {
// Implementation of C++20's std::remove_cvref.
//
// References:
// - https://en.cppreference.com/w/cpp/types/remove_cvref
// - https://wg21.link/meta.trans.other#lib:remove_cvref
template <typename T>
struct remove_cvref {
  using type = std::remove_cv_t<std::remove_reference_t<T>>;
};

// Implementation of C++20's std::remove_cvref_t.
//
// References:
// - https://en.cppreference.com/w/cpp/types/remove_cvref
// - https://wg21.link/meta.type.synop#lib:remove_cvref_t
template <typename T>
using remove_cvref_t = typename remove_cvref<T>::type;

/**
 * Checks if a type is template instance of another type.
 * For example:
 *     is_instance<std::shared_ptr<std::string>, std::shared_ptr> = true
 */
template <class, template <class, class...> class>
struct is_instance : public std::false_type {};

template <class... Ts, template <class, class...> class U>
struct is_instance<U<Ts...>, U> : public std::true_type {};

/**
 * By default, std::is_trivially_copyable_v<std::pair<int, int>> is false which
 * causes Vector<std::pair<int, int>> to be treated with non-trivial
 * element type. We check first and second elements of std::pair<> and treat the
 * whole type as trivial if both elements are trivial.
 *
 * From C++23, the standard was updated to mandate that if `T1` and `T2` are
 * both trivially copyable, then `std::pair<T1, T2>` must also be trivially
 * copyable.
 */
template <typename T, bool is_pair>
struct IsTrivial {};

template <typename T>
struct IsTrivial<T, false> {
  static constexpr auto value =
      std::is_trivially_destructible_v<T> && std::is_trivially_copyable_v<T>;
};

template <typename T>
struct IsTrivial<T, true> {
  static constexpr auto value =
      std::is_trivially_destructible_v<T> &&
      IsTrivial<typename T::first_type,
                is_instance<typename T::first_type, std::pair>{}>::value &&
      IsTrivial<typename T::second_type,
                is_instance<typename T::second_type, std::pair>{}>::value;
};

// Check if T has declared flag `TriviallyDestructibleAfterMove`.
// When this optimization hint is turned on, if one T instance is moved away,
// its destructor will not be called.
template <typename T, typename = void>
struct has_trivially_destructible_after_move_flag : std::false_type {};

template <typename T>
struct has_trivially_destructible_after_move_flag<
    T, std::void_t<typename T::TriviallyDestructibleAfterMove>>
    : std::true_type {};

template <typename T, bool is_pair>
struct IsTriviallyDestructibleAfterMove {};

template <typename T>
struct IsTriviallyDestructibleAfterMove<T, false> {
  static constexpr auto value =
      std::is_trivially_destructible_v<T> ||
      has_trivially_destructible_after_move_flag<T>::value;
};

template <typename T>
struct IsTriviallyDestructibleAfterMove<T, true> {
  static constexpr auto value =
      (std::is_trivially_destructible_v<typename T::first_type> ||
       IsTriviallyDestructibleAfterMove<
           typename T::first_type,
           is_instance<typename T::first_type, std::pair>{}>::value) &&
      (std::is_trivially_destructible_v<typename T::second_type> ||
       IsTriviallyDestructibleAfterMove<
           typename T::second_type,
           is_instance<typename T::second_type, std::pair>{}>::value);
};

// Check if T has declared flag `TriviallyRelocatable`.
// When this optimization hint is turned on, the instances of type T can be
// moved as a whole(if multiple instances on linear memory) to another piece of
// memory using the memcpy or memmove method, and the T type instance in the
// original memory does not need destruction.
template <typename T, typename = void>
struct has_trivially_relocatable_flag : std::false_type {};

template <typename T>
struct has_trivially_relocatable_flag<
    T, std::void_t<typename T::TriviallyRelocatable>> : std::true_type {};

template <typename T, bool is_pair>
struct IsTriviallyRelocatable {};

template <typename T>
struct IsTriviallyRelocatable<T, false> {
  static constexpr auto value =
      IsTrivial<T, is_instance<T, std::pair>{}>::value ||
      has_trivially_relocatable_flag<T>::value;
};

template <typename T>
struct IsTriviallyRelocatable<T, true> {
  static constexpr auto value =
      (IsTrivial<typename T::first_type,
                 is_instance<typename T::first_type, std::pair>{}>::value ||
       IsTriviallyRelocatable<
           typename T::first_type,
           is_instance<typename T::first_type, std::pair>{}>::value) &&
      (IsTrivial<typename T::second_type,
                 is_instance<typename T::second_type, std::pair>{}>::value ||
       IsTriviallyRelocatable<
           typename T::second_type,
           is_instance<typename T::second_type, std::pair>{}>::value);
};

template <typename T>
class alignas(T) TypeOfPlainBytes {
 public:
  bool operator==(const TypeOfPlainBytes& other) const {
    return reinterpret_cast<const T&>(*this) ==
           reinterpret_cast<const T&>(other);
  }

  // Must be trivially relocatable.
  using TriviallyRelocatable = bool;

 private:
  [[maybe_unused]] char buffer_[sizeof(T)];
};

template <typename T>
struct IsTypeOfPlainBytes : std::false_type {};

template <typename T>
struct IsTypeOfPlainBytes<TypeOfPlainBytes<T>> : std::true_type {};

}  // namespace base
}  // namespace lynx

namespace std {
template <typename T>
struct hash<lynx::base::TypeOfPlainBytes<T>> {
  std::size_t operator()(const lynx::base::TypeOfPlainBytes<T>& k) const {
    return hash<T>()(reinterpret_cast<const T&>(k));
  }
};
}  // namespace std

#endif  // BASE_INCLUDE_TYPE_TRAITS_ADDON_H_
