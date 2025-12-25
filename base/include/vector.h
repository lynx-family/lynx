// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VECTOR_H_
#define BASE_INCLUDE_VECTOR_H_

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <limits>
#include <stack>
#include <tuple>
#include <type_traits>
#include <utility>

#include "base/include/type_traits_addon.h"

#ifndef BASE_VECTOR_DISABLE_SSE2
#if defined(BASE_VECTOR_ENABLE_SSE2) || defined(__SSE2__) || \
    defined(_M_X64) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
#define BASE_VECTOR_SSE2
#endif
#endif

#ifndef BASE_VECTOR_DISABLE_NEON
#if defined(BASE_VECTOR_ENABLE_NEON) || defined(__ARM_NEON)
#define BASE_VECTOR_NEON
#endif
#endif

#ifdef BASE_VECTOR_SSE2
#include <immintrin.h>
#elif defined(BASE_VECTOR_NEON)
#include <arm_neon.h>
#if defined(__aarch64__)
#define BASE_VECTOR_NEON_ARMV8
#endif
#endif

#ifdef _MSC_VER
#include <intrin.h>  // for _BitScanForward
#endif

#ifdef DEBUG
#define BASE_VECTOR_DCHECK(...) assert(__VA_ARGS__)
#else
#define BASE_VECTOR_DCHECK(...)
#endif

#ifdef _MSC_VER
#define BASE_VECTOR_UNLIKELY(x) x
#define BASE_VECTOR_LIKELY(x) x
#else
#define BASE_VECTOR_UNLIKELY(x) __builtin_expect(!!(x), 0)
#define BASE_VECTOR_LIKELY(x) __builtin_expect(!!(x), 1)
#endif

/**
 * Inline annotations to precisely control functions for benefits of binary
 * size.
 */
#if _MSC_VER
#define BASE_VECTOR_INLINE inline __forceinline
#define BASE_VECTOR_NEVER_INLINE __declspec(noinline)
#else
#define BASE_VECTOR_INLINE inline __attribute__((always_inline))
#define BASE_VECTOR_NEVER_INLINE __attribute__((noinline))
#endif

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-template"

namespace lynx {
namespace base {

namespace {  // NOLINT

template <bool EnableCounter>
BASE_VECTOR_INLINE void* HeapAlloc(const size_t size) {
  if constexpr (EnableCounter) {
    // Call counter
  }
  return std::malloc(size);
}

BASE_VECTOR_INLINE void HeapFree(void* ptr) { std::free(ptr); }

template <typename T>
constexpr T AlignUp(T a, unsigned int align) {
  const T mask = T(align - 1);
  return T((a + mask) & ~mask);
}

BASE_VECTOR_INLINE int unchecked_count_right_zero(int x) {
#ifdef _MSC_VER
  unsigned long r;
  _BitScanForward(&r, (unsigned long)x);
  return (int)r;
#else
  return __builtin_ctz((unsigned int)x);
#endif
}

template <class T>
void _nontrivial_destruct_reverse([[maybe_unused]] T* begin,
                                  [[maybe_unused]] size_t count) {
  if constexpr (!std::is_trivially_destructible_v<T>) {
    // To be consistent with std::vector. Elements are destructed from back.
    auto end = begin + count;
    while (end != begin) {
      --end;
      end->~T();
    }
    //  for (; begin != end; ++begin) {
    //    begin->~T();
    //  }
  }
}

template <class T>
void _nontrivial_destruct_one(T* begin) {
  if constexpr (!std::is_trivially_destructible_v<T>) {
    begin->~T();
  }
}

template <class T>
BASE_VECTOR_NEVER_INLINE T* _nontrivial_move(T* dest, T* source,
                                             ptrdiff_t count) {
  auto end = source + count;
  auto step = count >= 0 ? 1 : -1;
  for (; source != end; source += step, dest += step) {
    *dest = std::move(*source);
  }
  return dest;
}

template <class T>
BASE_VECTOR_INLINE T* _nontrivial_move_forward(T* dest, T* source,
                                               size_t count) {
  return _nontrivial_move<T>(dest, source, count);
}

template <class T>
BASE_VECTOR_INLINE T* _nontrivial_move_backward(T* dest, T* source,
                                                size_t count) {
  return _nontrivial_move<T>(dest, source, -count);
}

template <class T>
BASE_VECTOR_NEVER_INLINE void _nontrivial_construct_move(T* dest, T* source,
                                                         size_t count) {
  auto end = source + count;
  for (; source != end; ++source, ++dest) {
    ::new (static_cast<void*>(dest)) T(std::move(*source));
  }
}

template <class T>
BASE_VECTOR_NEVER_INLINE void _nontrivial_construct_copy(T* dest, T* source,
                                                         size_t count) {
  auto end = source + count;
  for (; source != end; ++source, ++dest) {
    ::new (static_cast<void*>(dest)) T(*source);
  }
}
}  // namespace

/**
 * Prototype of Vector provides view of its three members.
 * This base class does not provide any method to change data.
 *
 * `ExtraBytesPerElement`: Leading extra bytes allocated along with array
 * buffer. This extra space is currently used by map/set to store hash values
 * and is transparent to Vector or InlineVector's insert, push_back or other
 * data modifier methods. But the data is copied along with array elements by
 * Vector's copy and move constructors and copy and move assign operators.
 *
 * `CountRealloc`: A flag for local performance debugging. When enabled,
 * array buffer reallocation will be counted. Note that the first allocation
 * will NOT be counted.
 */
template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
struct VectorPrototype {
  static constexpr auto is_trivial =
      IsTrivial<T, is_instance<T, std::pair>{}>::value;
  static constexpr auto is_trivially_destructible =
      std::is_trivially_destructible_v<T>;
  static constexpr auto is_trivially_relocatable =
      IsTriviallyRelocatable<T, is_instance<T, std::pair>{}>::value;
  static constexpr auto is_trivially_destructible_after_move =
      is_trivially_relocatable ||
      IsTriviallyDestructibleAfterMove<T, is_instance<T, std::pair>{}>::value;

  using iterator = T*;
  using const_iterator = const T*;

  VectorPrototype() {}

  [[maybe_unused]] size_t size() const { return count_; }

  bool empty() const { return count_ == 0; }

  size_t capacity() const { return std::abs(capacity_); }

  T* data() { return reinterpret_cast<T*>(_begin_iter()); }

  T* data() const { return reinterpret_cast<T*>(_begin_iter()); }

  bool is_static_buffer() const { return capacity_ < 0; }

  BASE_VECTOR_INLINE void* get_memory_allocate() const {
    return _buffer_alloc_address(_begin_iter(), capacity());
  }

 protected:
  template <class, size_t, bool>
  friend struct VectorPrototype;

  template <size_t, bool>
  friend struct VectorTemplateless;

  template <class, class, class, size_t, bool>
  friend struct KeyValueArray;

  BASE_VECTOR_INLINE static void* _buffer_alloc_address(const void* buffer,
                                                        size_t capacity) {
    return static_cast<uint8_t*>(const_cast<void*>(buffer)) -
           AlignUp(capacity * ExtraBytesPerElement, 8);
  }

  BASE_VECTOR_NEVER_INLINE static void* _reallocate_buffer(void* array,
                                                           size_t element_size,
                                                           size_t count) {
    auto* proto_array = reinterpret_cast<
        VectorPrototype<uint8_t, ExtraBytesPerElement, CountRealloc>*>(array);
    auto old_capacity = proto_array->capacity();
    // old*2 for consistent with std::vector of libc++.
    // But we do not use max(count, old_cap * 2).
    auto new_capacity =
        count > 0
            ? count
            : (old_capacity == 0
                   ? (element_size >= 48
                          ? 1
                          : (element_size >= 16 ? 2 : (32 / element_size)))
                   : 2 * old_capacity);
    if (BASE_VECTOR_UNLIKELY(new_capacity <= old_capacity)) {
      return nullptr;
    }
    size_t extra_bytes_size = 0;
    if constexpr (ExtraBytesPerElement > 0) {
      extra_bytes_size = AlignUp(new_capacity * ExtraBytesPerElement, 8);
    }
    const auto buffer_size = extra_bytes_size + new_capacity * element_size;
    uint8_t* buffer;
    if constexpr (CountRealloc) {
      if (old_capacity > 0) {
        // This is a RE-allocation.
        buffer = static_cast<uint8_t*>(HeapAlloc<true>(buffer_size));
      } else {
        // First time allocation, skip counter.
        buffer = static_cast<uint8_t*>(HeapAlloc<false>(buffer_size));
      }
    } else {
      buffer = static_cast<uint8_t*>(HeapAlloc<false>(buffer_size));
    }
    if (BASE_VECTOR_UNLIKELY(buffer == nullptr)) {
      return nullptr;
    }
    buffer += extra_bytes_size;  // slide extra bytes
    proto_array->_set_memory(buffer);
    proto_array->_set_capacity(new_capacity, false);
    return buffer;
  }

  BASE_VECTOR_INLINE void _copy_extra_bytes_from(const VectorPrototype& other) {
    if constexpr (ExtraBytesPerElement > 0) {
      if (other.size() > 0) {
        std::memcpy(get_memory_allocate(), other.get_memory_allocate(),
                    ExtraBytesPerElement * other.size());
      }
    }
  }

  BASE_VECTOR_NEVER_INLINE void _reallocate_nontrivial(size_t count = 0) {
    static_assert(
        !is_trivial,
        "Only for non-trivial element types you can use this method.");
    auto prev_begin = _begin_iter();
    [[maybe_unused]] void* prev_alloc_buffer;
    if constexpr (ExtraBytesPerElement > 0) {
      prev_alloc_buffer = get_memory_allocate();
    }
    bool need_free = !is_static_buffer();
    auto buffer =
        VectorPrototype<uint8_t, ExtraBytesPerElement,
                        CountRealloc>::_reallocate_buffer(this, sizeof(T),
                                                          count);
    if (buffer && prev_begin) {
      /* Leading bytes should be copied separately. */
      if constexpr (ExtraBytesPerElement > 0) {
        if (size() > 0) {
          std::memcpy(get_memory_allocate(), prev_alloc_buffer,
                      ExtraBytesPerElement * size());
        }
      }

      if constexpr (is_trivially_relocatable) {
        /* Fastest, allowing memcpy to copy all and skip destructors. */
        std::memcpy(buffer, prev_begin, sizeof(T) * size());
      } else {
        /* Move T objects one by one from old buffer to new buffer. */
        _nontrivial_construct_move((iterator)buffer, prev_begin, size());

        if constexpr (is_trivially_destructible_after_move) {
          /* Faster, skip destructors. */
        } else {
          /* Slow path, destruct objects on old buffer one by one. */
          _nontrivial_destruct_reverse(prev_begin, size());
        }
      }
      if (need_free) {
        if constexpr (ExtraBytesPerElement > 0) {
          HeapFree(prev_alloc_buffer);
        } else {
          HeapFree(prev_begin);
        }
      }
    }
  }

  void _free() {
    if (memory_ != nullptr) {
      if constexpr (!is_trivial) {
        _nontrivial_destruct_reverse(_begin_iter(), size());
      }
      if (!is_static_buffer()) {
        HeapFree(get_memory_allocate());
      }
    }
  }

  void _reset() {
    memory_ = nullptr;
    count_ = 0;
    capacity_ = 0;
  }

  BASE_VECTOR_INLINE void* _memory() const { return memory_; }

  BASE_VECTOR_INLINE void _set_memory(void* value) {
    memory_ = reinterpret_cast<T*>(value);
  }

  BASE_VECTOR_INLINE iterator _begin_iter() const {
    return reinterpret_cast<iterator>(_memory());
  }

  BASE_VECTOR_INLINE iterator _end_iter() const {
    return _begin_iter() + count_;
  }

  BASE_VECTOR_INLINE iterator _finish_iter() const {
    return _begin_iter() + capacity();
  }

  BASE_VECTOR_INLINE void _set_count(size_t value) {
    count_ = static_cast<uint32_t>(value);
  }

  BASE_VECTOR_INLINE void _inc_count(size_t count = 1) {
    count_ += static_cast<uint32_t>(count);
  }

  BASE_VECTOR_INLINE void _dec_count(size_t count = 1) {
    count_ -= static_cast<uint32_t>(count);
  }

  BASE_VECTOR_INLINE void _set_capacity(size_t value, bool is_static) {
    capacity_ =
        is_static ? -static_cast<int32_t>(value) : static_cast<int32_t>(value);
  }

  BASE_VECTOR_INLINE void _transfer_from(VectorPrototype& other) {
    // Memory buffer of other array might be an inplace buffer which is
    // unsafe to be directly transferred ownership to self.
    BASE_VECTOR_DCHECK(!other.is_static_buffer());

    memory_ = other.memory_;
    count_ = other.count_;
    capacity_ = other.capacity_;
  }

  void _swap(VectorPrototype& other) {
    // Memory buffer of other array might be an inplace buffer which is
    // unsafe to be directly transferred ownership to self.
    BASE_VECTOR_DCHECK(!is_static_buffer() && !other.is_static_buffer());

    std::swap(memory_, other.memory_);
    std::swap(count_, other.count_);
    std::swap(capacity_, other.capacity_);
  }

 private:
  T* memory_{nullptr};

  uint32_t count_{0};

  // Negative capacity value means the memory_ buffer should not be freed.
  int32_t capacity_{0};
};

/**
 APIs to manipulate Vector without template argument T.
 Note that templateless manipulators should only be used for trivial T types.
*/
template <size_t ExtraBytesPerElement, bool CountRealloc>
struct VectorTemplateless {
  BASE_VECTOR_NEVER_INLINE static void ReallocateTrivial(void* array,
                                                         size_t element_size,
                                                         size_t count) {
    auto* proto_array = reinterpret_cast<
        VectorPrototype<uint8_t, ExtraBytesPerElement, CountRealloc>*>(array);
    const bool free_prev = !proto_array->is_static_buffer();
    auto prev_begin = proto_array->_begin_iter();
    [[maybe_unused]] void* prev_alloc_buffer;
    if constexpr (ExtraBytesPerElement > 0) {
      prev_alloc_buffer = proto_array->get_memory_allocate();
    }
    auto buffer =
        VectorPrototype<uint8_t, ExtraBytesPerElement,
                        CountRealloc>::_reallocate_buffer(array, element_size,
                                                          count);
    if (buffer && prev_begin) {
      /* Leading bytes should be copied separately. */
      if constexpr (ExtraBytesPerElement > 0) {
        if (proto_array->size() > 0) {
          std::memcpy(proto_array->get_memory_allocate(), prev_alloc_buffer,
                      ExtraBytesPerElement * proto_array->size());
        }
      }

      /* Copy array data. */
      std::memcpy(buffer, prev_begin, proto_array->size() * element_size);
      if (free_prev) {
        if constexpr (ExtraBytesPerElement > 0) {
          HeapFree(prev_alloc_buffer);
        } else {
          HeapFree(prev_begin);
        }
      }
    }
  }

  /**
   Use this never-inline function to replace ReallocateTrivial full arguments
   version to avoid one default-to-0 argument at caller site. This function
   passes count==0 to grow the capacity.
   */
  BASE_VECTOR_NEVER_INLINE static void ReallocateTrivial(void* array,
                                                         size_t element_size) {
    ReallocateTrivial(array, element_size, 0);
  }

  /**
   All trivial arrays share the same Resize implementation for binary size
   benefits. Note that filling operations are done by std::memcpy which may be
   slower than std::vector.
   */
  BASE_VECTOR_NEVER_INLINE static bool Resize(void* array, size_t element_size,
                                              size_t count,
                                              const void* fill_value) {
    auto* proto_array = reinterpret_cast<
        VectorPrototype<uint8_t, ExtraBytesPerElement, CountRealloc>*>(array);
    bool reallocated = false;
    if (BASE_VECTOR_LIKELY(count > proto_array->size())) {
      if (BASE_VECTOR_LIKELY(count > proto_array->capacity())) {
        ReallocateTrivial(array, element_size, count);
        reallocated = true;
      }
      if (fill_value) {
        auto begin = proto_array->_begin_iter();
        for (size_t i = proto_array->size(); i < count; i++) {
          std::memcpy(begin + i * element_size, fill_value, element_size);
        }
      }
    }
    proto_array->_set_count(count);
    return reallocated;
  }

