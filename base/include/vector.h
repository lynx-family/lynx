// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_VECTOR_H_
#define BASE_INCLUDE_VECTOR_H_

#include <algorithm>
#include <cassert>
#include <cstring>
#include <functional>
#include <limits>
#include <stack>
#include <type_traits>
#include <utility>

#ifdef DEBUG
#define BASE_VECTOR_DCHECK(...) assert(__VA_ARGS__)
#else
#define BASE_VECTOR_DCHECK(...)
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

BASE_VECTOR_INLINE void* HeapAlloc(const size_t size) {
  return std::malloc(size);
}

BASE_VECTOR_INLINE void HeapFree(void* ptr) { std::free(ptr); }

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

template <class T>
BASE_VECTOR_NEVER_INLINE void _nontrivial_destruct_reverse(T* begin,
                                                           size_t count) {
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
 */
template <class T>
struct VectorPrototype {
  static constexpr auto is_trivial =
      IsTrivial<T, is_instance<T, std::pair>{}>::value;
  using iterator = T*;
  using const_iterator = const T*;

  VectorPrototype() = default;

  size_t size() const { return count_; }

  bool empty() const { return count_ == 0; }

  size_t capacity() const { return std::abs(capacity_); }

  T* data() { return reinterpret_cast<T*>(_begin_iter()); }

  T* data() const { return reinterpret_cast<T*>(_begin_iter()); }

  /**
   * Returns if the array buffer is static or inplace buffer that should not be
   * freed.
   */
  bool is_static_buffer() const { return capacity_ < 0; }

 protected:
  template <class U>
  friend struct VectorPrototype;

  friend struct VectorTemplateless;

  BASE_VECTOR_NEVER_INLINE static void* _reallocate_buffer(void* array,
                                                           size_t element_size,
                                                           size_t count) {
    VectorPrototype<uint8_t>* proto_array =
        reinterpret_cast<VectorPrototype<uint8_t>*>(array);
    auto old_capacity = proto_array->capacity();
    // old*2 for consistent with std::vector of libc++.
    auto new_capacity =
        count > 0 ? count : (old_capacity == 0 ? 4 : 2 * old_capacity);
    if (new_capacity <= old_capacity) {
      return nullptr;
    }
    const auto buffer_size = new_capacity * element_size;
    auto buffer = HeapAlloc(buffer_size);
    if (buffer == nullptr) {
      return nullptr;
    }
    proto_array->_set_memory(buffer);
    proto_array->capacity_ = static_cast<int32_t>(new_capacity);
    return buffer;
  }

  BASE_VECTOR_NEVER_INLINE void _reallocate_nontrivial(size_t count = 0) {
    static_assert(
        !is_trivial,
        "Only for non-trivial element types you can use this method.");
    auto prev_begin = _begin_iter();
    bool need_free = !is_static_buffer();
    auto buffer =
        VectorPrototype<uint8_t>::_reallocate_buffer(this, sizeof(T), count);
    if (buffer && prev_begin) {
      /* Move objects of T from old buffer to new buffer. */
      _nontrivial_construct_move((iterator)buffer, prev_begin, size());

      /* Destruct objects on previous buffer and free memory. */
      _nontrivial_destruct_reverse(prev_begin, size());
      if (need_free) {
        HeapFree(prev_begin);
      }
    }
  }

  void _free() {
    if (memory_ != nullptr) {
      if constexpr (!is_trivial) {
        _nontrivial_destruct_reverse(_begin_iter(), size());
      }
      if (!is_static_buffer()) {
        HeapFree(_begin_iter());
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

  /**
   * @brief Set capacity value by sub-classes.
   * @param value Capacity value.
   * @param static_buffer If true, capacity value will be stored as negative and
   *   the buffer will not be freed in reallocation or destructor.
   */
  BASE_VECTOR_INLINE void _set_capacity(size_t value, bool static_buffer) {
    capacity_ = static_buffer ? -static_cast<int32_t>(value)
                              : static_cast<int32_t>(value);
  }

  BASE_VECTOR_INLINE void _transfer_from(VectorPrototype& other) {
    // Memory buffer of other array might be an inplace buffer which is
    // unsafe to be directly transferred ownership to self.
    BASE_VECTOR_DCHECK(!other.is_static_buffer());

    memory_ = other.memory_;
    count_ = other.count_;
    capacity_ = other.capacity_;
  }

  template <class U>
  BASE_VECTOR_INLINE void _transfer_as_byte_array_from(
      VectorPrototype<U>& other) {
    // Memory buffer of other array might be an inplace buffer which is
    // unsafe to be directly transferred ownership to self.
    BASE_VECTOR_DCHECK(!other.is_static_buffer());

    memory_ = reinterpret_cast<T*>(other.memory_);
    count_ = static_cast<uint32_t>(other.count_ * sizeof(U));
    capacity_ = static_cast<int32_t>(other.capacity_ * sizeof(U));
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
 This is why we use Vector to replace Vector for JS
 interoperability. Note that templateless manipulators should only be used for
 trivial T types.
*/
struct VectorTemplateless {
  BASE_VECTOR_NEVER_INLINE static void ReallocateTrivial(void* array,
                                                         size_t element_size,
                                                         size_t count) {
    VectorPrototype<uint8_t>* proto_array =
        reinterpret_cast<VectorPrototype<uint8_t>*>(array);
    /**
     Vector is also used for ECS dynamic buffer component storage.
     Buffer may be initially in chunk and the capacity_ value is negative
     to indicate that the memory should not be freed.
     */
    const bool free_prev = !proto_array->is_static_buffer();
    auto prev_begin = proto_array->_begin_iter();
    auto buffer = VectorPrototype<uint8_t>::_reallocate_buffer(
        array, element_size, count);
    if (buffer && prev_begin) {
      std::memcpy(buffer, prev_begin, proto_array->size() * element_size);
      if (free_prev) {
        HeapFree(prev_begin);
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
    VectorPrototype<uint8_t>* proto_array =
        reinterpret_cast<VectorPrototype<uint8_t>*>(array);
    bool reallocated = false;
    if (count > proto_array->size()) {
      if (count > proto_array->capacity()) {
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

  BASE_VECTOR_NEVER_INLINE static void* Insert(void* array, size_t element_size,
                                               void* dest, const void* source) {
    VectorPrototype<uint8_t>* proto_array =
        reinterpret_cast<VectorPrototype<uint8_t>*>(array);
    auto diff = static_cast<uint8_t*>(dest) - proto_array->_begin_iter();
    if (proto_array->size() == proto_array->capacity()) {
      ReallocateTrivial(array, element_size);
    }
    auto pos = proto_array->_begin_iter() + diff;
    auto new_last =
        proto_array->_begin_iter() + proto_array->size() * element_size;
    if (pos > new_last) {
      BASE_VECTOR_DCHECK(false);
    } else {
      if (pos < new_last) {
        std::memmove(pos + element_size, pos, new_last - pos);
      }
      std::memcpy(pos, source, element_size);
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
    VectorPrototype<uint8_t>* proto_array =
        reinterpret_cast<VectorPrototype<uint8_t>*>(array);
    if (index >= proto_array->size()) {
      return false;
    }
    if (index + count > proto_array->size()) {
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
    if (source == nullptr) {
      return;
    }
    VectorPrototype<uint8_t>* proto_array =
        reinterpret_cast<VectorPrototype<uint8_t>*>(array);
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
    if (source_count == 0) {
      return;
    }
    VectorPrototype<uint8_t>* proto_array =
        reinterpret_cast<VectorPrototype<uint8_t>*>(array);
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
template <class T>
struct Vector : protected VectorTemplateless, public VectorPrototype<T> {
  using iterator = typename VectorPrototype<T>::iterator;
  using const_iterator = typename VectorPrototype<T>::const_iterator;
  using VectorPrototype<T>::is_trivial;
  using VectorPrototype<T>::size;
  using VectorPrototype<T>::capacity;
  using VectorPrototype<T>::is_static_buffer;
  using VectorPrototype<T>::_begin_iter;
  using VectorPrototype<T>::_end_iter;
  using VectorPrototype<T>::_finish_iter;
  using VectorPrototype<T>::_free;
  using VectorPrototype<T>::_transfer_from;
  using VectorPrototype<T>::_transfer_as_byte_array_from;
  using VectorPrototype<T>::_swap;
  using VectorPrototype<T>::_reset;
  using VectorPrototype<T>::_set_count;
  using VectorPrototype<T>::_inc_count;
  using VectorPrototype<T>::_dec_count;
  using VectorPrototype<T>::_reallocate_nontrivial;

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
  Vector() = default;
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
  }

  Vector& operator=(const Vector& other) {
    if (this != &other) {
      clear();
      _from(other.data(), other.size(), _nontrivial_construct_copy);
    }
    return *this;
  }

  Vector& operator=(std::initializer_list<T> list) {
    clear();
    _from(list.begin(), list.size(), _nontrivial_construct_move);
    return *this;
  }

  Vector(Vector&& other) {
    if (other.is_static_buffer()) {
      _from(other.data(), other.size(), _nontrivial_construct_move);
      other.clear();
    } else {
      _transfer_from(other);
      other._reset();
    }
  }

  Vector& operator=(Vector&& other) {
    if (this != &other) {
      if (other.is_static_buffer()) {
        clear();
        _from(other.data(), other.size(), _nontrivial_construct_move);
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

  reference push_back(const T& v) {
    _grow_if_need();
    auto end = _end_iter();
    ::new (static_cast<void*>(end)) T(v);
    _inc_count();
    return *end;
  }

  reference push_back(T&& v) {
    _grow_if_need();
    auto end = _end_iter();
    ::new (static_cast<void*>(end)) T(std::move(v));
    _inc_count();
    return *end;
  }

  template <class... Args>
  reference emplace_back(Args&&... args) {
    if constexpr (std::is_integral_v<T> || std::is_pointer_v<T>) {
      return push_back(std::forward<Args>(args)...);
    } else {
      _grow_if_need();
      auto end = _end_iter();
      ::new (static_cast<void*>(end)) T(std::forward<Args>(args)...);
      _inc_count();
      return *end;
    }
  }

  void pop_back() {
    if (size() == 0) {
      return;
    }
    if constexpr (!is_trivial) {
      _nontrivial_destruct_reverse(_end_iter() - 1, 1);
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

  const_iterator erase(const_iterator pos) {
    return (const_iterator)erase((iterator)pos);
  }

  iterator erase(iterator first, iterator last) {
    BASE_VECTOR_DCHECK(first <= last);
    if (first != last) {
      if constexpr (is_trivial) {
        std::memmove(first, last, (_end_iter() - last) * sizeof(T));
      } else {
        _nontrivial_destruct_reverse(
            _nontrivial_move_forward(first, last, _end_iter() - last),
            last - first);
      }
      _dec_count(last - first);
    }
    return first;
  }

  template <class... Args>
  iterator emplace(const iterator pos, Args&&... args) {
    // Not real emplace. Anyway, emplace/insert for array is not recommended.
    T temp(std::forward<Args>(args)...);
    return insert(pos, std::move(temp));
  }

  iterator insert(std::nullptr_t,
                  const T& value) = delete;  // Avoid misuse of insert(0)
  iterator insert(const iterator pos, const T& value) {
    if constexpr (is_trivial) {
      // For trivial types, always call templateless implementation for binary
      // size benefits.
      return (iterator)Insert(this, sizeof(T), pos, &value);
    } else {
      auto diff = pos - _begin_iter();
      _grow_if_need();
      auto dest_pos = _begin_iter() + diff;
      auto new_last = _end_iter();
      if (dest_pos > new_last) {
        BASE_VECTOR_DCHECK(false);
      } else {
        if (dest_pos < new_last) {
          // Construct new T at end, and move previous back item to it.
          _nontrivial_construct_move(new_last, new_last - 1, 1);
          // Move left objects.
          _nontrivial_move_backward(new_last - 1, new_last - 2,
                                    new_last - dest_pos - 1);
          // Copy assign value to dest_pos.
          *dest_pos = value;
        } else {
          // dest_pos == new_last, use constructor for non-trivial T.
          ::new (static_cast<void*>(dest_pos)) T(value);
        }
        _inc_count();
      }
      return dest_pos;
    }
  }

  iterator insert(std::nullptr_t,
                  T&& value) = delete;  // Avoid misuse of insert(0)
  iterator insert(const iterator pos, T&& value) {
    if constexpr (is_trivial) {
      // For trivial types, always call templateless implementation for binary
      // size benefits.
      return (iterator)Insert(this, sizeof(T), pos, &value);
    } else {
      auto diff = pos - _begin_iter();
      _grow_if_need();
      auto dest_pos = _begin_iter() + diff;
      auto new_last = _end_iter();
      if (dest_pos > new_last) {
        BASE_VECTOR_DCHECK(false);
      } else {
        if (dest_pos < new_last) {
          // Construct new T at end, and move previous back item to it.
          _nontrivial_construct_move(new_last, new_last - 1, 1);
          // Move left objects.
          _nontrivial_move_backward(new_last - 1, new_last - 2,
                                    new_last - dest_pos - 1);
          // Move assign value to dest_pos.
          *dest_pos = std::move(value);
        } else {
          // dest_pos == new_last, use constructor for non-trivial T.
          ::new (static_cast<void*>(dest_pos)) T(std::move(value));
        }
        _inc_count();
      }
      return dest_pos;
    }
  }

  /**
   * @brief Reserve capacity.
   * @return True if reallocation occurs.
   */
  bool reserve(size_t count) {
    if (count > capacity()) {
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
    // No op.
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
      _set_count(static_cast<uint32_t>(count));
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
      _set_count(static_cast<uint32_t>(count));
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
      _set_count(static_cast<uint32_t>(count));
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
    VectorTemplateless::Fill(this, sizeof(T), data, byte_size, position);
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
      const Vector<U>& other) {
    if (!other.empty()) {
      fill(other.data(), other.size() * sizeof(U), size());
    }
  }

  /**
   * @brief Transfer buffer to result array and set length of result array to
   * byte length of data.
   */
  template <class U = T>
  typename std::enable_if<std::is_same_v<U, T> && is_trivial && (sizeof(T) > 1),
                          Vector<uint8_t>>::type
  transfer_to_byte_array() {
    Vector<uint8_t> result;
    result._transfer_as_byte_array_from<T>(*this);
    _reset();
    return result;
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

 protected:
  template <class U>
  struct always_false : std::false_type {};

  void _reallocate(size_t value) {
    if constexpr (is_trivial) {
      ReallocateTrivial(this, sizeof(T), value);
    } else {
      _reallocate_nontrivial(value);
    }
  }

  void _grow_if_need() {
    if (size() == capacity()) {
      // _grow_if_need is inlined at caller side. Although we can call
      // _reallocate(0) but implement standalone for binary size optimization.
      if constexpr (is_trivial) {
        ReallocateTrivial(this, sizeof(T));
      } else {
        _reallocate_nontrivial(0);
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
        _set_count(static_cast<uint32_t>(size));
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
        _set_count(static_cast<uint32_t>(count));
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
        _set_count(static_cast<uint32_t>(count));
      }
    }
  }
};

template <class T>
inline bool operator==(const Vector<T>& __x, const Vector<T>& __y) {
  const typename Vector<T>::size_type __sz = __x.size();
  return __sz == __y.size() && std::equal(__x.begin(), __x.end(), __y.begin());
}

template <class T>
inline bool operator!=(const Vector<T>& __x, const Vector<T>& __y) {
  return !(__x == __y);
}

template <class T>
inline bool operator<(const Vector<T>& __x, const Vector<T>& __y) {
  return std::lexicographical_compare(__x.begin(), __x.end(), __y.begin(),
                                      __y.end());
}

template <class T>
inline bool operator>(const Vector<T>& __x, const Vector<T>& __y) {
  return __y < __x;
}

template <class T>
inline bool operator>=(const Vector<T>& __x, const Vector<T>& __y) {
  return !(__x < __y);
}

template <class T>
inline bool operator<=(const Vector<T>& __x, const Vector<T>& __y) {
  return !(__y < __x);
}

using ByteArray = Vector<uint8_t>;

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
template <class T, size_t N>
struct InlineVector : public Vector<T> {
  using iterator = typename VectorPrototype<T>::iterator;
  using const_iterator = typename VectorPrototype<T>::const_iterator;
  using VectorPrototype<T>::is_trivial;
  using VectorPrototype<T>::size;
  using VectorPrototype<T>::capacity;
  using VectorPrototype<T>::_set_memory;
  using VectorPrototype<T>::_set_capacity;
  using Vector<T>::_begin_iter;
  using Vector<T>::_end_iter;
  using Vector<T>::reserve;
  using Vector<T>::clear;
  using Vector<T>::clear_and_shrink;
  using Vector<T>::_from;
  using Vector<T>::_construct_fill;
  using Vector<T>::_construct_fill_default;

  static constexpr size_t kInlinedSize = N;

  InlineVector() : Vector<T>() {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
  }

  InlineVector(std::nullptr_t) : InlineVector() {}  // NOLINT

  explicit InlineVector(size_t count) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    _construct_fill_default(count);
  }

  InlineVector(size_t count, const T& value) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    _construct_fill(count, value);
  }

  InlineVector(const_iterator begin, const_iterator end) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    _from(begin, end - begin, _nontrivial_construct_copy);
  }

  InlineVector(iterator begin, iterator end) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    _from(begin, end - begin, _nontrivial_construct_copy);
  }

  InlineVector(std::initializer_list<T> list) : Vector<T>() {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    *this = std::move(list);
  }

  InlineVector(const Vector<T>& other) {  // NOLINT
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    *this = other;
  }

  InlineVector(const InlineVector& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    *this = other;
  }

  template <size_t N2>
  InlineVector(const InlineVector<T, N2>& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    *this = other;
  }

  InlineVector(Vector<T>&& other) {  // NOLINT
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    *this = std::move(other);
  }

  InlineVector(InlineVector&& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    *this = std::move(other);
  }

  template <size_t N2>
  InlineVector(InlineVector<T, N2>&& other) {
    static_assert(N > 0,
                  "InlineVector must have initial capacity larger than 0.");
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
    *this = std::move(other);
  }

  InlineVector& operator=(std::initializer_list<T> list) {
    reinterpret_cast<Vector<T>*>(this)->operator=(std::move(list));
    return *this;
  }

  InlineVector& operator=(const Vector<T>& other) {
    reinterpret_cast<Vector<T>*>(this)->operator=(other);
    return *this;
  }

  InlineVector& operator=(const InlineVector& other) {
    reinterpret_cast<Vector<T>*>(this)->operator=(other);
    return *this;
  }

  template <size_t N2>
  InlineVector& operator=(const InlineVector<T, N2>& other) {
    reinterpret_cast<Vector<T>*>(this)->operator=(other);
    return *this;
  }

  InlineVector& operator=(Vector<T>&& other) {
    if (this != &other) {
      if (other.size() > capacity()) {
        if (!other.is_static_buffer()) {
          // Just swap pointers.
          reinterpret_cast<Vector<T>*>(this)->operator=(std::move(other));
          return *this;
        } else {
          reserve(other.size());
        }
      }

      // Trying best to use inplace buffer, move items of other to self.
      clear();
      _from(other.begin(), other.size(), _nontrivial_construct_move);
      other.clear();
    }
    return *this;
  }

  InlineVector& operator=(InlineVector&& other) {
    return operator=(std::move(*reinterpret_cast<Vector<T>*>(&other)));
  }

  template <size_t N2>
  InlineVector& operator=(InlineVector<T, N2>&& other) {
    return operator=(std::move(*reinterpret_cast<Vector<T>*>(&other)));
  }

  void clear_and_shrink() {
    Vector<T>::clear_and_shrink();
    _set_capacity(N, true);
    _set_memory(&inplace_buffer_[0]);
  }

 private:
  alignas(std::max(alignof(T),
                   sizeof(void*))) uint8_t inplace_buffer_[sizeof(T) * N];
};

/**
 * @brief Stack using Vector<T> or InlineVector<T, N> as underlying container.
 */
template <class T>
using Stack = std::stack<T, Vector<T>>;

template <class T, size_t N>
using InlineStack = std::stack<T, InlineVector<T, N>>;

}  // namespace base
}  // namespace lynx

namespace std {
template <class T>
inline void swap(lynx::base::Vector<T>& x, lynx::base::Vector<T>& y) {
  x.swap(y);
}
}  // namespace std

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-template"

#endif  // BASE_INCLUDE_VECTOR_H_
