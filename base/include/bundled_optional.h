// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_BUNDLED_OPTIONAL_H_
#define BASE_INCLUDE_BUNDLED_OPTIONAL_H_

#include <algorithm>
#include <array>
#include <cstdint>
#include <numeric>
#include <utility>

#include "base/include/type_traits_addon.h"

namespace lynx {
namespace base {

namespace bo_priv {
template <typename T, typename... Pack>
struct get_type_index;
template <typename T, typename Head, typename... Rest>
struct get_type_index<T, Head, Rest...> {
  static constexpr std::size_t value = 1 + get_type_index<T, Rest...>::value;
};
template <typename T, typename... Rest>
struct get_type_index<T, T, Rest...> {
  static constexpr std::size_t value = 0;
};
template <typename T, typename... Pack>
constexpr std::size_t get_type_index_v = get_type_index<T, Pack...>::value;

template <std::size_t I, typename... Types>
struct get_type_at;
template <std::size_t I, typename Head, typename... Rest>
struct get_type_at<I, Head, Rest...> {
  using Type = typename get_type_at<I - 1, Rest...>::Type;
};
template <typename Head, typename... Rest>
struct get_type_at<0, Head, Rest...> {
  using Type = Head;
};
template <std::size_t I, typename... Types>
using get_type_at_t = typename get_type_at<I, Types...>::Type;

template <typename... Types>
struct sum_sizeof_aligned_helper;
template <typename T, typename... Rest>
struct sum_sizeof_aligned_helper<T, Rest...> {
  using FieldType = typename T::Type;
  static constexpr size_t alignment = sizeof(void*);
  static constexpr size_t aligned_size =
      (sizeof(FieldType) + alignment - 1) / alignment * alignment;
  static constexpr size_t value =
      aligned_size + sum_sizeof_aligned_helper<Rest...>::value;
};
template <>
struct sum_sizeof_aligned_helper<> {
  static constexpr size_t value = 0;
};
template <typename... Types>
constexpr size_t sum_sizeof_aligned() {
  return sum_sizeof_aligned_helper<Types...>::value;
}

constexpr uintptr_t align_up(uintptr_t addr, size_t alignment) {
  return (addr + alignment - 1) & ~(alignment - 1);
}
}  // namespace bo_priv

// =================================================================================
//  USAGE:
//
//  struct Parent {
//    struct NameDef { using Type = std::string; };
//    struct AttributesDef { using Type = std::vector<std::string>; };
//
//    char ch;
//    BundledOptionals<NameDef, AttributesDef> optionals;
//  };
// =================================================================================
template <typename... Types>
struct BundledOptionals {
 public:
  static constexpr auto kFieldsCount = sizeof...(Types);

 private:
  static_assert(bo_priv::sum_sizeof_aligned<Types...>() <=
                    (UINT8_MAX - 1) * sizeof(void*),
                "Total size of bundled types exceeds offset capacity.");
  static constexpr size_t kAlignment = sizeof(void*);
  static constexpr size_t kPaddedOffsetsSize =
      (kFieldsCount + kAlignment - 1) / kAlignment * kAlignment;

 public:
  BundledOptionals() noexcept : bundled_data_(nullptr) {
    std::fill(std::begin(offsets_), std::end(offsets_), UINT8_MAX);
  }

  ~BundledOptionals() { Clear(); }

  BundledOptionals(const BundledOptionals& other)
      : offsets_(other.offsets_), bundled_data_(nullptr) {
    if (!other.bundled_data_) return;
    size_t total_size = 0;
    for (size_t i = 0; i < kFieldsCount; ++i) {
      if (other.offsets_[i] != UINT8_MAX) {
        CallOnFieldType(i, SizeCalculatorFunctor(total_size));
      }
    }
    if (total_size > 0) {
      bundled_data_ = static_cast<void*>(new char[total_size]);
      for (size_t i = 0; i < kFieldsCount; ++i) {
        if (offsets_[i] != UINT8_MAX) {
          CallOnFieldType(
              i, CopyConstructorFunctor(i, bundled_data_, other.bundled_data_,
                                        offsets_));
        }
      }
    }
  }

  BundledOptionals(BundledOptionals&& other) noexcept
      : offsets_(other.offsets_), bundled_data_(other.bundled_data_) {
    other.bundled_data_ = nullptr;
    std::fill(std::begin(other.offsets_), std::end(other.offsets_), UINT8_MAX);
  }