  /**
   This function prepares space for the data to be inserted, but does not
   perform the final insertion.
   */
  BASE_VECTOR_NEVER_INLINE static void* PrepareInsert(void* array,
                                                      size_t element_size,
                                                      const void* dest) {
    auto* proto_array = reinterpret_cast<
        VectorPrototype<uint8_t, ExtraBytesPerElement, CountRealloc>*>(array);
    auto diff = static_cast<const uint8_t*>(dest) - proto_array->_begin_iter();
    if (BASE_VECTOR_UNLIKELY(proto_array->size() == proto_array->capacity())) {
      ReallocateTrivial(array, element_size);
    }
    auto pos = proto_array->_begin_iter() + diff;
    auto new_last =
        proto_array->_begin_iter() + proto_array->size() * element_size;
    if (BASE_VECTOR_UNLIKELY(pos > new_last)) {
      BASE_VECTOR_DCHECK(false);
    } else {
      if (BASE_VECTOR_LIKELY(pos < new_last)) {
        std::memmove(pos + element_size, pos, new_last - pos);
      }
      // The actual assignment is performed after PrepareInsert() so that memcpy
      // could be optimized by compiler. std::memcpy(pos, source, element_size);
      proto_array->_inc_count();
    }
    return pos;
  }

  /**
   Erase element at index. The index is measured by element_size.
   Returns false if index or index + count is out of range.
   */
  BASE_VECTOR_NEVER_INLINE static bool Erase(void* array, size_t element_size,
                                             size_t index, size_t count = 1) {
    auto* proto_array = reinterpret_cast<
        VectorPrototype<uint8_t, ExtraBytesPerElement, CountRealloc>*>(array);
    if (BASE_VECTOR_UNLIKELY(index >= proto_array->size())) {
      return false;
    }
    if (BASE_VECTOR_UNLIKELY(index + count > proto_array->size())) {
      return false;
    }
    auto dst = proto_array->_begin_iter() + element_size * index;
    std::memmove(dst, dst + element_size * count,
                 element_size * (proto_array->size() - index - count));
    proto_array->_set_count(proto_array->size() - count);
    return true;
  }

  /**
   Push N[count] elements from [source] to Vector of address [array].
   */
  BASE_VECTOR_NEVER_INLINE static void PushBackBatch(void* array,
                                                     size_t element_size,
                                                     const void* source,
                                                     size_t count) {
    if (BASE_VECTOR_UNLIKELY(source == nullptr)) {
      return;
    }
    auto* proto_array = reinterpret_cast<
        VectorPrototype<uint8_t, ExtraBytesPerElement, CountRealloc>*>(array);
    if (proto_array->size() + count > proto_array->capacity()) {
      ReallocateTrivial(array, element_size, proto_array->size() + count);
    }
    auto end = proto_array->_begin_iter() + element_size * proto_array->size();
    std::memcpy(end, source, count * element_size);
    proto_array->_inc_count(count);
  }

  /**
   Fill elements from index position of original array with contents of
   [source].
   */
  BASE_VECTOR_NEVER_INLINE static void Fill(void* array, size_t element_size,
                                            const void* source,
                                            size_t byte_size, size_t position) {
    const auto source_count = byte_size / element_size;
    if (BASE_VECTOR_UNLIKELY(source_count == 0)) {
      return;
    }
    auto* proto_array = reinterpret_cast<
        VectorPrototype<uint8_t, ExtraBytesPerElement, CountRealloc>*>(array);
    auto count = source_count + position;
    ReallocateTrivial(array, element_size, count);
    auto dest = proto_array->_begin_iter() + position * element_size;
    if (source != nullptr) {
      std::memcpy(dest, source, source_count * element_size);
    } else {
      std::memset(dest, 0, source_count * element_size);
    }
    proto_array->_set_count(count);
  }
};

/**
 * Replacement of Vector for binary size benefits. This linear container
 * provides basic methods of Vector with the same method signatures. For
 * POD types, it also provides some non-stl-standard methods for convenience.
 */
template <class T, size_t ExtraBytesPerElement = 0, bool CountRealloc = false>
struct Vector
    : protected VectorTemplateless<ExtraBytesPerElement, CountRealloc>,
      public VectorPrototype<T, ExtraBytesPerElement, CountRealloc> {
  using
      typename VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::iterator;
  using typename VectorPrototype<T, ExtraBytesPerElement,
                                 CountRealloc>::const_iterator;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::is_trivial;
  using VectorPrototype<T, ExtraBytesPerElement,
                        CountRealloc>::is_trivially_destructible;
  using VectorPrototype<T, ExtraBytesPerElement,
                        CountRealloc>::is_trivially_destructible_after_move;
  using VectorPrototype<T, ExtraBytesPerElement,
                        CountRealloc>::is_trivially_relocatable;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::size;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::capacity;
  using VectorPrototype<T, ExtraBytesPerElement,
                        CountRealloc>::is_static_buffer;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_begin_iter;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_end_iter;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_finish_iter;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_free;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_transfer_from;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_swap;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_reset;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_set_count;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_inc_count;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_dec_count;
  using VectorPrototype<T, ExtraBytesPerElement,
                        CountRealloc>::_reallocate_nontrivial;
  using VectorPrototype<T, ExtraBytesPerElement,
                        CountRealloc>::_copy_extra_bytes_from;

  using VectorTemplateless<ExtraBytesPerElement,
                           CountRealloc>::ReallocateTrivial;
  using VectorTemplateless<ExtraBytesPerElement, CountRealloc>::Resize;
  using VectorTemplateless<ExtraBytesPerElement, CountRealloc>::PrepareInsert;
  using VectorTemplateless<ExtraBytesPerElement, CountRealloc>::PushBackBatch;
  using VectorTemplateless<ExtraBytesPerElement, CountRealloc>::Fill;

  using size_type = size_t;
  using difference_type = ptrdiff_t;
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using unique_id_type = uintptr_t;

  /**
   * @brief We allows 'array = nullptr' to reset and clear memory of array.
   */
  Vector(std::nullptr_t) {}  // NOLINT
  Vector() {}
  explicit Vector(size_t count) { _construct_fill_default(count); }

  Vector(size_t count, const T& value) { _construct_fill(count, value); }

  /**
   * @brief Create array by size and data, if parameter 'data' is valid, copy it
   * to array.
   * @param count element count of array
   * @param data source data. If non-null, will copy from data.
   */
  template <class U = T, typename = typename std::enable_if<
                             std::is_same_v<U, T> && is_trivial>::type>
  Vector(size_t count, const void* data) {
    fill(data, count * sizeof(T));
  }

  Vector(std::initializer_list<T> list) {
    _from(list.begin(), list.size(), _nontrivial_construct_move);
  }

  /**
   * @brief We only support construct array from array iterators because we can
   * calculate distance from begin to end. std::vector supports being
   * constructed from other iterators such as std::set's, but it is not
   * efficient because we cannot precalculate capacity of the array.
   */
  Vector(const_iterator begin, const_iterator end) {
    _from(begin, end - begin, _nontrivial_construct_copy);
  }

  Vector(iterator begin, iterator end) {
    _from(begin, end - begin, _nontrivial_construct_copy);
  }

  Vector(const Vector& other) {
    _from(other.data(), other.size(), _nontrivial_construct_copy);
    _copy_extra_bytes_from(other);
  }

  Vector& operator=(const Vector& other) {
    if (BASE_VECTOR_LIKELY(this != &other)) {
      clear();
      _from(other.data(), other.size(), _nontrivial_construct_copy);
      _copy_extra_bytes_from(other);
    }
    return *this;
  }

  Vector& operator=(std::initializer_list<T> list) {
    clear();
    _from(list.begin(), list.size(), _nontrivial_construct_move);
    return *this;
  }

  Vector(Vector&& other) noexcept {
    if (other.is_static_buffer()) {
      _from(other.data(), other.size(), _nontrivial_construct_move);
      _copy_extra_bytes_from(other);
      other.clear();
    } else {
      _transfer_from(other);
      other._reset();
    }
  }

  Vector& operator=(Vector&& other) {
    if (BASE_VECTOR_LIKELY(this != &other)) {
      if (other.is_static_buffer()) {
        clear();
        _from(other.data(), other.size(), _nontrivial_construct_move);
        _copy_extra_bytes_from(other);
        other.clear();
      } else {
        _free();
        _transfer_from(other);
        other._reset();
      }
    }
    return *this;
  }

  ~Vector() { _free(); }

  template <class U>
  reference push_back(U&& v) {
    return emplace_back(std::forward<U>(v));
  }

  template <class... Args>
  reference emplace_back(Args&&... args) {
    _grow_if_need();
    auto end = _end_iter();
    ::new (static_cast<void*>(end)) T(std::forward<Args>(args)...);
    _inc_count();
    return *end;
  }

  void pop_back() {
    if (size() == 0) {
      return;
    }
    if constexpr (!is_trivial) {
      _nontrivial_destruct_one(_end_iter() - 1);
    }
    _dec_count();
  }

  const_reference back() const { return *(_end_iter() - 1); }

  reference back() { return *(_end_iter() - 1); }

  const_reference front() const { return *_begin_iter(); }

  reference front() { return *_begin_iter(); }

  const_reference operator[](size_t n) const {
    BASE_VECTOR_DCHECK(n < size());
    return *(_begin_iter() + n);
  }

  reference operator[](size_t n) {
    BASE_VECTOR_DCHECK(n < size());
    return *(_begin_iter() + n);
  }

  const_reference at(size_t n) const {
    BASE_VECTOR_DCHECK(n < size());
    return *(_begin_iter() + n);
  }  // Always nothrow

  reference at(size_t n) {
    BASE_VECTOR_DCHECK(n < size());
    return *(_begin_iter() + n);
  }  // Always nothrow

  /**
   * @brief Returns pointer to the underlying array serving as element storage.
   * @return If size() is ​0​, data() may or may not return a null pointer.
   */
  template <typename U = T>
  U* data() {
    return reinterpret_cast<U*>(_begin_iter());
  }

  /**
   * @brief Returns pointer to the underlying array serving as element storage.
   * @return If size() is ​0​, data() may or may not return a null pointer.
   */
  template <typename U = T>
  const U* data() const {
    return reinterpret_cast<const U*>(_begin_iter());
  }

  iterator begin() { return _begin_iter(); }

  const_iterator begin() const { return _begin_iter(); }

  const_iterator cbegin() const { return _begin_iter(); }

  iterator end() { return _end_iter(); }

  const_iterator end() const { return _end_iter(); }

  const_iterator cend() const { return _end_iter(); }

  reverse_iterator rbegin() { return reverse_iterator(_end_iter()); }

  const_reverse_iterator rbegin() const {
    return const_reverse_iterator(_end_iter());
  }

  const_reverse_iterator crbegin() const {
    return const_reverse_iterator(_end_iter());
  }

  reverse_iterator rend() { return reverse_iterator(_begin_iter()); }

  const_reverse_iterator rend() const {
    return const_reverse_iterator(_begin_iter());
  }

  const_reverse_iterator crend() const {
    return const_reverse_iterator(_begin_iter());
  }

  iterator erase(std::nullptr_t) = delete;  // Avoid misuse of erase(0)
  iterator erase(std::nullptr_t,
                 std::nullptr_t) = delete;  // Avoid misuse of erase(0)
  iterator erase(iterator pos) { return erase(pos, pos + 1); }

  iterator erase(const_iterator pos) { return erase((iterator)pos); }

  iterator erase(iterator first, iterator last) {
    BASE_VECTOR_DCHECK(first <= last);
    if (BASE_VECTOR_LIKELY(first != last)) {
      if constexpr (is_trivial || is_trivially_relocatable) {
        std::memmove((void*)first, last, (_end_iter() - last) * sizeof(T));
      } else {
        // Slow path, move elements one by one and skip destructors if possible.
        [[maybe_unused]] auto it =
            _nontrivial_move_forward(first, last, _end_iter() - last);
        if constexpr (!is_trivially_destructible_after_move) {
          _nontrivial_destruct_reverse(it, last - first);
        }
      }
      _dec_count(last - first);
    }
    return first;
  }

  template <class... Args>
  iterator emplace(const_iterator pos, Args&&... args) {
    if constexpr (is_trivial) {
      // For trivial types, always call templateless implementation for binary
      // size benefits.
      auto it = (iterator)PrepareInsert(this, sizeof(T), pos);
      ::new (static_cast<void*>(it)) T(std::forward<Args>(args)...);
      return it;
    } else {
      auto pair = _nontrivial_prepare_insert(pos);
      if constexpr (!is_trivially_destructible) {
        if constexpr (!is_trivially_relocatable) {
          // If T is_trivially_relocatable, we can skip destructor because in
          // _nontrivial_prepare_insert the element at `pair.first` is moved
          // away by `memmove`.
          if (!pair.second) {
            // Element at dest_pos was moved but needs destruction.
            pair.first->~T();
          }
        }
      }
      // Construct at dest_pos.
      ::new (static_cast<void*>(pair.first)) T(std::forward<Args>(args)...);
      return pair.first;
    }
  }

  template <class U>
  iterator insert(std::nullptr_t,
                  U&& value) = delete;  // Avoid misuse of insert(0)

  BASE_VECTOR_INLINE iterator insert(const_iterator pos, const T& value) {
    return emplace(pos, value);
  }

  BASE_VECTOR_INLINE iterator insert(const_iterator pos, T&& value) {
    return emplace(pos, std::move(value));
  }

  /**
   * @brief Reserve capacity.
   * @return True if reallocation occurs.
   */
  bool reserve(size_t count) {
    if (BASE_VECTOR_LIKELY(count > capacity())) {
      _reallocate(count);
      return true;
    }
    return false;
  }

  void clear() {
    if constexpr (!is_trivial) {
      _nontrivial_destruct_reverse(_begin_iter(), size());
    }
    // The capacity should not change.
    _set_count(0);
  }

  void clear_and_shrink() {
    _free();
    _reset();
  }

  void shrink_to_fit() {
    if (is_static_buffer()) {
      return;
    }
    if (BASE_VECTOR_LIKELY(capacity() > size())) {
      // This is not optimal for InlineVector if current size fits into inline
      // buffer.
      Vector tmp;
      tmp._from(data(), size(), _nontrivial_construct_move);
      tmp._copy_extra_bytes_from(*this);
      _swap(tmp);
    }
  }

  void swap(Vector& other) {
    if (is_static_buffer() || other.is_static_buffer()) {
      // If any one is using inline static buffer, we cannot simply swap the
      // pointers.
      Vector tmp(*this);
      *this = other;
      other = tmp;
    } else {
      _swap(other);
    }
  }

  /**
   * @brief Resize the array to desired length.
   *  template<bool fill>:  If you want to set different values for new elements
   *  after resize, you can pass fill as false to wipe out unnecessary
   * assignment operations. This template argument is required for performance
   * benefits.
   *
   * @param count Desired length.
   * @return True if reallocation occurs.
   */
  template <bool fill, class U = T>
  typename std::enable_if<std::is_same_v<U, T> && is_trivial && fill,
                          bool>::type
  resize(size_t count) {
    if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T> ||
                  std::is_pointer_v<T>) {
      // For efficiency, merge to single memset instead of std::memcpy in
      // Resize.
      bool reallocated = false;
      if (count > size()) {
        if (count > capacity()) {
          ReallocateTrivial(this, sizeof(T), count);
          reallocated = true;
        }
        std::memset(begin() + size(), 0, (count - size()) * sizeof(T));
      }
      _set_count(count);
      return reallocated;
    } else {
      if (count > size()) {
        T value;
        return Resize(this, sizeof(T), count, &value);
      } else {
        _set_count(count);
        return false;
      }
    }
  }

  template <bool fill, class U = T>
  typename std::enable_if<std::is_same_v<U, T> && is_trivial && fill,
                          bool>::type
  resize(size_t count, const T& value) {
    if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T> ||
                  std::is_pointer_v<T>) {
      // For efficiency, merge to single memset if value is T() instead of
      // std::memcpy in Resize.
      bool reallocated = false;
      if (count > size()) {
        if (count > capacity()) {
          ReallocateTrivial(this, sizeof(T), count);
          reallocated = true;
        }
        if (value == T()) {
          std::memset(begin() + size(), 0, (count - size()) * sizeof(T));
        } else {
          std::fill(begin() + size(), begin() + count, value);
        }
      }
      _set_count(count);
      return reallocated;
    } else {
      return Resize(this, sizeof(T), count, &value);
    }
  }

  template <bool fill, class U = T>
  typename std::enable_if<std::is_same_v<U, T> && is_trivial && !fill,
                          bool>::type
  resize(size_t count) {
    return Resize(this, sizeof(T), count, nullptr);
  }

  /**
   * \brief It is recommended to call grow(N) for nontrivial type if you are
   * surely to expand the array. Because resize() method specialize a branch to
   * erase items.
   */
  template <class U = T>
  typename std::enable_if<std::is_same_v<U, T> && !is_trivial &&
                              std::is_copy_constructible_v<U>,
                          bool>::type
  resize(size_t count, const T& value) {
    bool reallocated = false;
    if (count > size()) {
      if (count > capacity()) {
        _reallocate(count);
        reallocated = true;
      }
      _fill(_end_iter(), _begin_iter() + count, value);
      _set_count(count);
    } else {
      erase(_begin_iter() + count, _end_iter());
    }
    return reallocated;
  }

  /**
   * \brief It is recommended to call grow(N) for nontrivial type if you are
   * surely to expand the array. Because resize() method specialize a branch to
   * erase items.
   */
  template <class U = T>
  typename std::enable_if<std::is_same_v<U, T> && !is_trivial, bool>::type
  resize(size_t count) {
    bool reallocated = false;
    if (count > size()) {
      if (count > capacity()) {
        _reallocate(count);
        reallocated = true;
      }
      _fill(_end_iter(), _begin_iter() + count);
      _set_count(count);
    } else {
      erase(_begin_iter() + count, _end_iter());
    }
    return reallocated;
  }

  /**
   * \brief Grow by increment length and returns the last object.
   * The difference between grow and push_back/emplace_back is that for trivial
   * type grow method does not do any copy or construction. For non-trivial
   * type, grow only construct by default.
   */
  reference grow() {
    _grow_if_need();
    if constexpr (!is_trivial) {
      auto end = _end_iter();
      ::new (static_cast<void*>(end)) T();
      _inc_count();
      return *end;
    } else {
      _inc_count();
      return back();
    }
  }

  template <class U = T>
  typename std::enable_if<std::is_same_v<U, T> && is_trivial>::type grow(
      size_t count) {
    BASE_VECTOR_DCHECK(count >= size());
    resize<false>(count);
  }

  template <class U = T>
  typename std::enable_if<std::is_same_v<U, T> && !is_trivial>::type grow(
      size_t count) {
    if (count > size()) {
      if (count > capacity()) {
        _reallocate(count);
      }
      _fill(_end_iter(), _begin_iter() + count);
      _set_count(count);
    } else if (count < size()) {
      BASE_VECTOR_DCHECK(false);
    }
  }

  /**
   * @brief Use data to fill array buffer. The size of array will be reset to
   *  byte_size / sizeof(T) + position.
   *
   * @param data Data source pointer. If null, buffer will be filled by 0.
   * @param byte_size Data source byte length.
   * @param position Optional, from which index of T to write.
   */
  template <class U = T>
  typename std::enable_if<std::is_same_v<U, T> && is_trivial>::type fill(
      const void* data, size_type byte_size, size_type position = 0) {
    Fill(this, sizeof(T), data, byte_size, position);
  }

  /**
   * @brief Append data buffer to end of this array.
   * @param data Data source pointer. If null, buffer will be filled by 0.
   * @param byte_size Data source byte length.
   */
  template <class U = T>
  typename std::enable_if<std::is_same_v<U, T> && is_trivial>::type append(
      const void* data, size_type byte_size) {
    fill(data, byte_size, size());
  }

  /**
   * @brief Append data buffer of another array to end of this array.
   * @param other Another array which may differ in element type.
   */
  template <class W = T, class U>
  typename std::enable_if<std::is_same_v<W, T> && is_trivial>::type append(
      const Vector<U, ExtraBytesPerElement, CountRealloc>& other) {
    if (!other.empty()) {
      fill(other.data(), other.size() * sizeof(U), size());
    }
  }

  /**
   * @brief Sugar for_each method which provides only T or T& in callback.
   */
  template <typename Callback>
  void for_each(Callback&& callback) {
    const auto count = size();
    for (uint32_t i = 0; i < count; i++) {
      callback((*this)[i]);
    }
  }

  class Unsafe {
    // ATTENTION: function under this class is UNSAFE to use.
    // Do NOT use them unless you have consulted with the owners of Vector.
   public:
    Unsafe() = delete;

    // Force to set size of vector to target value. It is usually called for a
    // vector that is about to be destroyed and sets the size to 0 to avoid
    // calling the destructor method of the internal objects that have been
    // relocated.
    static void SetSize(Vector& vector, size_t value) {
      vector._set_count(value);
    }
  };

 protected:
  friend class Unsafe;

  template <class U>
  struct always_false : std::false_type {};

  void _reallocate(size_t value) {
    if constexpr (is_trivial || is_trivially_relocatable) {
      ReallocateTrivial(this, sizeof(T), value);
    } else {
      _reallocate_nontrivial(value);
    }
  }

  BASE_VECTOR_INLINE void _grow_if_need() {
    if (BASE_VECTOR_UNLIKELY(size() == capacity())) {
      // _grow_if_need is inlined at caller side. Although we can call
      // _reallocate(0) but implement standalone for binary size optimization.
      if constexpr (is_trivial || is_trivially_relocatable) {
        ReallocateTrivial(this, sizeof(T));
      } else {
        _reallocate_nontrivial();
      }
    }
  }

  void _from(const void* src, size_t size,
             [[maybe_unused]] void (*func)(iterator, iterator, size_t)) {
    if (size > 0) {
      if constexpr (is_trivial) {
        PushBackBatch(this, sizeof(T), src, size);
      } else {
        reserve(size);
        func(_begin_iter(), reinterpret_cast<iterator>(const_cast<void*>(src)),
             size);
        _set_count(size);
      }
    }
  }

  void _fill(iterator begin, iterator end, const T& v) {
    for (; begin != end; begin++) {
      ::new (static_cast<void*>(begin)) T(v);
    }
  }

  void _fill(iterator begin, iterator end) {
    for (; begin != end; begin++) {
      ::new (static_cast<void*>(begin)) T();
    }
  }

  void _construct_fill_default(size_t count) {
    if (count > 0) {
      if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T> ||
                    std::is_pointer_v<T>) {
        fill(nullptr, count * sizeof(T));  // Resize and set all bits to 0.
      } else if constexpr (is_trivial) {
        resize<true>(count);
      } else {
        // Do not use `resize` directly because for non-trivial types, the
        // `resize` method instantiates `erase` method.
        _reallocate(count);
        _fill(_end_iter(), _begin_iter() + count);
        _set_count(count);
      }
    }
  }

  void _construct_fill(size_t count, const T& value) {
    if (count > 0) {
      if constexpr (std::is_integral_v<T> || std::is_floating_point_v<T> ||
                    std::is_pointer_v<T>) {
        if (value == T()) {
          fill(nullptr, count * sizeof(T));  // Resize and set all bits to 0.
        } else {
          resize<false>(count);
          std::fill(begin(), end(), value);
        }
      } else if constexpr (is_trivial) {
        resize<true>(count, value);
      } else {
        // Do not use `resize` directly because for non-trivial types, the
        // `resize` method instantiates `erase` method.
        _reallocate(count);
        _fill(_end_iter(), _begin_iter() + count, value);
        _set_count(count);
      }
    }
  }

  // Returns pair<pos, true_if_construct_at_end>
  BASE_VECTOR_INLINE std::pair<iterator, bool> _nontrivial_prepare_insert(
      const_iterator pos) {
    auto diff = pos - _begin_iter();
    _grow_if_need();
    auto dest_pos = _begin_iter() + diff;
    auto new_last = _end_iter();
    bool construct_at_end = false;
    if (BASE_VECTOR_UNLIKELY(dest_pos > new_last)) {
      BASE_VECTOR_DCHECK(false);
    } else {
      if (dest_pos < new_last) {
        if constexpr (is_trivially_relocatable) {
          // Fast path, use memmove to shift all elements to next slot.
          std::memmove((void*)(dest_pos + 1), dest_pos,
                       sizeof(T) * (new_last - dest_pos));
        } else {
          // Slow path, construct new T at end, move previous back item to it.
          _nontrivial_construct_move(new_last, new_last - 1, 1);
          // Move left objects.
          _nontrivial_move_backward(new_last - 1, new_last - 2,
                                    new_last - dest_pos - 1);
        }
      } else {
        construct_at_end = true;
      }
      _inc_count();
    }
    return {dest_pos, construct_at_end};
  }
};

template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
inline bool operator==(
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __x,
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __y) {
  const typename Vector<T, ExtraBytesPerElement, CountRealloc>::size_type __sz =
      __x.size();
  if (__sz != __y.size()) {
    return false;
  }
  if (!std::equal(__x.begin(), __x.end(), __y.begin())) {
    return false;
  }
  if constexpr (ExtraBytesPerElement > 0) {
    // Also compares extra bytes.
    if (__sz > 0) {
      if (std::memcmp(__x.get_memory_allocate(), __y.get_memory_allocate(),
                      __sz * ExtraBytesPerElement) != 0) {
        return false;
      }
    }
  }
  return true;
}

template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
inline bool operator!=(
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __x,
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __y) {
  return !(__x == __y);
}

template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
inline bool operator<(
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __x,
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __y) {
  return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(),
                                      __y.end());
}

template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
inline bool operator>(
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __x,
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __y) {
  return __y < __x;
}

template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
inline bool operator>=(
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __x,
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __y) {
  return !(__x < __y);
}

template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
inline bool operator<=(
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __x,
    const Vector<T, ExtraBytesPerElement, CountRealloc>& __y) {
  return !(__y < __x);
}

using ByteArray = Vector<uint8_t, 0>;

/**
 * @brief Sugar for construction ByteArray from static primitive array.
 * ByteArray a = ByteArrayFromBuffer((float[]) {0, 1, 1, 1, 1, 0, 0, 0});
 */
template <typename T, size_t Num>
ByteArray ByteArrayFromBuffer(const T (&data)[Num]) {
  return ByteArray(sizeof(data), data);
}

/**
 * @brief A resizable array type initialized with capacity of N and the buffer
 * is inplace following up the array struct. When element count exceeds N, a new
 * external buffer will be allocated and the inplace_buffer_ will be wasted.
 */
template <class T, size_t N, size_t ExtraBytesPerElement = 0,
          bool CountRealloc = false>
struct InlineVector : public Vector<T, ExtraBytesPerElement, CountRealloc> {
  using
      typename VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::iterator;
  using typename VectorPrototype<T, ExtraBytesPerElement,
                                 CountRealloc>::const_iterator;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::is_trivial;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::size;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::capacity;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_set_memory;
  using VectorPrototype<T, ExtraBytesPerElement, CountRealloc>::_set_capacity;
  using VectorPrototype<T, ExtraBytesPerElement,
                        CountRealloc>::_copy_extra_bytes_from;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::_begin_iter;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::_end_iter;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::reserve;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::clear;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::clear_and_shrink;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::_from;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::_construct_fill;
  using Vector<T, ExtraBytesPerElement, CountRealloc>::_construct_fill_default;

  static constexpr size_t kInlinedSize = N;

  InlineVector() : Vector<T, ExtraBytesPerElement, CountRealloc>() {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
  }

  InlineVector(std::nullptr_t) : InlineVector() {}  // NOLINT

  explicit InlineVector(size_t count) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    _construct_fill_default(count);
  }

  InlineVector(size_t count, const T& value) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    _construct_fill(count, value);
  }

  InlineVector(const_iterator begin, const_iterator end) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    _from(begin, end - begin, _nontrivial_construct_copy);
  }

  InlineVector(iterator begin, iterator end) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    _from(begin, end - begin, _nontrivial_construct_copy);
  }

  InlineVector(std::initializer_list<T> list)
      : Vector<T, ExtraBytesPerElement, CountRealloc>() {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    *this = std::move(list);
  }

  InlineVector(
      const Vector<T, ExtraBytesPerElement, CountRealloc>& other) {  // NOLINT
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    *this = other;
  }

  InlineVector(const InlineVector& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    *this = other;
  }

  template <size_t N2>
  InlineVector(
      const InlineVector<T, N2, ExtraBytesPerElement, CountRealloc>& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    *this = other;
  }

  InlineVector(
      Vector<T, ExtraBytesPerElement, CountRealloc>&& other) {  // NOLINT
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    *this = std::move(other);
  }

  InlineVector(InlineVector&& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    *this = std::move(other);
  }

  template <size_t N2>
  InlineVector(
      InlineVector<T, N2, ExtraBytesPerElement, CountRealloc>&& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _init_static();
    *this = std::move(other);
  }

  InlineVector& operator=(std::initializer_list<T> list) {
    reinterpret_cast<Vector<T, ExtraBytesPerElement, CountRealloc>*>(this)->
    operator=(std::move(list));
    return *this;
  }

  InlineVector& operator=(
      const Vector<T, ExtraBytesPerElement, CountRealloc>& other) {
    reinterpret_cast<Vector<T, ExtraBytesPerElement, CountRealloc>*>(this)->
    operator=(other);
    return *this;
  }

  InlineVector& operator=(const InlineVector& other) {
    reinterpret_cast<Vector<T, ExtraBytesPerElement, CountRealloc>*>(this)->
    operator=(other);
    return *this;
  }

  template <size_t N2>
  InlineVector& operator=(
      const InlineVector<T, N2, ExtraBytesPerElement, CountRealloc>& other) {
    reinterpret_cast<Vector<T, ExtraBytesPerElement, CountRealloc>*>(this)->
    operator=(other);
    return *this;
  }

  InlineVector& operator=(
      Vector<T, ExtraBytesPerElement, CountRealloc>&& other) {
    if (this != &other) {
      if (other.size() > capacity()) {
        if (!other.is_static_buffer()) {
          // Just swap pointers.
          reinterpret_cast<Vector<T, ExtraBytesPerElement, CountRealloc>*>(this)
              ->
              operator=(std::move(other));
          return *this;
        } else {
          reserve(other.size());
        }
      }

      // Trying best to use inplace buffer, move items of other to self.
      clear();
      _from(other.begin(), other.size(), _nontrivial_construct_move);
      _copy_extra_bytes_from(other);
      other.clear();
    }
    return *this;
  }

  InlineVector& operator=(InlineVector&& other) {
    return operator=(std::move(
        *reinterpret_cast<Vector<T, ExtraBytesPerElement, CountRealloc>*>(
            &other)));
  }

  template <size_t N2>
  InlineVector& operator=(
      InlineVector<T, N2, ExtraBytesPerElement, CountRealloc>&& other) {
    return operator=(std::move(
        *reinterpret_cast<Vector<T, ExtraBytesPerElement, CountRealloc>*>(
            &other)));
  }

  void clear_and_shrink() {
    Vector<T, ExtraBytesPerElement, CountRealloc>::clear_and_shrink();
    _init_static();
  }

 private:
  alignas(std::max(alignof(T), sizeof(void*))) uint8_t
      inplace_buffer_[AlignUp(N * ExtraBytesPerElement, 8) + sizeof(T) * N];

  void _init_static() {
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[AlignUp(N * ExtraBytesPerElement, 8)]);
  }
};

/**
 * @brief Stack using Vector<T> or InlineVector<T, N> as underlying container.
 */
template <class T>
using Stack = std::stack<T, Vector<T>>;

template <class T, size_t N>
using InlineStack = std::stack<T, InlineVector<T, N>>;

template <bool Flag>
struct MapStatisticsBase;

enum class MapStatisticsFindKind {
  kFind,                 // find(), contains() or [] to find a value
  kInsertFindCollision,  // key found before insert
  kInsertFind,           // key not found before insert
};

template <>
struct MapStatisticsBase<true> {
  mutable uint32_t max_count{0};
  mutable uint32_t insert_count{0};
  mutable uint32_t erase_count{0};
  mutable uint32_t total_find_count{0};
  mutable uint32_t find_count{0};
  mutable uint32_t insert_find_collision_count{0};

  ~MapStatisticsBase();

  void UpdateMaxCount(size_t v) const;

  void IncreaseInsertCount() const { insert_count++; }

  void IncreaseEraseCount() const { erase_count++; }

  void RecordFind(MapStatisticsFindKind kind, size_t find_of_count) const;

  static void IncreaseFindOfCount(MapStatisticsFindKind kind,
                                  size_t find_of_count);
};

template <>
struct MapStatisticsBase<false> {
  void UpdateMaxCount([[maybe_unused]] size_t v) const {}
  void IncreaseInsertCount() const {}
  void IncreaseEraseCount() const {}
  void RecordFind([[maybe_unused]] MapStatisticsFindKind kind,
                  [[maybe_unused]] size_t find_of_count) const {}
};