  BundledOptionals& operator=(const BundledOptionals& other) {
    if (this != &other) {
      BundledOptionals temp(other);
      swap(temp);
    }
    return *this;
  }

  BundledOptionals& operator=(BundledOptionals&& other) noexcept {
    if (this != &other) {
      Clear();
      swap(other);
    }
    return *this;
  }

  template <typename T>
  static constexpr auto GetIndex() {
    return bo_priv::get_type_index_v<T, Types...>;
  }

  template <typename T>
  bool HasValue() const {
    return offsets_[GetIndex<T>()] != UINT8_MAX;
  }

  template <typename T>
  typename T::Type* GetOrNull() {
    constexpr auto type_index = GetIndex<T>();
    if (HasValue<T>()) {
      return reinterpret_cast<typename T::Type*>(
          reinterpret_cast<uintptr_t>(bundled_data_) +
          offsets_[type_index] * sizeof(void*));
    }
    return nullptr;
  }

  template <typename T>
  const typename T::Type* GetOrNull() const {
    constexpr auto type_index = GetIndex<T>();
    if (HasValue<T>()) {
      return reinterpret_cast<const typename T::Type*>(
          reinterpret_cast<uintptr_t>(bundled_data_) +
          offsets_[type_index] * sizeof(void*));
    }
    return nullptr;
  }

  template <typename T>
  typename T::Type& Get() {
    constexpr auto type_index = GetIndex<T>();
    if (!HasValue<T>()) {
      CreateField(type_index);
    }
    return *reinterpret_cast<typename T::Type*>(
        reinterpret_cast<uintptr_t>(bundled_data_) +
        offsets_[type_index] * sizeof(void*));
  }

  template <typename T>
  void Release() {
    ReleaseField(GetIndex<T>());
  }

  template <typename T>
  typename T::Type ReleaseTransfer() {
    typename T::Type result;
    constexpr auto type_index = GetIndex<T>();
    if (type_index >= kFieldsCount || offsets_[type_index] == UINT8_MAX) {
      return result;
    }
    result = std::move(Get<T>());
    ReleaseField(type_index);
    return result;
  }

  void Clear() {
    if (!bundled_data_) return;
    for (std::size_t i = 0; i < kFieldsCount; ++i) {
      if (offsets_[i] != UINT8_MAX) {
        destructors_[i](
            reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(bundled_data_) +
                                    offsets_[i] * sizeof(void*)));
      }
    }
    delete[] static_cast<char*>(bundled_data_);
    bundled_data_ = nullptr;
    std::fill(std::begin(offsets_), std::end(offsets_), UINT8_MAX);
  }

 private:
  struct SizeCalculatorFunctor {
    size_t& r;
    explicit SizeCalculatorFunctor(size_t& s) : r(s) {}
    template <class F>
    void operator()() const {
      r += (sizeof(F) + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*);
    }
  };

  struct CopyConstructorFunctor {
    size_t i;
    void* d;
    const void* s;
    const std::array<uint8_t, kPaddedOffsetsSize>& o;
    CopyConstructorFunctor(size_t i, void* d, const void* s,
                           const std::array<uint8_t, kPaddedOffsetsSize>& o)
        : i(i), d(d), s(s), o(o) {}
    template <class F>
    void operator()() const {
      new (reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(d) +
                                   o[i] * sizeof(void*)))
          F(*reinterpret_cast<const F*>(reinterpret_cast<uintptr_t>(s) +
                                        o[i] * sizeof(void*)));
    }
  };

  struct CreateFieldLayoutFunctor {
    size_t& t;
    uint8_t& o;
    CreateFieldLayoutFunctor(size_t& t, uint8_t& o) : t(t), o(o) {}
    template <class F>
    void operator()() const {
      o = t / sizeof(void*);
      t += (sizeof(F) + sizeof(void*) - 1) / sizeof(void*) * sizeof(void*);
    }
  };