#define BASE_VECTOR_HAS_MEMBER_OF_NAME(N)                                      \
  template <typename T, typename = void>                                       \
  struct has_##N##_member : std::false_type {};                                \
  template <typename T>                                                        \
  struct has_##N##_member<T, std::void_t<decltype(T::N)>> : std::true_type {}; \
  template <typename T>                                                        \
  inline constexpr bool has_##N##_member_v = has_##N##_member<T>::value

#define BASE_VECTOR_HAS_TYPE_MEMBER_OF_NAME(N)                                \
  template <typename T, typename = void>                                      \
  struct has_##N##_member : std::false_type {};                               \
  template <typename T>                                                       \
  struct has_##N##_member<T, std::void_t<typename T::N>> : std::true_type {}; \
  template <typename T>                                                       \
  inline constexpr bool has_##N##_member_v = has_##N##_member<T>::value

BASE_VECTOR_HAS_MEMBER_OF_NAME(use_hash);
BASE_VECTOR_HAS_MEMBER_OF_NAME(consecutive_key);
BASE_VECTOR_HAS_MEMBER_OF_NAME(assign_existing_for_merge);
BASE_VECTOR_HAS_TYPE_MEMBER_OF_NAME(hash);
BASE_VECTOR_HAS_TYPE_MEMBER_OF_NAME(equal);
BASE_VECTOR_HAS_TYPE_MEMBER_OF_NAME(equal_when_hash_equal);

#undef BASE_VECTOR_HAS_MEMBER_OF_NAME
#undef BASE_VECTOR_HAS_TYPE_MEMBER_OF_NAME

// Only stores 32bit hash value for SIMD/NEON instructions.
using KeyPolicyReducedHashValueType = uint32_t;

template <class K, class Hash = std::hash<K>>
struct ReducedHash {
  using Type = KeyPolicyReducedHashValueType;
  using OriginalHash = Hash;

  static constexpr auto ByReinterpret =
      (sizeof(K) <= sizeof(Type)) &&
      (std::is_enum_v<K> || std::is_integral_v<K>);

  using BestTypeForOperator =
      std::conditional_t<(sizeof(K) <= sizeof(void*)) &&
                             std::is_trivially_copyable_v<K>,
                         K, const K&>;

  Type operator()(BestTypeForOperator v) const {
    if constexpr (sizeof(K) <= sizeof(Type)) {
      // Reinterpret K with unsigned zero extend when K is integer type or enum
      // type and fits into Type.
      if constexpr (std::is_enum_v<K>) {
        using EnumIntType = std::underlying_type_t<K>;
        using UnsignedEnumIntType = std::make_unsigned_t<EnumIntType>;
        UnsignedEnumIntType num_value = static_cast<UnsignedEnumIntType>(v);
        return static_cast<Type>(num_value);
      } else if constexpr (std::is_integral_v<K>) {
        using UnsignedIntType = std::make_unsigned_t<K>;
        UnsignedIntType num_value = static_cast<UnsignedIntType>(v);
        return static_cast<Type>(num_value);
      }
    }
    // Calculate original hash value and reduce.
    return static_cast<Type>(OriginalHash()(v));
  }

  // If K can be directly reinterpreted as uint32_t, EqualWhenHashEqual always
  // returns true.
  struct AlwaysEqualWhenHashEqual {
    bool operator()([[maybe_unused]] K x, [[maybe_unused]] K y) const {
      return true;
    }
  };
};

/**
 * The KeyPolicy type is used to speed up key lookups. For example, you can
 * provide a hash method for the key type. When implementing Map/Set, internal
 * lookups can use hash values for quick judgment. Currently, this only works
 * for Map/Set implemented by linear search.
 *
 * `Hash` is optional. You could also provide a policy like `KeyPolicy<K, void,
 * CustomEqual>` to customize the equality check function such as case
 * insensitive comparision without using hash.
 *
 * `EqualWhenHashEqual` is optional. When provided, it would be used to check if
 * two keys are strict equal after their hash values are known to be equal.
 * Sometimes you can use this to do tiny optimizations. For example, if the
 * `operator==` method of the K type already has a hash check, then when the
 * hashes are known to be equal, the hash check can be avoided and other
 * attributes can be checked to save one integer equality check and branch.
 * For another example, if the hash value of the K is itself, EqualWhenHashEqual
 * can directly return true which will be optimized out.
 */
template <class K, class Hash = std::hash<K>, class Equal = std::equal_to<K>,
          class EqualWhenHashEqual = Equal>
struct KeyPolicy {
  static constexpr auto use_hash = !std::is_void_v<Hash>;

  using hash = Hash;
  using equal = Equal;
  using equal_when_hash_equal = EqualWhenHashEqual;
};

/**
 * This policy enables KeyPolicy for LinearFlatMap by default, which means using
 * hash for search optimization. If the K type contains the
 * `equal_when_hash_equal` structure, it will be used as an optimized method to
 * determine whether two K objects are completely equal when their hashes are
 * already equal. If K can be directly reinterpreted as uint32_t, use
 * ReducedHash.
 */
template <class K>
struct ReducedHashKeyPolicy {
 public:
  static constexpr auto use_hash = true;

  using hash = ReducedHash<K, std::hash<K>>;
  using equal = std::equal_to<K>;

 private:
  template <typename AnyType>
  struct type_identity {
    using type = AnyType;
  };

  template <bool Explicit>
  struct EqualWhenHashEqualHelper {
    static auto select() {
      if constexpr (Explicit) {
        return type_identity<typename K::equal_when_hash_equal>{};
      } else if constexpr (hash::ByReinterpret) {
        return type_identity<typename hash::AlwaysEqualWhenHashEqual>{};
      } else {
        return type_identity<equal>{};
      }
    }
  };

 public:
  using equal_when_hash_equal =
      typename decltype(EqualWhenHashEqualHelper<
                        has_equal_when_hash_equal_member_v<K>>::select())::type;
};

/**
 * This is a key policy for map and to optimize key of integer types.
 * By default, the map stores `std::pair<K, V>`, and the data can be accessed
 * through `it->first` and `it->second` during iteration. When
 * `MapKeyPolicyConsecutiveIntegers` is used, keys and values are stored
 * independently and continuously. In short, key values are stored like
 * "K-K-K-V-V-V..." instead of "K-V-K-V-K-V..." and iterators only return
 * values. Requirements:
 * 1. K is trivial and fits into 32 bits. uint32_t, int16_t, enums are
 * supported.
 * 2. hash(key) == key.
 * 3. Only for maps.
 */
template <class K>
struct MapKeyPolicyConsecutiveIntegers {
  static_assert(ReducedHash<K>::ByReinterpret,
                "Using MapKeyPolicyConsecutiveIntegers, the key type K must be "
                "integer or enum which fit into 32 bits.");

  static constexpr auto use_hash = true;
  static constexpr auto consecutive_key = true;

  using hash = ReducedHash<K>;
  using equal = std::equal_to<K>;
  using equal_when_hash_equal = typename hash::AlwaysEqualWhenHashEqual;

  template <class T, bool Const>
  struct Reference {
    using key_iterator = const KeyPolicyReducedHashValueType*;
    using value_iterator = std::conditional_t<Const, const T*, T*>;
    using value_reference = std::conditional_t<Const, const T&, T&>;

    // Mimic the behavior of ordinary value type `std::pair<K, T>`.
    const K& first;
    value_reference second;

    Reference(key_iterator key_it, value_iterator value_it)
        : first(*reinterpret_cast<const K*>(key_it)), second(*value_it) {}
  };

  template <class T, bool Const>
  struct RangeLoopIterator {
    using key_iterator = const KeyPolicyReducedHashValueType*;
    using value_iterator = std::conditional_t<Const, const T*, T*>;
    using value_reference = std::conditional_t<Const, const T&, T&>;

    struct PointerView {
      key_iterator first;
      value_iterator second;
    };

    // Mimic the behavior of ordinary value type `std::pair<K, T>`.
    const K& first;
    value_reference second;

    RangeLoopIterator(key_iterator key_it, value_iterator value_it)
        : first(*reinterpret_cast<const K*>(key_it)), second(*value_it) {}
    RangeLoopIterator(const RangeLoopIterator& other)
        : first(other.first), second(other.second) {}
    RangeLoopIterator& operator=(const RangeLoopIterator& other) {
      *reinterpret_cast<PointerView*>(this) =
          *reinterpret_cast<const PointerView*>(&other);
      return *this;
    }

    RangeLoopIterator& operator*() { return *this; }
    const RangeLoopIterator& operator*() const { return *this; }

    RangeLoopIterator* operator->() { return this; }
    const RangeLoopIterator* operator->() const { return this; }

    operator std::pair<const K, T>() const { return {first, second}; }

    RangeLoopIterator& operator++() {
      reinterpret_cast<PointerView*>(this)->first++;
      reinterpret_cast<PointerView*>(this)->second++;
      return *this;
    }

    RangeLoopIterator operator++(int) {
      RangeLoopIterator t(*this);
      ++(*this);
      return t;
    }

    friend bool operator==(const RangeLoopIterator& x, const T* y) {
      return &x.second == y;
    }

    friend bool operator==(const T* x, const RangeLoopIterator& y) {
      return x == &y.second;
    }

    friend bool operator!=(const RangeLoopIterator& x, const T* y) {
      return &x.second != y;
    }

    friend bool operator!=(const T* x, const RangeLoopIterator& y) {
      return x != &y.second;
    }

    friend bool operator==(const RangeLoopIterator& x,
                           const RangeLoopIterator& y) {
      return &x.second == &y.second;
    }

    friend bool operator!=(const RangeLoopIterator& x,
                           const RangeLoopIterator& y) {
      return &x.second != &y.second;
    }

    value_iterator value_it() const { return &second; }
  };
};

/**
 * @brief Base storage type for binary-search array and linear search array.
 * It provides definitions and interfaces which have nothing to do with
 * query method of key.
 */
template <class K, class T, class KeyPolicy, size_t N, bool Stat>
struct KeyValueArray : protected MapStatisticsBase<Stat> {
 private:
  static constexpr bool get_is_key_hash() {
    if constexpr (has_use_hash_member_v<KeyPolicy>) {
      return KeyPolicy::use_hash;
    } else {
      return false;
    }
  }

  static constexpr bool get_consecutive_key() {
    if constexpr (has_consecutive_key_member_v<KeyPolicy>) {
      return KeyPolicy::consecutive_key;
    } else {
      return false;
    }
  }

  template <typename AnyType>
  struct type_identity {
    using type = AnyType;
  };

 public:
  static constexpr auto is_map = !std::is_void_v<T>;
  static constexpr auto is_key_hash = get_is_key_hash();
  static constexpr auto consecutive_key = get_consecutive_key();

  static_assert(!consecutive_key || is_map, "KeyPolicy only applies for map.");

 private:
  // V is insert_value_type or store_value_type.
  struct KeyExtractor {
    template <typename V>
    const auto& operator()(V&& v) {
      if constexpr (is_map) {
        return v.first;
      } else {
        return v;
      }
    }
  };

  struct ValueExtractor {
    template <typename V>
    auto& operator()(V&& v) {
      if constexpr (is_map) {
        if constexpr (consecutive_key) {
          return v;
        } else {
          return v.second;
        }
      } else {
        return v;
      }
    }
  };

 public:
  using key_type = K;
  using mapped_type = T;
  using key_policy = KeyPolicy;
  using key_extractor = KeyExtractor;
  using value_extractor = ValueExtractor;

  // Paramater type for traditional map insert() method and constructor from
  // initializer list.
  using insert_value_type =
      typename std::conditional_t<is_map, std::pair<const K, T>, K>;

  using non_const_insert_value_type =
      typename std::conditional_t<is_map, std::pair<K, T>, K>;

  // Value type in array from user's perspective.
  using value_type = typename std::conditional_t<
      is_map, std::conditional_t<consecutive_key, T, std::pair<const K, T>>, K>;

  // The actual type for array template `T` which removes const qualifier of K.
  using store_value_type = typename std::conditional_t<
      is_map, std::conditional_t<consecutive_key, T, std::pair<K, T>>, K>;

  using container_type = typename std::conditional_t<
      (N > 0),
      InlineVector<value_type, N,
                   is_key_hash ? sizeof(KeyPolicyReducedHashValueType) : 0,
                   Stat>,
      Vector<value_type,
             is_key_hash ? sizeof(KeyPolicyReducedHashValueType) : 0, Stat>>;

  using store_container_type = typename std::conditional_t<
      (N > 0),
      InlineVector<store_value_type, N,
                   is_key_hash ? sizeof(KeyPolicyReducedHashValueType) : 0,
                   Stat>,
      Vector<store_value_type,
             is_key_hash ? sizeof(KeyPolicyReducedHashValueType) : 0, Stat>>;
  using store_iterator =
      std::conditional_t<is_map, typename store_container_type::iterator,
                         typename store_container_type::const_iterator>;
  using const_store_iterator = typename store_container_type::const_iterator;

 protected:
  container_type& array() const {
    return *reinterpret_cast<container_type*>(
        const_cast<store_container_type*>(&array_));
  }

  template <class K2, class T2, class KeyPolicy2, size_t N2, bool Stat2>
  friend struct KeyValueArray;

  using MapStatisticsBase<Stat>::UpdateMaxCount;
  using MapStatisticsBase<Stat>::IncreaseEraseCount;

 public:
  using iterator = std::conditional_t<is_map, typename container_type::iterator,
                                      typename container_type::const_iterator>;
  using const_iterator = typename container_type::const_iterator;
  using reverse_iterator =
      std::conditional_t<is_map, typename container_type::reverse_iterator,
                         typename container_type::const_reverse_iterator>;
  using const_reverse_iterator =
      typename container_type::const_reverse_iterator;

 private:
  // For maps with Consecutive-Key policy enabled, since the key and value are
  // separate, special iterators are used for the begin() and end() methods, and
  // access to the key and value is provided through it.first() and it.second().
  template <bool Const>
  struct RangeLoopIteratorHelper {
    static auto select() {
      if constexpr (is_map && consecutive_key) {
        return type_identity<
            typename KeyPolicy::template RangeLoopIterator<T, Const>>{};
      } else {
        return type_identity<
            std::conditional_t<Const, const_iterator, iterator>>{};
      }
    }
  };

  template <bool Const>
  struct ReferenceHelper {
    static auto select() {
      if constexpr (is_map) {
        if constexpr (consecutive_key) {
          return type_identity<
              typename KeyPolicy::template Reference<T, Const>>{};
        } else {
          return type_identity<
              std::conditional_t<Const, const value_type&, value_type&>>{};
        }
      } else {
        return type_identity<const value_type&>{};
      }
    }
  };

 public:
  using range_loop_iterator =
      typename decltype(RangeLoopIteratorHelper<false>::select())::type;
  using range_loop_const_iterator =
      typename decltype(RangeLoopIteratorHelper<true>::select())::type;

  using reference_type =
      typename decltype(ReferenceHelper<false>::select())::type;
  using const_reference_type =
      typename decltype(ReferenceHelper<true>::select())::type;

 protected:
  // To accommodate the Consecutive-Key feature, the begin() and end() method
  // return a range_loop_iterator.
  iterator value_begin() { return array().begin(); }
  const_iterator value_begin() const { return array().begin(); }
  iterator value_end() { return array().end(); }
  const_iterator value_end() const { return array().end(); }

 public:
  KeyValueArray() {}

  // Only when not using hash, the internal array can be moved from source.
  // Because we are using extra leading bytes of array buffer to store hash
  // values.
  template <size_t ExtraBytesPerElement, bool CountRealloc,
            typename = std::enable_if_t<ExtraBytesPerElement == 0>>
  KeyValueArray(Vector<store_value_type, ExtraBytesPerElement, CountRealloc>&&
                    source_array)
      : array_(std::move(source_array)) {
    static_assert(!is_key_hash);
    UpdateMaxCount(array_.size());
  }

  template <size_t N2>
  KeyValueArray(const KeyValueArray<K, T, KeyPolicy, N2, Stat>& other)
      : array_(other.array_) {
    UpdateMaxCount(array_.size());
  }

  template <size_t N2>
  KeyValueArray(KeyValueArray<K, T, KeyPolicy, N2, Stat>&& other)
      : array_(std::move(other.array_)) {
    UpdateMaxCount(array_.size());
  }

  template <size_t N2>
  KeyValueArray& operator=(
      const KeyValueArray<K, T, KeyPolicy, N2, Stat>& other) {
    array_ = other.array_;
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  KeyValueArray& operator=(KeyValueArray<K, T, KeyPolicy, N2, Stat>&& other) {
    array_ = std::move(other.array_);
    UpdateMaxCount(array_.size());
    return *this;
  }

  bool is_static_buffer() const { return array_.is_static_buffer(); }

  size_t size() const { return array_.size(); }

  size_t capacity() const { return array_.capacity(); }

  bool empty() const { return array_.empty(); }

  void clear() {
    // We should also free memory so `array_.clear()` is not feasible.
    array_.clear_and_shrink();
  }
  void clear_keep_buffer() { array_.clear(); }

  bool reserve(size_t count) { return array_.reserve(count); }

  reference_type front() {
    if constexpr (is_map && consecutive_key) {
      return reference_type(_key_hash_data(), array().begin());
    } else {
      return array().front();
    }
  }

  const_reference_type front() const {
    if constexpr (is_map && consecutive_key) {
      return const_reference_type(_key_hash_data(), array().begin());
    } else {
      return array().front();
    }
  }

  reference_type back() {
    if constexpr (is_map && consecutive_key) {
      return reference_type(&_key_hash_data()[array().size() - 1],
                            &array().back());
    } else {
      return array().back();
    }
  }

  const_reference_type back() const {
    if constexpr (is_map && consecutive_key) {
      return const_reference_type(&_key_hash_data()[array().size() - 1],
                                  &array().back());
    } else {
      return array().back();
    }
  }

  range_loop_iterator begin() {
    if constexpr (is_map && consecutive_key) {
      return range_loop_iterator(_key_hash_data(), array().begin());
    } else {
      return array().begin();
    }
  }

  range_loop_const_iterator begin() const {
    if constexpr (is_map && consecutive_key) {
      return range_loop_const_iterator(_key_hash_data(), array().begin());
    } else {
      return array().begin();
    }
  }

  const_iterator cbegin() const { return array().cbegin(); }

  range_loop_iterator end() {
    if constexpr (is_map && consecutive_key) {
      // RangeLoopIterator only compares value iterator.
      return range_loop_iterator(nullptr, array().end());
    } else {
      return array().end();
    }
  }

  const_iterator cend() const { return array().cend(); }

  range_loop_const_iterator end() const {
    if constexpr (is_map && consecutive_key) {
      // RangeLoopIterator only compares value iterator.
      return range_loop_const_iterator(nullptr, array().end());
    } else {
      return array().end();
    }
  }

  reverse_iterator rbegin() { return array().rbegin(); }

  const_reverse_iterator rbegin() const { return array().rbegin(); }

  const_reverse_iterator crbegin() const { return array().crbegin(); }

  reverse_iterator rend() { return array().rend(); }

  const_reverse_iterator rend() const { return array().rend(); }

  const_reverse_iterator crend() const { return array().crend(); }

  template <typename It,
            typename = std::enable_if_t<std::is_same_v<It, iterator> ||
                                        std::is_same_v<It, const_iterator>>>
  iterator erase(It pos) {
    IncreaseEraseCount();
    if constexpr (is_key_hash) {
      _erase_key_hash(pos - value_begin());
    }
    if constexpr (std::is_same_v<It, iterator>) {
      return reinterpret_cast<iterator>(
          array_.erase(reinterpret_cast<store_iterator>(pos)));
    } else {
      return reinterpret_cast<iterator>(
          array_.erase(reinterpret_cast<const_store_iterator>(pos)));
    }
  }

  // For usage of `map.erase(map.begin());` when map is using
  // MapKeyPolicyConsecutiveIntegers.
  template <typename It, typename = std::enable_if_t<
                             (is_map && consecutive_key) &&
                             (std::is_same_v<It, range_loop_iterator> ||
                              std::is_same_v<It, range_loop_const_iterator>)>>
  iterator erase(const It& pos) {
    IncreaseEraseCount();
    if constexpr (is_key_hash) {
      _erase_key_hash(pos.value_it() - value_begin());
    }
    if constexpr (std::is_same_v<It, range_loop_iterator>) {
      return reinterpret_cast<iterator>(
          array_.erase(reinterpret_cast<store_iterator>(pos.value_it())));
    } else {
      return reinterpret_cast<iterator>(
          array_.erase(reinterpret_cast<const_store_iterator>(pos.value_it())));
    }
  }

  template <size_t N2>
  bool operator==(const KeyValueArray<K, T, KeyPolicy, N2, Stat>& other) const {
    return array_ == other.array_;
  }

  template <size_t N2>
  bool operator!=(const KeyValueArray<K, T, KeyPolicy, N2, Stat>& other) const {
    return array_ != other.array_;
  }

  class Unsafe {
    // ATTENTION: function under this class is UNSAFE to use.
    // Do NOT use them unless you have consulted with the owners of Vector.
   public:
    Unsafe() = delete;

    // Force to set size of vector to target value. It is usually called for a
    // vector that is about to be destroyed and sets the size to 0 to avoid
    // calling the destructor method of the internal objects that have been
    // relocated.
    static void SetSize(KeyValueArray& map, size_t value) {
      decltype(map.array_)::Unsafe::SetSize(map.array_, value);
    }
  };

 protected:
  friend class Unsafe;

  store_container_type array_;

  template <size_t N2>
  void _swap(KeyValueArray<K, T, KeyPolicy, N2, Stat>& other) {
    array_.swap(other.array_);
  }

  BASE_VECTOR_INLINE KeyPolicyReducedHashValueType* _key_hash_data() const {
    return static_cast<KeyPolicyReducedHashValueType*>(
        array_.get_memory_allocate());
  }

  void _erase_key_hash(size_t index) {
    auto* data = _key_hash_data();
    std::memmove(&data[index], &data[index + 1],
                 (size() - index - 1) * sizeof(KeyPolicyReducedHashValueType));
  }
};

/**
 * @brief For ordered map/set base on array. Performance consideration:
 *  1. Insertion is relatively slow because of reallocation and moving of
 * elements. While map/set also need rebalancing of RB tree.
 * Performance test data shows that when the key is a basic type such as int,
 * the insertion performance of the container is still ahead of std::map when
 * the data volume reaches several thousands. When the key is a type with
 * relatively low movement overhead such as std::string, the insertion
 * performance is on par with std::map when the data volume reaches 30-50.
 * Besides, ordered map/set on array supports real reserve() to pre-allocate
 * container buffer.
 *  2. Finding is array-based binary-search which should outperform map/set
 * because array is linear and much simpler.
 *  3. Memory layout is much better than node-based map and unordered_map
 * because elements are stored linearly and continuously.
 *  4. Compiled binary size is the same as map/set and smaller than
 * unordered_map/unordered_set.
 *  5. Struct type size(16b) is smaller than std::map, std::set,
 * std::unordered_map and std::unordered_set.
 *
 * Best use scenarios:
 * 1. Small amount of data, from dozens to hundreds.
 * 2. Low moving overhead for Key and Value, such as primitive types,
 * std::string, std::shared_ptr.
 * 3. More queries than data modifications.
 *
 * Please use the following specialized types:
 *   OrderedFlatMap
 *   InlineOrderedFlatMap
 *   OrderedFlatSet
 *   InlineOrderedFlatSet
 *
 * When implementing custom Compare method. Use f(const K&, const K&) as
 * signature to avoid unnecessary copy.
 *
 * Binary search array is ordered and does not support KeyPolicy with hash
 * value to boost finding right now.
 */
template <class K, class T, size_t N, class Compare, bool Stat>
struct BinarySearchArray : public KeyValueArray<K, T, void, N, Stat> {
 protected:
  template <class K2, class T2, size_t N2, class Compare2, bool Stat2>
  friend struct BinarySearchArray;

  using MapStatisticsBase<Stat>::UpdateMaxCount;
  using MapStatisticsBase<Stat>::IncreaseInsertCount;
  using MapStatisticsBase<Stat>::IncreaseEraseCount;
  using MapStatisticsBase<Stat>::RecordFind;

  using KeyValueArray<K, T, void, N, Stat>::is_map;
  using KeyValueArray<K, T, void, N, Stat>::array_;
  using KeyValueArray<K, T, void, N, Stat>::_swap;

 public:
  using key_compare = Compare;
  using typename KeyValueArray<K, T, void, N, Stat>::key_extractor;
  using typename KeyValueArray<K, T, void, N, Stat>::value_type;
  using typename KeyValueArray<K, T, void, N, Stat>::store_value_type;
  using typename KeyValueArray<K, T, void, N, Stat>::iterator;
  using typename KeyValueArray<K, T, void, N, Stat>::const_iterator;
  using typename KeyValueArray<K, T, void, N, Stat>::store_iterator;
  using KeyValueArray<K, T, void, N, Stat>::begin;
  using KeyValueArray<K, T, void, N, Stat>::end;
  using KeyValueArray<K, T, void, N, Stat>::erase;

 public:
  BinarySearchArray() {}
  BinarySearchArray(std::initializer_list<value_type> list) {
    array_.reserve(list.size());
    for (auto& v : list) {
      insert(std::move(v));
    }
  }

  template <size_t N2>
  BinarySearchArray(const BinarySearchArray<K, T, N2, Compare, Stat>& other)
      : KeyValueArray<K, T, void, N, Stat>(other) {}

  template <size_t N2>
  BinarySearchArray(BinarySearchArray<K, T, N2, Compare, Stat>&& other)
      : KeyValueArray<K, T, void, N, Stat>(std::move(other)) {}

  template <size_t N2>
  BinarySearchArray& operator=(
      const BinarySearchArray<K, T, N2, Compare, Stat>& other) {
    array_ = other.array_;
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  BinarySearchArray& operator=(
      BinarySearchArray<K, T, N2, Compare, Stat>&& other) {
    array_ = std::move(other.array_);
    UpdateMaxCount(array_.size());
    return *this;
  }

  std::pair<iterator, bool> insert(const value_type& value) {
    if constexpr (is_map) {
      auto pos = lower_bound(value.first);
      if (BASE_VECTOR_UNLIKELY(pos != end() && pos->first == value.first)) {
        RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
        return {pos, false};  // duplicate
      } else {
        RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
        return {
            reinterpret_cast<iterator>(array_.emplace(
                reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
                std::forward_as_tuple(value.first),
                std::forward_as_tuple(value.second))),
            true};
      }
    } else {
      auto pos = lower_bound(value);
      if (BASE_VECTOR_UNLIKELY(pos != end() && *pos == value)) {
        RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
        return {pos, false};  // duplicate
      } else {
        RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
        return {reinterpret_cast<iterator>(array_.emplace(
                    reinterpret_cast<store_iterator>(pos), value)),
                true};
      }
    }
  }

  std::pair<iterator, bool> insert(value_type&& value) {
    if constexpr (is_map) {
      auto pos = lower_bound(value.first);
      if (BASE_VECTOR_UNLIKELY(pos != end() && pos->first == value.first)) {
        RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
        return {pos, false};  // duplicate
      } else {
        RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
        return {
            reinterpret_cast<iterator>(array_.emplace(
                reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
                std::forward_as_tuple(std::move(value.first)),
                std::forward_as_tuple(std::move(value.second)))),
            true};
      }
    } else {
      auto pos = lower_bound(value);
      if (BASE_VECTOR_UNLIKELY(pos != end() && *pos == value)) {
        RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
        return {pos, false};  // duplicate
      } else {
        RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
        return {reinterpret_cast<iterator>(array_.emplace(
                    reinterpret_cast<store_iterator>(pos), std::move(value))),
                true};
      }
    }
  }

  // Without this overload, `std::pair<K, T> v; map.insert(std::move(v));` would
  // implicitly construct `std::pair<const K, T>` from `std::move(v)`.
  template <class U, class = std::enable_if_t<
                         is_map && std::is_same_v<U, store_value_type>>>
  std::pair<iterator, bool> insert(U&& value) {
    auto pos = lower_bound(value.first);
    if (BASE_VECTOR_UNLIKELY(pos != end() && pos->first == value.first)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      return {pos, false};  // duplicate
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {reinterpret_cast<iterator>(array_.emplace(
                  reinterpret_cast<store_iterator>(pos), std::move(value))),
              true};
    }
  }

  size_t erase(const K& key) {
    IncreaseEraseCount();
    RecordFind(MapStatisticsFindKind::kFind, array_.size());
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      array_.erase(reinterpret_cast<store_iterator>(pos));
      return 1;
    } else {
      return 0;
    }
  }

  iterator find(const K& key) {
    RecordFind(MapStatisticsFindKind::kFind, array_.size());
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      return pos;
    } else {
      return end();
    }
  }

  const_iterator find(const K& key) const {
    RecordFind(MapStatisticsFindKind::kFind, array_.size());
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      return pos;
    } else {
      return end();
    }
  }

  bool contains(const K& key) const { return find(key) != end(); }

  size_t count(const K& key) const { return contains(key) ? 1 : 0; }

  template <size_t N2>
  void swap(BinarySearchArray<K, T, N2, Compare, Stat>& other) {
    _swap(other);
  }

  template <size_t N2>
  void merge(BinarySearchArray<K, T, N2, Compare, Stat>& other) {
    auto& other_array = other.array_;
    for (intptr_t i = other_array.size() - 1; i >= 0; i--) {
      if (insert(std::move(other_array[i])).second) {
        other_array.erase(other_array.begin() + i);
      }
    }
  }

  bool is_data_ordered() const { return true; }

  template <size_t N2>
  bool operator==(
      const BinarySearchArray<K, T, N2, Compare, Stat>& other) const {
    return array_ == other.array_;
  }

  template <size_t N2>
  bool operator!=(
      const BinarySearchArray<K, T, N2, Compare, Stat>& other) const {
    return array_ != other.array_;
  }

 protected:
  BASE_VECTOR_NEVER_INLINE iterator lower_bound(const K& key) const {
    return const_cast<iterator>(std::lower_bound(
        begin(), end(), key, [](const value_type& item, const K& value) {
          return Compare()(key_extractor()(item), value);
        }));
  }
};

/**
 * @brief Used to implement map/set, but only provides map/set style interface,
 * internal data is stored in array linearly, and search also uses linear
 * search.
 *
 * It is very suitable for scenarios that require a map/set interface, but the
 * amount of data is relatively small, and the number of container traversals is
 * far greater than the number of searches.
 *
 * You could provide non-void KeyPolicy to boost key finding with hash value and
 * customize key equality check function.
 */
template <class K, class T, class KeyPolicy, size_t N, bool Stat>
struct LinearSearchArray : public KeyValueArray<K, T, KeyPolicy, N, Stat> {
 private:
  template <typename AnyType>
  struct type_identity {
    using type = AnyType;
  };

  template <bool Flag>
  struct KeyPolicyEqualHelper {
    static auto select() {
      if constexpr (Flag) {
        return type_identity<typename KeyPolicy::equal>{};
      } else {
        return type_identity<std::equal_to<K>>{};
      }
    }
  };

  template <bool Flag>
  struct KeyPolicyEqualWhenHashEqualHelper {
    static auto select() {
      if constexpr (Flag) {
        return type_identity<typename KeyPolicy::equal_when_hash_equal>{};
      } else {
        return type_identity<std::equal_to<K>>{};
      }
    }
  };

  static constexpr bool get_assign_existing_for_merge() {
    if constexpr (has_assign_existing_for_merge_member_v<KeyPolicy>) {
      return KeyPolicy::assign_existing_for_merge;
    } else {
      return false;
    }
  }

 protected:
  template <class K2, class T2, class KeyPolicy2, size_t N2, bool Stat2>
  friend struct LinearSearchArray;

  using MapStatisticsBase<Stat>::UpdateMaxCount;
  using MapStatisticsBase<Stat>::IncreaseInsertCount;
  using MapStatisticsBase<Stat>::IncreaseEraseCount;
  using MapStatisticsBase<Stat>::RecordFind;

  using KeyValueArray<K, T, KeyPolicy, N, Stat>::is_map;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::array_;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::_swap;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::_key_hash_data;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::value_begin;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::value_end;

 public:
  static constexpr auto assign_existing_for_merge =
      get_assign_existing_for_merge();

  using key_compare = void;
  using key_equal =
      typename decltype(KeyPolicyEqualHelper<
                        has_equal_member_v<KeyPolicy>>::select())::type;
  using key_equal_when_hash_equal =
      typename decltype(KeyPolicyEqualWhenHashEqualHelper<
                        has_equal_when_hash_equal_member_v<KeyPolicy>>::
                            select())::type;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::key_extractor;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::value_extractor;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::is_key_hash;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::consecutive_key;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::insert_value_type;
  using typename KeyValueArray<K, T, KeyPolicy, N,
                               Stat>::non_const_insert_value_type;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::value_type;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::store_value_type;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::iterator;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::const_iterator;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::store_iterator;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::range_loop_iterator;
  using typename KeyValueArray<K, T, KeyPolicy, N,
                               Stat>::range_loop_const_iterator;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::begin;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::end;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::erase;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::empty;