  struct MoverAndCreatorFunctor {
    size_t ci, ni;
    void *nd, *od;
    const std::array<uint8_t, kPaddedOffsetsSize>& oo;
    const std::array<uint8_t, kPaddedOffsetsSize>& no;
    MoverAndCreatorFunctor(size_t ci, size_t ni, void* nd, void* od,
                           const std::array<uint8_t, kPaddedOffsetsSize>& oo,
                           const std::array<uint8_t, kPaddedOffsetsSize>& no)
        : ci(ci), ni(ni), nd(nd), od(od), oo(oo), no(no) {}
    template <class F>
    void operator()() const {
      uintptr_t na = reinterpret_cast<uintptr_t>(nd) + no[ci] * sizeof(void*);
      if (ci == ni) {
        new (reinterpret_cast<void*>(na)) F();
      } else {
        uintptr_t oa = reinterpret_cast<uintptr_t>(od) + oo[ci] * sizeof(void*);
        static constexpr auto is_trivially_relocatable =
            IsTriviallyRelocatable<F, is_instance<F, std::pair>{}>::value;
        static constexpr auto is_trivially_destructible_after_move =
            is_trivially_relocatable ||
            IsTriviallyDestructibleAfterMove<
                F, is_instance<F, std::pair>{}>::value;
        if constexpr (is_trivially_relocatable) {
          // Trivially copy as plain bytes and skip destructor.
          using TrivialFType = TypeOfPlainBytes<F>;
          *reinterpret_cast<TrivialFType*>(na) =
              *reinterpret_cast<TrivialFType*>(oa);
        } else {
          new (reinterpret_cast<void*>(na))
              F(std::move(*reinterpret_cast<F*>(oa)));
          if constexpr (!is_trivially_destructible_after_move) {
            reinterpret_cast<F*>(oa)->~F();
          }
        }
      }
    }
  };

  template <std::size_t I = 0, typename Func>
  void CallOnFieldType(std::size_t index, Func&& func) const {
    if constexpr (I < kFieldsCount) {
      if (I == index) {
        func.template
        operator()<typename bo_priv::get_type_at_t<I, Types...>::Type>();
      } else {
        CallOnFieldType<I + 1>(index, std::forward<Func>(func));
      }
    }
  }

  using DestructorFn = void (*)(void*);
  template <typename T>
  static void destroy_field_impl(void* ptr) {
    using ActualType = typename T::Type;
    reinterpret_cast<ActualType*>(ptr)->~ActualType();
  }
  static constexpr std::array<DestructorFn, kFieldsCount> destructors_ = {
      &destroy_field_impl<Types>...};

  void swap(BundledOptionals& other) noexcept {
    std::swap(bundled_data_, other.bundled_data_);
    std::swap(offsets_, other.offsets_);
  }

  void CreateField(std::size_t type_index) {
    std::array<uint8_t, kPaddedOffsetsSize> new_offsets_map;
    new_offsets_map.fill(UINT8_MAX);
    size_t new_total_size = 0;
    for (size_t i = 0; i < kFieldsCount; ++i) {
      if (offsets_[i] != UINT8_MAX || i == type_index) {
        CallOnFieldType(
            i, CreateFieldLayoutFunctor(new_total_size, new_offsets_map[i]));
      }
    }
    if (new_total_size == 0) return;
    void* new_data = static_cast<void*>(new char[new_total_size]);
    void* old_data = bundled_data_;
    for (size_t i = 0; i < kFieldsCount; ++i) {
      if (new_offsets_map[i] != UINT8_MAX) {
        CallOnFieldType(
            i, MoverAndCreatorFunctor(i, type_index, new_data, old_data,
                                      offsets_, new_offsets_map));
      }
    }
    delete[] static_cast<char*>(old_data);
    bundled_data_ = new_data;
    offsets_ = new_offsets_map;
  }

  void ReleaseField(std::size_t type_index) {
    if (type_index >= kFieldsCount || offsets_[type_index] == UINT8_MAX) return;
    destructors_[type_index](
        reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(bundled_data_) +
                                offsets_[type_index] * sizeof(void*)));
    offsets_[type_index] = UINT8_MAX;
    bool all_released = true;
    for (size_t i = 0; i < kFieldsCount; ++i) {
      if (offsets_[i] != UINT8_MAX) {
        all_released = false;
        break;
      }
    }
    if (all_released) {
      delete[] static_cast<char*>(bundled_data_);
      bundled_data_ = nullptr;
    }
  }

  std::array<uint8_t, kPaddedOffsetsSize> offsets_;
  void* bundled_data_;
};

}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_BUNDLED_OPTIONAL_H_