 protected:
  inline static KeyPolicyReducedHashValueType _reduced_key_hash(const K& key) {
    if constexpr (is_key_hash) {
      return static_cast<KeyPolicyReducedHashValueType>(
          typename KeyPolicy::hash()(key));
    } else {
      return KeyPolicyReducedHashValueType{};
    }
  }

 public:
  LinearSearchArray() {}

  LinearSearchArray(std::initializer_list<insert_value_type> list) {
    array_.reserve(list.size());
    for (auto& v : list) {
      insert(std::move(v));
    }
  }

  // Only when not using hash, the internal array can be moved from source.
  // Because we are using extra leading bytes of array buffer to store hash
  // values.
  template <size_t ExtraBytesPerElement, bool CountRealloc,
            typename = std::enable_if_t<ExtraBytesPerElement == 0>>
  LinearSearchArray(Vector<store_value_type, ExtraBytesPerElement,
                           CountRealloc>&& source_array)
      : KeyValueArray<K, T, KeyPolicy, N, Stat>(std::move(source_array)) {
    static_assert(!is_key_hash);
  }

  template <size_t ExtraBytesPerElement, bool CountRealloc,
            typename = std::enable_if_t<ExtraBytesPerElement == 0>>
  LinearSearchArray& operator=(Vector<store_value_type, ExtraBytesPerElement,
                                      CountRealloc>&& source_array) {
    static_assert(!is_key_hash);
    array_ = std::move(source_array);
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  LinearSearchArray(const LinearSearchArray<K, T, KeyPolicy, N2, Stat>& other)
      : KeyValueArray<K, T, KeyPolicy, N, Stat>(other) {}

  template <size_t N2>
  LinearSearchArray(LinearSearchArray<K, T, KeyPolicy, N2, Stat>&& other)
      : KeyValueArray<K, T, KeyPolicy, N, Stat>(std::move(other)) {}

  template <size_t N2>
  LinearSearchArray& operator=(
      const LinearSearchArray<K, T, KeyPolicy, N2, Stat>& other) {
    array_ = other.array_;
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  LinearSearchArray& operator=(
      LinearSearchArray<K, T, KeyPolicy, N2, Stat>&& other) {
    array_ = std::move(other.array_);
    UpdateMaxCount(array_.size());
    return *this;
  }

  // No check of key existence.
  iterator insert_unique(const insert_value_type& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
    }

    iterator result;
    if constexpr (is_map) {
      if constexpr (consecutive_key) {
        result = reinterpret_cast<iterator>(&array_.emplace_back(value.second));
      } else {
        result = reinterpret_cast<iterator>(&array_.emplace_back(
            std::piecewise_construct, std::forward_as_tuple(value.first),
            std::forward_as_tuple(value.second)));
      }
    } else {
      result = reinterpret_cast<iterator>(&array_.emplace_back(value));
    }
    if constexpr (is_key_hash) {
      // Store hash value after array insertion because array may expand.
      _key_hash_data()[array_.size() - 1] = hash;
    }
    return result;
  }

  iterator insert_unique(insert_value_type&& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
    }

    iterator result;
    if constexpr (is_map) {
      if constexpr (consecutive_key) {
        result = reinterpret_cast<iterator>(
            &array_.emplace_back(std::move(value.second)));
      } else {
        result = reinterpret_cast<iterator>(&array_.emplace_back(
            std::piecewise_construct,
            std::forward_as_tuple(std::move(value.first)),
            std::forward_as_tuple(std::move(value.second))));
      }
    } else {
      result =
          reinterpret_cast<iterator>(&array_.emplace_back(std::move(value)));
    }
    if constexpr (is_key_hash) {
      // Store hash value after array insertion because array may expand.
      _key_hash_data()[array_.size() - 1] = hash;
    }
    return result;
  }

  template <bool enabled = is_map>
  std::enable_if_t<enabled, iterator> insert_unique(
      const non_const_insert_value_type& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
    }

    iterator result;
    if constexpr (consecutive_key) {
      result = reinterpret_cast<iterator>(&array_.emplace_back(value.second));
    } else {
      result = reinterpret_cast<iterator>(&array_.emplace_back(
          std::piecewise_construct, std::forward_as_tuple(value.first),
          std::forward_as_tuple(value.second)));
    }
    if constexpr (is_key_hash) {
      // Store hash value after array insertion because array may expand.
      _key_hash_data()[array_.size() - 1] = hash;
    }
    return result;
  }

  template <bool enabled = is_map>
  std::enable_if_t<enabled, iterator> insert_unique(
      non_const_insert_value_type&& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
    }

    iterator result;
    if constexpr (consecutive_key) {
      result = reinterpret_cast<iterator>(
          &array_.emplace_back(std::move(value.second)));
    } else {
      result = reinterpret_cast<iterator>(
          &array_.emplace_back(std::piecewise_construct,
                               std::forward_as_tuple(std::move(value.first)),
                               std::forward_as_tuple(std::move(value.second))));
    }
    if constexpr (is_key_hash) {
      // Store hash value after array insertion because array may expand.
      _key_hash_data()[array_.size() - 1] = hash;
    }
    return result;
  }

  std::pair<iterator, bool> insert(const insert_value_type& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
      pos = find_exact(key_extractor()(value),
                       hash);  // find with hash value
    } else {
      pos = find_exact(key_extractor()(value));
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};  // duplicate
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (is_map) {
        if constexpr (consecutive_key) {
          result = {
              reinterpret_cast<iterator>(&array_.emplace_back(value.second)),
              true};
        } else {
          result = {
              reinterpret_cast<iterator>(&array_.emplace_back(
                  std::piecewise_construct, std::forward_as_tuple(value.first),
                  std::forward_as_tuple(value.second))),
              true};
        }
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(value)),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  std::pair<iterator, bool> insert(insert_value_type&& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
      pos = find_exact(key_extractor()(value),
                       hash);  // find with hash value
    } else {
      pos = find_exact(key_extractor()(value));
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};  // duplicate
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (is_map) {
        if constexpr (consecutive_key) {
          result = {reinterpret_cast<iterator>(
                        &array_.emplace_back(std::move(value.second))),
                    true};
        } else {
          result = {reinterpret_cast<iterator>(&array_.emplace_back(
                        std::piecewise_construct,
                        std::forward_as_tuple(std::move(value.first)),
                        std::forward_as_tuple(std::move(value.second)))),
                    true};
        }
      } else {
        result = {
            reinterpret_cast<iterator>(&array_.emplace_back(std::move(value))),
            true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  template <bool enabled = is_map>
  std::enable_if_t<enabled, std::pair<iterator, bool>> insert(
      const non_const_insert_value_type& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
      pos = find_exact(key_extractor()(value),
                       hash);  // find with hash value
    } else {
      pos = find_exact(key_extractor()(value));
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};  // duplicate
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {
            reinterpret_cast<iterator>(&array_.emplace_back(value.second)),
            true};
      } else {
        result = {
            reinterpret_cast<iterator>(&array_.emplace_back(
                std::piecewise_construct, std::forward_as_tuple(value.first),
                std::forward_as_tuple(value.second))),
            true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  template <bool enabled = is_map>
  std::enable_if_t<enabled, std::pair<iterator, bool>> insert(
      non_const_insert_value_type&& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key_extractor()(value));
      pos = find_exact(key_extractor()(value),
                       hash);  // find with hash value
    } else {
      pos = find_exact(key_extractor()(value));
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};  // duplicate
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(
                      &array_.emplace_back(std::move(value.second))),
                  true};
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(
                      std::piecewise_construct,
                      std::forward_as_tuple(std::move(value.first)),
                      std::forward_as_tuple(std::move(value.second)))),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  size_t erase(const K& key) {
    RecordFind(MapStatisticsFindKind::kFind, array_.size());
    iterator pos;
    if constexpr (is_key_hash) {
      auto hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }
    if (pos != nullptr) {
      erase(pos);
      return 1;
    } else {
      return 0;
    }
  }

  range_loop_iterator find(const K& key) {
    RecordFind(MapStatisticsFindKind::kFind, array_.size());
    iterator pos;
    if constexpr (is_key_hash) {
      auto hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }
    if (pos != nullptr) {
      if constexpr (is_map && consecutive_key) {
        return range_loop_iterator(_key_hash_data() + (pos - value_begin()),
                                   pos);
      } else {
        return pos;
      }
    } else {
      return end();
    }
  }

  range_loop_const_iterator find(const K& key) const {
    RecordFind(MapStatisticsFindKind::kFind, array_.size());
    iterator pos;
    if constexpr (is_key_hash) {
      auto hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }
    if (pos != nullptr) {
      if constexpr (is_map && consecutive_key) {
        return range_loop_const_iterator(
            _key_hash_data() + (pos - value_begin()), pos);
      } else {
        return pos;
      }
    } else {
      return end();
    }
  }

  bool contains(const K& key) const {
    RecordFind(MapStatisticsFindKind::kFind, array_.size());
    if constexpr (is_key_hash) {
      auto hash = _reduced_key_hash(key);
      return find_exact(key, hash) != nullptr;  // find with hash value
    } else {
      return find_exact(key) != nullptr;
    }
  }

  size_t count(const K& key) const { return contains(key) ? 1 : 0; }

  template <size_t N2>
  void swap(LinearSearchArray<K, T, KeyPolicy, N2, Stat>& other) {
    _swap(other);
  }

  template <size_t N2>
  void merge(const LinearSearchArray<K, T, KeyPolicy, N2, Stat>& other) {
    merge(const_cast<LinearSearchArray<K, T, KeyPolicy, N2, Stat>&>(other));
  }

  template <size_t N2>
  void merge(LinearSearchArray<K, T, KeyPolicy, N2, Stat>& other) {
    auto& other_array = other.array_;
    if (empty()) {
      // Fast path when self is empty.
      if constexpr (assign_existing_for_merge) {
        array_ = other_array;
      } else {
        array_ = std::move(other_array);
      }
      return;
    }

    [[maybe_unused]] const KeyPolicyReducedHashValueType* other_key_hash_data;
    if constexpr (is_key_hash) {
      other_key_hash_data = other._key_hash_data();
    }
    // LinearSearchArray has an implicit property: the iteration order is
    // consistent with the data insertion order. To ensure this property is not
    // violated, the `other` container must also be traversed from front to back
    // during the `merge` operation.
    size_t other_size = other_array.size();
    for (size_t i = 0; i < other_size;) {
      auto& object = other_array[i];
      [[maybe_unused]] KeyPolicyReducedHashValueType hash;
      iterator pos;
      if constexpr (is_key_hash) {
        hash = other_key_hash_data[i];
        if constexpr (consecutive_key) {
          pos = find_exact(static_cast<K>(hash), hash);
        } else {
          pos = find_exact(key_extractor()(object), hash);
        }
      } else {
        pos = find_exact(key_extractor()(object));
      }
      if constexpr (assign_existing_for_merge) {
        // If KeyPolicy specifies assign_existing_for_merge as true, for data
        // that already exists in this, do assignment and do not modify other.
        if (pos == nullptr) {
          array_.emplace_back(object);
          if constexpr (is_key_hash) {
            // Store hash value after array insertion because array may expand.
            _key_hash_data()[array_.size() - 1] = hash;
          }
        } else if constexpr (is_map) {
          value_extractor()(*pos) = value_extractor()(object);
        }
      } else {
        // Behavior like std::unordered_map, splice from other.
        if (pos == nullptr) {
          array_.emplace_back(std::move(object));
          if constexpr (is_key_hash) {
            // Store hash value after array insertion because array may expand.
            _key_hash_data()[array_.size() - 1] = hash;
          }
          other.erase(other.value_begin() + i);
          other_size--;
          continue;
        }
      }
      i++;
    }
  }

  bool is_data_ordered() const { return false; }

  template <size_t N2>
  bool operator==(
      const LinearSearchArray<K, T, KeyPolicy, N2, Stat>& other) const {
    return array_ == other.array_;
  }

  template <size_t N2>
  bool operator!=(
      const LinearSearchArray<K, T, KeyPolicy, N2, Stat>& other) const {
    return array_ != other.array_;
  }

 protected:
  BASE_VECTOR_INLINE iterator find_exact(const K& key) const {
    for (auto it = array_.begin(), end = array_.end(); it != end; it++) {
      if (key_equal()(key_extractor()(*it), key)) {
        return reinterpret_cast<iterator>(const_cast<store_iterator>(it));
      }
    }
    return nullptr;
  }

  template <bool enabled = is_key_hash>
  std::enable_if_t<enabled, iterator> find_exact(
      const K& key, KeyPolicyReducedHashValueType hash) const {
    // Boosted find with hash value when KeyPolicy provided.
    const auto* key_hash_data = _key_hash_data();
    const intptr_t count = static_cast<intptr_t>(array_.size());
    auto begin = array_.begin();
    intptr_t i = 0;

#define BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL                            \
  if constexpr (consecutive_key) {                                       \
    return reinterpret_cast<iterator>(const_cast<store_iterator>(it));   \
  } else {                                                               \
    if (key_equal_when_hash_equal()(key_extractor()(*it), key)) {        \
      return reinterpret_cast<iterator>(const_cast<store_iterator>(it)); \
    }                                                                    \
  }

#ifdef BASE_VECTOR_NEON
    const uint32x4_t target_vec = vdupq_n_u32(hash);
    for (; i <= count - 4; i += 4) {
      uint32x4_t data_vec = vld1q_u32(key_hash_data + i);
      uint32x4_t mask = vceqq_u32(data_vec, target_vec);

#ifdef BASE_VECTOR_NEON_ARMV8
      // Using armv8, pre-checking once for existence is faster than checking
      // twice.
      if (vmaxvq_u32(mask) != 0)
#endif
      {
        if (auto pair = vgetq_lane_u64(mask, 0); pair != 0) {
          if ((uint32_t)pair != 0) {
            auto it = begin + i + 0;
            BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL
          }
          if ((pair >> 32) != 0) {
            auto it = begin + i + 1;
            BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL
          }
        }
        if (auto pair = vgetq_lane_u64(mask, 1); pair != 0) {
          if ((uint32_t)pair != 0) {
            auto it = begin + i + 2;
            BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL
          }
          if ((pair >> 32) != 0) {
            auto it = begin + i + 3;
            BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL
          }
        }
      }
    }
#elif defined(BASE_VECTOR_SSE2)
    const __m128i target_vec = _mm_set1_epi32(static_cast<int>(hash));
    for (; i <= count - 4; i += 4) {
      __m128i data_vec =
          _mm_loadu_si128(reinterpret_cast<const __m128i*>(key_hash_data + i));
      __m128i mask = _mm_cmpeq_epi32(data_vec, target_vec);
      int movemask_result = _mm_movemask_ps(_mm_castsi128_ps(mask));
      if (movemask_result != 0) {
        int temp_mask = movemask_result;
        while (temp_mask != 0) {
          int j = unchecked_count_right_zero(temp_mask);
          auto it = begin + i + j;
          BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL
          // Clear this bit and goto the next iteration.
          temp_mask &= (temp_mask - 1);
        }
      }
    }
#else
    // No SIMD supports, fall-through
#endif

    for (; i < count; ++i) {
      if (key_hash_data[i] == hash) {
        auto it = begin + i;
        BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL
      }
    }
    return nullptr;
  }
#undef BASE_VECTOR_CHECK_KEY_WHEN_HASH_EQUAL
};

template <class K, class T, size_t N, class Compare, bool Stat>
struct BinarySearchMap : public BinarySearchArray<K, T, N, Compare, Stat> {
 protected:
  template <class K2, class T2, size_t N2, class Compare2, bool Stat2>
  friend struct BinarySearchMap;

  using MapStatisticsBase<Stat>::UpdateMaxCount;
  using MapStatisticsBase<Stat>::IncreaseInsertCount;
  using MapStatisticsBase<Stat>::IncreaseEraseCount;
  using MapStatisticsBase<Stat>::RecordFind;

  using KeyValueArray<K, T, void, N, Stat>::array_;

 public:
  using typename KeyValueArray<K, T, void, N, Stat>::key_extractor;
  using typename BinarySearchArray<K, T, N, Compare, Stat>::value_type;
  using typename BinarySearchArray<K, T, N, Compare, Stat>::iterator;
  using typename BinarySearchArray<K, T, N, Compare, Stat>::store_iterator;
  using BinarySearchArray<K, T, N, Compare, Stat>::insert;
  using BinarySearchArray<K, T, N, Compare, Stat>::lower_bound;
  using KeyValueArray<K, T, void, N, Stat>::end;

  BinarySearchMap() {}
  BinarySearchMap(std::initializer_list<value_type> list) {
    array_.reserve(list.size());
    for (auto& v : list) {
      insert(std::move(v));
    }
  }

  template <size_t N2>
  BinarySearchMap(const BinarySearchMap<K, T, N2, Compare, Stat>& other)
      : BinarySearchArray<K, T, N, Compare, Stat>(other) {}

  template <size_t N2>
  BinarySearchMap(BinarySearchMap<K, T, N2, Compare, Stat>&& other)
      : BinarySearchArray<K, T, N, Compare, Stat>(std::move(other)) {}

  template <size_t N2>
  BinarySearchMap& operator=(
      const BinarySearchMap<K, T, N2, Compare, Stat>& other) {
    array_ = other.array_;
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  BinarySearchMap& operator=(BinarySearchMap<K, T, N2, Compare, Stat>&& other) {
    array_ = std::move(other.array_);
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <class U>
  T& operator[](U&& key) {
    return try_insert(std::forward<U>(key)).first->second;
  }

  template <class U>
  T& at(U&& key) {
    return try_insert(std::forward<U>(key)).first->second;
  }

  template <class U>
  std::pair<iterator, bool> insert_default_if_absent(U&& key) {
    return try_insert(std::forward<U>(key));
  }

  template <class V>
  std::pair<iterator, bool> insert_or_assign(const K& key, V&& value) {
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      pos->second = std::forward<V>(value);
      return {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {reinterpret_cast<iterator>(array_.emplace(
                  reinterpret_cast<store_iterator>(pos),
                  std::piecewise_construct, std::forward_as_tuple(key),
                  std::forward_as_tuple(std::forward<V>(value)))),
              true};
    }
  }

  template <class V>
  std::pair<iterator, bool> insert_or_assign(K&& key, V&& value) {
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      pos->second = std::forward<V>(value);
      return {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {
          reinterpret_cast<iterator>(array_.emplace(
              reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
              std::forward_as_tuple(std::move(key)),
              std::forward_as_tuple(std::forward<V>(value)))),
          true};
    }
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(const K& key, Args&&... args) {
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      return {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {reinterpret_cast<iterator>(array_.emplace(
                  reinterpret_cast<store_iterator>(pos),
                  std::piecewise_construct, std::forward_as_tuple(key),
                  std::forward_as_tuple(std::forward<Args>(args)...))),
              true};
    }
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(K&& key, Args&&... args) {
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      return {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {
          reinterpret_cast<iterator>(array_.emplace(
              reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
              std::forward_as_tuple(std::move(key)),
              std::forward_as_tuple(std::forward<Args>(args)...))),
          true};
    }
  }

  template <class... Args>
  std::pair<iterator, bool> emplace_or_assign(const K& key, Args&&... args) {
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      pos->second.~T();
      new (&(pos->second)) T(std::forward<Args>(args)...);
      return {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {reinterpret_cast<iterator>(array_.emplace(
                  reinterpret_cast<store_iterator>(pos),
                  std::piecewise_construct, std::forward_as_tuple(key),
                  std::forward_as_tuple(std::forward<Args>(args)...))),
              true};
    }
  }

  template <class... Args>
  std::pair<iterator, bool> emplace_or_assign(K&& key, Args&&... args) {
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      pos->second.~T();
      new (&(pos->second)) T(std::forward<Args>(args)...);
      return {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {
          reinterpret_cast<iterator>(array_.emplace(
              reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
              std::forward_as_tuple(std::move(key)),
              std::forward_as_tuple(std::forward<Args>(args)...))),
          true};
    }
  }

  template <class U, class... Args>
  std::pair<iterator, bool> try_emplace(U&& key, Args&&... args) {
    return emplace(std::forward<U>(key), std::forward<Args>(args)...);
  }

  template <typename... Args1, typename... Args2>
  std::pair<iterator, bool> emplace(std::piecewise_construct_t,
                                    std::tuple<Args1...> args1,
                                    std::tuple<Args2...> args2) {
    // Construct key in first place.
    K key = std::make_from_tuple<K>(std::move(args1));

    // Do emplace.
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      return {pos, false};  // key is discarded
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      return {
          reinterpret_cast<iterator>(array_.emplace(
              reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
              std::forward_as_tuple(std::move(key)), std::move(args2))),
          true};
    }
  }

  template <size_t N2>
  bool operator==(const BinarySearchMap<K, T, N2, Compare, Stat>& other) const {
    return array_ == other.array_;
  }

  template <size_t N2>
  bool operator!=(const BinarySearchMap<K, T, N2, Compare, Stat>& other) const {
    return array_ != other.array_;
  }

 protected:
  std::pair<iterator, bool> try_insert(const K& key) {
    std::pair<iterator, bool> result;
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      result = {
          reinterpret_cast<iterator>(array_.emplace(
              reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
              std::forward_as_tuple(key), std::tuple<>())),
          true};
    }
    return result;
  }

  std::pair<iterator, bool> try_insert(K&& key) {
    std::pair<iterator, bool> result;
    auto pos = lower_bound(key);
    if (pos != end() && key_extractor()(*pos) == key) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      result = {
          reinterpret_cast<iterator>(array_.emplace(
              reinterpret_cast<store_iterator>(pos), std::piecewise_construct,
              std::forward_as_tuple(std::move(key)), std::tuple<>())),
          true};
    }
    return result;
  }
};

template <class K, size_t N, class Compare, bool Stat>
struct BinarySearchSet : public BinarySearchArray<K, void, N, Compare, Stat> {
 protected:
  template <class K2, size_t N2, class Compare2, bool Stat2>
  friend struct BinarySearchSet;

  using MapStatisticsBase<Stat>::UpdateMaxCount;

  using BinarySearchArray<K, void, N, Compare, Stat>::array_;

 public:
  using typename BinarySearchArray<K, void, N, Compare, Stat>::value_type;
  using typename BinarySearchArray<K, void, N, Compare, Stat>::iterator;
  using BinarySearchArray<K, void, N, Compare, Stat>::insert;

  BinarySearchSet() {}
  BinarySearchSet(std::initializer_list<value_type> list) {
    array_.reserve(list.size());
    for (auto& v : list) {
      insert(std::move(v));
    }
  }

  template <size_t N2>
  BinarySearchSet(const BinarySearchSet<K, N2, Compare, Stat>& other)
      : BinarySearchArray<K, void, N, Compare, Stat>(other) {}

  template <size_t N2>
  BinarySearchSet(BinarySearchSet<K, N2, Compare, Stat>&& other)
      : BinarySearchArray<K, void, N, Compare, Stat>(std::move(other)) {}

  template <size_t N2>
  BinarySearchSet& operator=(
      const BinarySearchSet<K, N2, Compare, Stat>& other) {
    array_ = other.array_;
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  BinarySearchSet& operator=(BinarySearchSet<K, N2, Compare, Stat>&& other) {
    array_ = std::move(other.array_);
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return insert(K(std::forward<Args>(args)...));
  }

  template <size_t N2>
  bool operator==(const BinarySearchSet<K, N2, Compare, Stat>& other) const {
    return array_ == other.array_;
  }

  template <size_t N2>
  bool operator!=(const BinarySearchSet<K, N2, Compare, Stat>& other) const {
    return array_ != other.array_;
  }
};

template <class K, class T, class KeyPolicy, size_t N, bool Stat>
struct LinearSearchMap : public LinearSearchArray<K, T, KeyPolicy, N, Stat> {
 protected:
  template <class K2, class T2, class KeyPolicy2, size_t N2, bool Stat2>
  friend struct LinearSearchMap;

  using MapStatisticsBase<Stat>::UpdateMaxCount;
  using MapStatisticsBase<Stat>::IncreaseInsertCount;
  using MapStatisticsBase<Stat>::IncreaseEraseCount;
  using MapStatisticsBase<Stat>::RecordFind;

  using KeyValueArray<K, T, KeyPolicy, N, Stat>::array_;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::_key_hash_data;
  using typename KeyValueArray<K, T, KeyPolicy, N, Stat>::value_extractor;

  using LinearSearchArray<K, T, KeyPolicy, N, Stat>::_reduced_key_hash;

 public:
  using LinearSearchArray<K, T, KeyPolicy, N, Stat>::is_key_hash;
  using LinearSearchArray<K, T, KeyPolicy, N, Stat>::consecutive_key;
  using typename LinearSearchArray<K, T, KeyPolicy, N, Stat>::insert_value_type;
  using typename LinearSearchArray<K, T, KeyPolicy, N, Stat>::value_type;
  using typename LinearSearchArray<K, T, KeyPolicy, N, Stat>::store_value_type;
  using typename LinearSearchArray<K, T, KeyPolicy, N, Stat>::iterator;
  using typename LinearSearchArray<K, T, KeyPolicy, N, Stat>::store_iterator;
  using LinearSearchArray<K, T, KeyPolicy, N, Stat>::insert;
  using LinearSearchArray<K, T, KeyPolicy, N, Stat>::find_exact;
  using KeyValueArray<K, T, KeyPolicy, N, Stat>::end;

  LinearSearchMap() {}
  LinearSearchMap(std::initializer_list<insert_value_type> list) {
    array_.reserve(list.size());
    for (auto& v : list) {
      insert(std::move(v));
    }
  }

  // Only when not using hash, the internal array can be moved from source.
  // Because we are using extra leading bytes of array buffer to store hash
  // values.
  template <size_t ExtraBytesPerElement, bool CountRealloc,
            typename = std::enable_if_t<ExtraBytesPerElement == 0>>
  LinearSearchMap(Vector<store_value_type, ExtraBytesPerElement, CountRealloc>&&
                      source_array)
      : LinearSearchArray<K, T, KeyPolicy, N, Stat>(std::move(source_array)) {
    static_assert(!is_key_hash);
  }

  template <size_t ExtraBytesPerElement, bool CountRealloc,
            typename = std::enable_if_t<ExtraBytesPerElement == 0>>
  LinearSearchMap& operator=(Vector<store_value_type, ExtraBytesPerElement,
                                    CountRealloc>&& source_array) {
    static_assert(!is_key_hash);
    array_ = std::move(source_array);
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  LinearSearchMap(const LinearSearchMap<K, T, KeyPolicy, N2, Stat>& other)
      : LinearSearchArray<K, T, KeyPolicy, N, Stat>(other) {}

  template <size_t N2>
  LinearSearchMap(LinearSearchMap<K, T, KeyPolicy, N2, Stat>&& other)
      : LinearSearchArray<K, T, KeyPolicy, N, Stat>(std::move(other)) {}

  template <size_t N2>
  LinearSearchMap& operator=(
      const LinearSearchMap<K, T, KeyPolicy, N2, Stat>& other) {
    array_ = other.array_;
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  LinearSearchMap& operator=(
      LinearSearchMap<K, T, KeyPolicy, N2, Stat>&& other) {
    array_ = std::move(other.array_);
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <class U>
  T& operator[](U&& key) {
    return value_extractor()(*try_insert(std::forward<U>(key)).first);
  }

  template <class U>
  T& at(U&& key) {
    return value_extractor()(*try_insert(std::forward<U>(key)).first);
  }

  template <class U>
  std::pair<iterator, bool> insert_default_if_absent(U&& key) {
    return try_insert(std::forward<U>(key));
  }

  template <class U>
  std::pair<iterator, bool> insert_if_absent(U&& key, const T& obj) {
    auto result = try_insert(std::forward<U>(key));
    if (result.second) {
      // Insertion took place and that means key not exists before.
      value_extractor()(*result.first) = obj;
    }
    return result;
  }

  template <class U>
  std::pair<iterator, bool> insert_if_absent(U&& key, T&& obj) {
    auto result = try_insert(std::forward<U>(key));
    if (result.second) {
      // Insertion took place and that means key not exists before.
      value_extractor()(*result.first) = std::move(obj);
    }
    return result;
  }

  template <class V>
  std::pair<iterator, bool> insert_or_assign(const K& key, V&& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (pos != nullptr) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      value_extractor()(*pos) = std::forward<V>(value);
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(
                      &array_.emplace_back(std::forward<V>(value))),
                  true};
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(
                      std::piecewise_construct, std::forward_as_tuple(key),
                      std::forward_as_tuple(std::forward<V>(value)))),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  template <class V>
  std::pair<iterator, bool> insert_or_assign(K&& key, V&& value) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (pos != nullptr) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      value_extractor()(*pos) = std::forward<V>(value);
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(
                      &array_.emplace_back(std::forward<V>(value))),
                  true};
      } else {
        result = {
            reinterpret_cast<iterator>(&array_.emplace_back(
                std::piecewise_construct, std::forward_as_tuple(std::move(key)),
                std::forward_as_tuple(std::forward<V>(value)))),
            true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  // No check of key existence.
  template <class... Args>
  iterator emplace_unique(const K& key, Args&&... args) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
    }

    iterator result;
    if constexpr (consecutive_key) {
      result = reinterpret_cast<iterator>(
          &array_.emplace_back(std::forward<Args>(args)...));
    } else {
      result = reinterpret_cast<iterator>(&array_.emplace_back(
          std::piecewise_construct, std::forward_as_tuple(key),
          std::forward_as_tuple(std::forward<Args>(args)...)));
    }
    if constexpr (is_key_hash) {
      // Store hash value after array insertion because array may expand.
      _key_hash_data()[array_.size() - 1] = hash;
    }
    return result;
  }

  template <class... Args>
  iterator emplace_unique(K&& key, Args&&... args) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
    }

    iterator result;
    if constexpr (consecutive_key) {
      result = reinterpret_cast<iterator>(
          &array_.emplace_back(std::forward<Args>(args)...));
    } else {
      result = reinterpret_cast<iterator>(&array_.emplace_back(
          std::piecewise_construct, std::forward_as_tuple(std::move(key)),
          std::forward_as_tuple(std::forward<Args>(args)...)));
    }
    if constexpr (is_key_hash) {
      // Store hash value after array insertion because array may expand.
      _key_hash_data()[array_.size() - 1] = hash;
    }
    return result;
  }

  template <typename... Args1, typename... Args2>
  iterator emplace_unique(std::piecewise_construct_t,
                          std::tuple<Args1...> args1,
                          std::tuple<Args2...> args2) {
    // Construct key in first place and find.
    K key = std::make_from_tuple<K>(std::move(args1));

    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
    }

    // Do emplace.
    iterator result;
    if constexpr (consecutive_key) {
      std::apply(
          [&](auto&&... unpacked_args) {
            result = reinterpret_cast<iterator>(&array_.emplace_back(
                std::forward<decltype(unpacked_args)>(unpacked_args)...));
          },
          std::move(args2));
    } else {
      result = reinterpret_cast<iterator>(&array_.emplace_back(
          std::piecewise_construct, std::forward_as_tuple(std::move(key)),
          std::move(args2)));
    }
    if constexpr (is_key_hash) {
      // Store hash value after array insertion because array may expand.
      _key_hash_data()[array_.size() - 1] = hash;
    }
    return result;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(const K& key, Args&&... args) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(
                      &array_.emplace_back(std::forward<Args>(args)...)),
                  true};
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(
                      std::piecewise_construct, std::forward_as_tuple(key),
                      std::forward_as_tuple(std::forward<Args>(args)...))),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(K&& key, Args&&... args) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(
                      &array_.emplace_back(std::forward<Args>(args)...)),
                  true};
      } else {
        result = {
            reinterpret_cast<iterator>(&array_.emplace_back(
                std::piecewise_construct, std::forward_as_tuple(std::move(key)),
                std::forward_as_tuple(std::forward<Args>(args)...))),
            true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace_or_assign(const K& key, Args&&... args) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      auto& v = value_extractor()(*pos);
      v.~T();
      new (&v) T(std::forward<Args>(args)...);
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(
                      &array_.emplace_back(std::forward<Args>(args)...)),
                  true};
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(
                      std::piecewise_construct, std::forward_as_tuple(key),
                      std::forward_as_tuple(std::forward<Args>(args)...))),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace_or_assign(K&& key, Args&&... args) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      auto& v = value_extractor()(*pos);
      v.~T();
      new (&v) T(std::forward<Args>(args)...);
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(
                      &array_.emplace_back(std::forward<Args>(args)...)),
                  true};
      } else {
        result = {
            reinterpret_cast<iterator>(&array_.emplace_back(
                std::piecewise_construct, std::forward_as_tuple(std::move(key)),
                std::forward_as_tuple(std::forward<Args>(args)...))),
            true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  template <class U, class... Args>
  std::pair<iterator, bool> try_emplace(U&& key, Args&&... args) {
    return emplace(std::forward<U>(key), std::forward<Args>(args)...);
  }

  template <typename... Args1, typename... Args2>
  std::pair<iterator, bool> emplace(std::piecewise_construct_t,
                                    std::tuple<Args1...> args1,
                                    std::tuple<Args2...> args2) {
    // Construct key in first place and find.
    K key = std::make_from_tuple<K>(std::move(args1));

    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    // Do emplace.
    std::pair<iterator, bool> result;
    if (BASE_VECTOR_UNLIKELY(pos != nullptr)) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};  // insertion failed and key is discarded
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        std::apply(
            [&](auto&&... unpacked_args) {
              result = {
                  reinterpret_cast<iterator>(&array_.emplace_back(
                      std::forward<decltype(unpacked_args)>(unpacked_args)...)),
                  true};
            },
            std::move(args2));
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(
                      std::piecewise_construct,
                      std::forward_as_tuple(std::move(key)), std::move(args2))),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  /**
   * @brief Iterates over each key-value pair in the map and applies a
   * user-provided callback.
   *
   * This method supports two types of callbacks for maximum flexibility:
   * 1.  **Callback with `void` return type**: The function will be executed for
   * every element in the map. Signature: `void(const K& key, const T& value)`
   *
   * 2.  **Callback with `bool` return type**: This allows for early termination
   * of the loop. If the callback returns `true`, the iteration stops
   * immediately. If it returns `false`, the iteration continues to the next
   * element. Signature: `bool(const K& key, const T& value)`
   *
   * @tparam Callback The type of the callback function or callable object
   * (e.g., a lambda).
   *
   * @param callback The callable object to be invoked for each key-value pair.
   * It will be perfectly forwarded.
   *
   * @note This method is enabled only if the provided `Callback` is invocable
   * with arguments of type `const K&` and `const T&`.
   * @note The method uses `if constexpr` to create a zero-overhead abstraction,
   * ensuring that the check for a boolean return value happens at compile-time
   * with no runtime cost.
   *
   * @code
   * MyMap<std::string, int> map = {{"a", 1}, {"b", 2}};
   *
   * // Example 1: Print all elements (void return).
   * map.for_each([](const std::string& key, int value) {
   *   std::cout << key << ": " << value << std::endl;
   * });
   *
   * // Example 2: Find an element and stop (bool return).
   * map.for_each([](const std::string& key, int value) -> bool {
   *   if (key == "b") {
   *     std::cout << "Found 'b'!" << std::endl;
   *     return true; // Stop iterating
   *   }
   *   return false; // Continue iterating
   * });
   * @endcode
   */
  template <typename Callback, typename = std::enable_if_t<std::is_invocable_v<
                                   Callback, const K&, const T&>>>
  void for_each(Callback&& callback) const {
    using callback_ret_type =
        std::invoke_result_t<Callback, const K&, const T&>;
    constexpr bool callback_returns_bool =
        std::is_same_v<callback_ret_type, bool>;
    const auto sz = array_.size();
    auto* begin = array_.begin();
    if constexpr (consecutive_key) {
      const auto* hash_data = _key_hash_data();
      if constexpr (callback_returns_bool) {
        for (size_t i = 0; i < sz; i++) {
          if (callback(static_cast<K>(hash_data[i]), *(begin + i))) {
            break;
          }
        }
      } else {
        for (size_t i = 0; i < sz; i++) {
          callback(static_cast<K>(hash_data[i]), *(begin + i));
        }
      }
    } else {
      if constexpr (callback_returns_bool) {
        for (size_t i = 0; i < sz; i++) {
          auto it = begin + i;
          if (callback(it->first, it->second)) {
            break;
          }
        }
      } else {
        for (size_t i = 0; i < sz; i++) {
          auto it = begin + i;
          callback(it->first, it->second);
        }
      }
    }
  }

  template <typename Callback, typename = std::enable_if_t<
                                   std::is_invocable_v<Callback, const K&, T&>>>
  void for_each(Callback&& callback) {
    using callback_ret_type = std::invoke_result_t<Callback, const K&, T&>;
    constexpr bool callback_returns_bool =
        std::is_same_v<callback_ret_type, bool>;
    const auto sz = array_.size();
    auto* begin = array_.begin();
    if constexpr (consecutive_key) {
      const auto* hash_data = _key_hash_data();
      if constexpr (callback_returns_bool) {
        for (size_t i = 0; i < sz; i++) {
          if (callback(static_cast<K>(hash_data[i]), *(begin + i))) {
            break;
          }
        }
      } else {
        for (size_t i = 0; i < sz; i++) {
          callback(static_cast<K>(hash_data[i]), *(begin + i));
        }
      }
    } else {
      if constexpr (callback_returns_bool) {
        for (size_t i = 0; i < sz; i++) {
          auto it = begin + i;
          if (callback(it->first, it->second)) {
            break;
          }
        }
      } else {
        for (size_t i = 0; i < sz; i++) {
          auto it = begin + i;
          callback(it->first, it->second);
        }
      }
    }
  }

  template <size_t N2>
  bool operator==(
      const LinearSearchMap<K, T, KeyPolicy, N2, Stat>& other) const {
    // Data in self is not ordered, we cannot compare array_ directly.
    if (array_.size() != other.array_.size()) {
      return false;
    }
    bool equal = true;
    for_each([&](const K& k, const T& v) -> bool {
      auto it = other.find(k);
      if (it == other.value_end()) {
        equal = false;
        return true;  // stop
      }
      if (it->second != v) {
        equal = false;
        return true;  // stop
      }
      return false;  // continue
    });
    return equal;
  }

  template <size_t N2>
  bool operator!=(
      const LinearSearchMap<K, T, KeyPolicy, N2, Stat>& other) const {
    return !(*this == other);
  }

 protected:
  std::pair<iterator, bool> try_insert(const K& key) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (pos != nullptr) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(&array_.emplace_back()), true};
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(
                      std::piecewise_construct, std::forward_as_tuple(key),
                      std::tuple<>())),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }

  std::pair<iterator, bool> try_insert(K&& key) {
    [[maybe_unused]] KeyPolicyReducedHashValueType hash;
    iterator pos;
    if constexpr (is_key_hash) {
      hash = _reduced_key_hash(key);
      pos = find_exact(key, hash);  // find with hash value
    } else {
      pos = find_exact(key);
    }

    std::pair<iterator, bool> result;
    if (pos != nullptr) {
      RecordFind(MapStatisticsFindKind::kInsertFindCollision, array_.size());
      result = {pos, false};
    } else {
      RecordFind(MapStatisticsFindKind::kInsertFind, array_.size());
      if constexpr (consecutive_key) {
        result = {reinterpret_cast<iterator>(&array_.emplace_back()), true};
      } else {
        result = {reinterpret_cast<iterator>(&array_.emplace_back(
                      std::piecewise_construct,
                      std::forward_as_tuple(std::move(key)), std::tuple<>())),
                  true};
      }
      if constexpr (is_key_hash) {
        // Store hash value after array insertion because array may expand.
        _key_hash_data()[array_.size() - 1] = hash;
      }
    }
    return result;
  }
};

template <class K, class KeyPolicy, size_t N, bool Stat>
struct LinearSearchSet : public LinearSearchArray<K, void, KeyPolicy, N, Stat> {
 protected:
  template <class K2, class KeyPolicy2, size_t N2, bool Stat2>
  friend struct LinearSearchSet;

  using MapStatisticsBase<Stat>::UpdateMaxCount;

  using LinearSearchArray<K, void, KeyPolicy, N, Stat>::array_;

 public:
  using LinearSearchArray<K, void, KeyPolicy, N, Stat>::is_key_hash;
  using typename LinearSearchArray<K, void, KeyPolicy, N,
                                   Stat>::insert_value_type;
  using typename LinearSearchArray<K, void, KeyPolicy, N, Stat>::value_type;
  using
      typename LinearSearchArray<K, void, KeyPolicy, N, Stat>::store_value_type;
  using typename LinearSearchArray<K, void, KeyPolicy, N, Stat>::iterator;
  using LinearSearchArray<K, void, KeyPolicy, N, Stat>::insert;

  LinearSearchSet() {}
  LinearSearchSet(std::initializer_list<insert_value_type> list) {
    array_.reserve(list.size());
    for (auto& v : list) {
      insert(std::move(v));
    }
  }

  // Only when not using hash, the internal array can be moved from source.
  // Because we are using extra leading bytes of array buffer to store hash
  // values.
  template <size_t ExtraBytesPerElement, bool CountRealloc,
            typename = std::enable_if_t<ExtraBytesPerElement == 0>>
  LinearSearchSet(Vector<store_value_type, ExtraBytesPerElement, CountRealloc>&&
                      source_array)
      : LinearSearchArray<K, void, KeyPolicy, N, Stat>(
            std::move(source_array)) {
    static_assert(!is_key_hash);
  }

  template <size_t ExtraBytesPerElement, bool CountRealloc,
            typename = std::enable_if_t<ExtraBytesPerElement == 0>>
  LinearSearchSet& operator=(Vector<store_value_type, ExtraBytesPerElement,
                                    CountRealloc>&& source_array) {
    static_assert(!is_key_hash);
    array_ = std::move(source_array);
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  LinearSearchSet(const LinearSearchSet<K, KeyPolicy, N2, Stat>& other)
      : LinearSearchArray<K, void, KeyPolicy, N, Stat>(other) {}

  template <size_t N2>
  LinearSearchSet(LinearSearchSet<K, KeyPolicy, N2, Stat>&& other)
      : LinearSearchArray<K, void, KeyPolicy, N, Stat>(std::move(other)) {}

  template <size_t N2>
  LinearSearchSet& operator=(
      const LinearSearchSet<K, KeyPolicy, N2, Stat>& other) {
    array_ = other.array_;
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <size_t N2>
  LinearSearchSet& operator=(LinearSearchSet<K, KeyPolicy, N2, Stat>&& other) {
    array_ = std::move(other.array_);
    UpdateMaxCount(array_.size());
    return *this;
  }

  template <class... Args>
  std::pair<iterator, bool> emplace(Args&&... args) {
    return insert(K(std::forward<Args>(args)...));
  }

  template <size_t N2>
  bool operator==(const LinearSearchSet<K, KeyPolicy, N2, Stat>& other) const {
    // Data in self is not ordered, we cannot compare array_ directly.
    if (array_.size() != other.array_.size()) {
      return false;
    }
    for (const auto& k : *this) {
      if (!other.contains(k)) {
        return false;
      }
    }
    return true;
  }

  template <size_t N2>
  bool operator!=(const LinearSearchSet<K, KeyPolicy, N2, Stat>& other) const {
    return !(*this == other);
  }
};

template <class K, class T, class Compare = std::less<K>>
using OrderedFlatMap = BinarySearchMap<K, T, 0, Compare, false>;

template <class K, class T, size_t N, class Compare = std::less<K>>
using InlineOrderedFlatMap = BinarySearchMap<K, T, N, Compare, false>;

template <class K, class Compare = std::less<K>>
using OrderedFlatSet = BinarySearchSet<K, 0, Compare, false>;

template <class K, size_t N, class Compare = std::less<K>>
using InlineOrderedFlatSet = BinarySearchSet<K, N, Compare, false>;

template <class K, class T, class KeyPolicy = ReducedHashKeyPolicy<K>>
using LinearFlatMap = LinearSearchMap<K, T, KeyPolicy, 0, false>;

template <class K, class T, size_t N, class KeyPolicy = ReducedHashKeyPolicy<K>>
using InlineLinearFlatMap = LinearSearchMap<K, T, KeyPolicy, N, false>;

template <class K, class KeyPolicy = ReducedHashKeyPolicy<K>>
using LinearFlatSet = LinearSearchSet<K, KeyPolicy, 0, false>;

template <class K, size_t N, class KeyPolicy = ReducedHashKeyPolicy<K>>
using InlineLinearFlatSet = LinearSearchSet<K, KeyPolicy, N, false>;

// As HybridMap's policy template argument for inlined flat map types.
// The generic MapPolicy template wont't accept InlineOrderedFlatMap
// which contains non-type template arguments.
template <template <typename, typename, size_t, typename...> class T, size_t N,
          typename... Args>
struct InlineFlatMapPolicy {
  template <typename Key, typename Value>
  using type = T<Key, Value, N, Args...>;
};

/**
 Helper to declare inlined version of existing map type.
    using MapType = OrderedFlatMap<int, int>;
    Inlined<MapType, 5>::type inlined_map;
 */
template <class From, size_t N>
struct Inlined {
  using type = std::conditional_t<
      std::is_same_v<From, OrderedFlatMap<typename From::key_type,
                                          typename From::mapped_type,
                                          typename From::key_compare>>,
      InlineOrderedFlatMap<typename From::key_type, typename From::mapped_type,
                           N, typename From::key_compare>,
      std::conditional_t<
          std::is_same_v<From, OrderedFlatSet<typename From::key_type,
                                              typename From::key_compare>>,
          InlineOrderedFlatSet<typename From::key_type, N,
                               typename From::key_compare>,

          std::conditional_t<
              std::is_same_v<From, LinearFlatMap<typename From::key_type,
                                                 typename From::mapped_type,
                                                 typename From::key_policy>>,
              InlineLinearFlatMap<typename From::key_type,
                                  typename From::mapped_type, N,
                                  typename From::key_policy>,

              std::conditional_t<
                  std::is_same_v<From,
                                 LinearFlatSet<typename From::key_type,
                                               typename From::key_policy>>,
                  InlineLinearFlatSet<typename From::key_type, N,
                                      typename From::key_policy>,
                  void>>>>;
};

static_assert(std::is_same_v<Inlined<OrderedFlatMap<int, int>, 5>::type,
                             InlineOrderedFlatMap<int, int, 5>>);
static_assert(std::is_same_v<Inlined<LinearFlatMap<int, int>, 5>::type,
                             InlineLinearFlatMap<int, int, 5>>);

static_assert(std::is_same_v<Inlined<OrderedFlatSet<int>, 5>::type,
                             InlineOrderedFlatSet<int, 5>>);
static_assert(std::is_same_v<Inlined<LinearFlatSet<int>, 5>::type,
                             InlineLinearFlatSet<int, 5>>);

}  // namespace base
}  // namespace lynx

namespace std {
template <class T, size_t ExtraBytesPerElement, bool CountRealloc>
inline void swap(lynx::base::Vector<T, ExtraBytesPerElement, CountRealloc>& x,
                 lynx::base::Vector<T, ExtraBytesPerElement, CountRealloc>& y) {
  x.swap(y);
}

template <class K, class T, size_t N1, size_t N2, class Compare1,
          class Compare2, bool Stat1, bool Stat2,
          typename = typename std::enable_if<N1 != N2>::type>
inline void swap(lynx::base::BinarySearchMap<K, T, N1, Compare1, Stat1>& x,
                 lynx::base::BinarySearchMap<K, T, N2, Compare2, Stat2>& y) {
  x.swap(y);
}

template <class K, size_t N1, size_t N2, class Compare1, class Compare2,
          bool Stat1, bool Stat2,
          typename = typename std::enable_if<N1 != N2>::type>
inline void swap(lynx::base::BinarySearchSet<K, N1, Compare1, Stat1>& x,
                 lynx::base::BinarySearchSet<K, N2, Compare2, Stat2>& y) {
  x.swap(y);
}

template <class K, class T, class KeyPolicy, size_t N1, size_t N2, bool Stat1,
          bool Stat2, typename = typename std::enable_if<N1 != N2>::type>
inline void swap(lynx::base::LinearSearchMap<K, T, KeyPolicy, N1, Stat1>& x,
                 lynx::base::LinearSearchMap<K, T, KeyPolicy, N2, Stat2>& y) {
  x.swap(y);
}

template <class K, class KeyPolicy, size_t N1, size_t N2, bool Stat1,
          bool Stat2, typename = typename std::enable_if<N1 != N2>::type>
inline void swap(lynx::base::LinearSearchSet<K, KeyPolicy, N1, Stat1>& x,
                 lynx::base::LinearSearchSet<K, KeyPolicy, N2, Stat2>& y) {
  x.swap(y);
}
}  // namespace std

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-template"

#endif  // BASE_INCLUDE_VECTOR_H_
