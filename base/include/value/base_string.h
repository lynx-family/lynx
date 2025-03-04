// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef BASE_INCLUDE_VALUE_BASE_STRING_H_
#define BASE_INCLUDE_VALUE_BASE_STRING_H_

#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
// TODO(yuyang), move StringTable, StringConvertHelper to other files.
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "base/include/fml/memory/ref_counted.h"

namespace lynx {
namespace base {

class RefCountedStringImpl;
class String;

namespace static_string {
template <char... chars>
struct Entry;

class StaticString;
class GenericCache;
}  // namespace static_string

struct StaticStringPayload {
  RefCountedStringImpl* impl;
  const char string[];
};

/// General implementation of String managed by atomic reference counter.
/// Constructors and factory methods of RefCountedStringImpl are hidden and
/// are only visible to String.
class RefCountedStringImpl : public fml::RefCountedThreadSafeStorage {
 public:
  ~RefCountedStringImpl() override = default;

  std::size_t hash() const { return hash_; }

  const char* c_str() const { return str_.c_str(); }
  const std::string& str() const { return str_; }

  bool empty() const { return str_.empty(); }

  std::size_t length() const { return length_; }
  std::size_t length_utf8();
  std::size_t length_utf16();

  class Unsafe {
    // ATTENTION: function under this class is UNSAFE to use.
    // Do NOT use them unless you have consulted with the owners of String.
   public:
    Unsafe() = delete;
    // For kEmptyString it is desired to be a static C++ global-initialization
    // variable when image is loaded because our base::String default
    // constructor heavily relies on it for best performance.
    static RefCountedStringImpl kEmptyString;
    static RefCountedStringImpl& kTrueString();
    static RefCountedStringImpl& kFalseString();
    static inline __attribute__((always_inline)) RefCountedStringImpl*
    RawCreate(const char* str) {
      return new RefCountedStringImpl(str);
    }

    static inline __attribute__((always_inline)) RefCountedStringImpl*
    RawCreate(std::string str) {
      return new RefCountedStringImpl(std::move(str));
    }
  };

 protected:
  void ReleaseSelf() const override { delete this; }

 private:
  std::string str_;
  std::size_t hash_;
  uint32_t length_;

  union {
    struct {
      // utf16_length_ is lazily calculated and cached when length_utf16() is
      // invoked.
      uint32_t utf16_length_ : 31;
      uint32_t utf16_len_calculated_ : 1;
    };

    uint32_t init_{0};  // initialize the anonymous struct above to all 0
  };

  RefCountedStringImpl(const char* str, std::size_t len);
  explicit RefCountedStringImpl(const char* str);
  explicit RefCountedStringImpl(std::string str);

  RefCountedStringImpl(const RefCountedStringImpl&) = delete;
  RefCountedStringImpl& operator=(const RefCountedStringImpl&) = delete;

  friend class String;
  friend class Unsafe;
  friend class static_string::StaticString;
  friend class static_string::GenericCache;

  template <char... chars>
  friend struct static_string::Entry;

  static RefCountedStringImpl* RawCreate(const char* str, std::size_t len) {
    return new RefCountedStringImpl(str, len);
  }

  static RefCountedStringImpl* RawCreateStatic(const char* str) {
    // System strlen may do tricky optimizations for long strings and
    // additional branch instructions are involved.
    // Considering that most our static strings are short ones less than
    // 20 characters, calculate length directly.
    const char* s = str;
    for (; *s; ++s) {
    }
    return new RefCountedStringImpl(str, s - str);
  }
};

/// Basic thread-safe constant string type.
class String {
 public:
  // The impl pointer is tagged of last bit if it is not strong referenced
  // by String instance.
  // Use tagged pointer instead of some enum field such as storage_type
  // to denote the pointer type for better performance because adding a enum
  // field would make String not fit into machine pointer size or register.
  static constexpr uintptr_t kPointerLastBitSetMask = 1 << 0;
  static constexpr uintptr_t kPointerLastBitClearMask =
      std::numeric_limits<uintptr_t>::max() - 1;

  static inline __attribute__((pure)) bool IsTaggedImpl(
      RefCountedStringImpl* p) {
    return (reinterpret_cast<uintptr_t>(p) & kPointerLastBitSetMask) > 0;
  }

  static inline RefCountedStringImpl* MakeTaggedImpl(RefCountedStringImpl* p) {
    return reinterpret_cast<RefCountedStringImpl*>(
        reinterpret_cast<uintptr_t>(p) | kPointerLastBitSetMask);
  }

  static inline RefCountedStringImpl* UntagImpl(RefCountedStringImpl* p) {
    return reinterpret_cast<RefCountedStringImpl*>(
        reinterpret_cast<uintptr_t>(p) & kPointerLastBitClearMask);
  }

 public:
  // Store kEmptyString as tagged and skip its AddRef()
  String()
      : ref_impl_(MakeTaggedImpl(&RefCountedStringImpl::Unsafe::kEmptyString)) {
  }
  String(const std::string& str)
      : ref_impl_(RefCountedStringImpl::Unsafe::RawCreate(str)) {}
  String(std::string&& str)
      : ref_impl_(RefCountedStringImpl::Unsafe::RawCreate(std::move(str))) {}
  String(const char* c_str)
      : ref_impl_(RefCountedStringImpl::Unsafe::RawCreate(c_str)) {}
  String(const char* c_str, std::size_t length)
      : ref_impl_(RefCountedStringImpl::RawCreate(c_str, length)) {}

  String(const String& other) {
    // Copy from a raw ref pointer one, self should be upgraded to normal
    // ref counted one.
    ref_impl_ = UntagImpl(other.ref_impl_);
    ref_impl_->AddRef();
  }

  String(String&& other) noexcept {
    // Move from a raw ref pointer, self should be upgraded to normal ref
    // counted one.
    ref_impl_ = UntagImpl(other.ref_impl_);
    if (ref_impl_ != other.ref_impl_) {
      ref_impl_->AddRef();
    }
    other.SetEmptyString();
  }

  String& operator=(const String& other) {
    String(other).swap(*this);
    return *this;
  }

  String& operator=(String&& other) {
    String(std::move(other)).swap(*this);
    return *this;
  }

  // Make it inlined so that ~StaticString() sees.
  inline __attribute__((always_inline)) ~String() {
    if (!IsTaggedImpl(ref_impl_)) {
      ref_impl_->Release();
    }
  }

  std::size_t hash() const { return UntagImpl(ref_impl_)->hash(); }

  std::string_view string_view() const {
    return std::string_view(UntagImpl(ref_impl_)->str());
  }

  const std::string& str() const { return UntagImpl(ref_impl_)->str(); }

  const char* c_str() const { return UntagImpl(ref_impl_)->c_str(); }

  bool empty() const { return UntagImpl(ref_impl_)->empty(); }

  size_t length() const { return UntagImpl(ref_impl_)->length(); }

  size_t length_utf8() const { return UntagImpl(ref_impl_)->length_utf8(); }

  size_t length_utf16() const { return UntagImpl(ref_impl_)->length_utf16(); }

  bool IsEqual(const char* other) const { return str() == other; }
  bool IsEqual(const std::string& other) const { return str() == other; }
  bool IsEqual(const String& other) const { return str() == other.str(); }

  template <size_t N>
  bool IsEquals(char const (&p)[N]) const {
    return string_view() == std::string_view(p, N - 1);
  }

  bool operator==(const String& other) const { return str() == other.str(); }
  bool operator==(const char* other) const { return str() == other; }
  bool operator==(const std::string& other) const { return str() == other; }

  bool operator!=(const String& other) const { return str() != other.str(); }
  bool operator!=(const char* other) const { return str() != other; }
  bool operator!=(const std::string& other) const { return str() != other; }

  bool operator<(const String& other) const { return str() < other.str(); }
  bool operator<(const char* other) const { return str() < other; }
  bool operator<(const std::string& other) const { return str() < other; }

  bool operator<=(const String& other) const { return str() <= other.str(); }
  bool operator<=(const char* other) const { return str() <= other; }
  bool operator<=(const std::string& other) const { return str() <= other; }

  bool operator>(const String& other) const { return str() > other.str(); }
  bool operator>(const char* other) const { return str() > other; }
  bool operator>(const std::string& other) const { return str() > other; }

  bool operator>=(const String& other) const { return str() >= other.str(); }
  bool operator>=(const char* other) const { return str() >= other; }
  bool operator>=(const std::string& other) const { return str() >= other; }

  std::size_t find(const String& other, std::size_t pos) const {
    return str().find(other.str(), pos);
  }
  std::size_t find(const char* other, std::size_t pos) const {
    return str().find(other, pos);
  }
  std::size_t find(const std::string& other, std::size_t pos) const {
    return str().find(other, pos);
  }

  class Unsafe {
    // ATTENTION: function under this class is UNSAFE to use.
    // Do NOT use them unless you have consulted with the owners of String.
   public:
    Unsafe() = delete;

    static inline __attribute__((always_inline)) String
    ConstructWeakRefStringFromRawRef(RefCountedStringImpl* str) {
      return String(str, String::kCreateAsRawRefPointerTag);
    }

    static inline __attribute__((always_inline)) String
    ConstructStringFromRawRef(RefCountedStringImpl* str) {
      return String(str);
    }

    static inline __attribute__((always_inline)) RefCountedStringImpl*
    GetStringRawRef(const String& str) {
      return str.ref_impl_;
    }

    static inline __attribute__((always_inline)) RefCountedStringImpl*
    GetUntaggedStringRawRef(const String& str) {
      return UntagImpl(str.ref_impl_);
    }

    static inline __attribute__((always_inline)) void SetStringToEmpty(
        String& str) {
      str.SetEmptyString();
    }
  };

 protected:
  friend class Unsafe;

  template <char... chars>
  friend struct static_string::Entry;

  friend class static_string::StaticString;

  RefCountedStringImpl* ref_impl_;  // Guaranteed to be non-null.

  /// Explicitly create a String instance which holds weak pointer of
  /// ref-counted impl.
  enum CreateAsRawRefPointerTag { kCreateAsRawRefPointerTag };
  String(RefCountedStringImpl* str, CreateAsRawRefPointerTag)
      : ref_impl_(MakeTaggedImpl(str)) {}

  /// No-op constructor.
  enum CreateAsUninitializedTag { kCreateAsUninitializedTag };
  String(CreateAsUninitializedTag) {}

  /// Explicitly create a String instance which retains the impl.
  explicit String(RefCountedStringImpl* str) : ref_impl_(str) {
    ref_impl_->AddRef();
  }

  inline void swap(String& other) noexcept {
    std::swap(ref_impl_, other.ref_impl_);
  }

  inline void SetEmptyString() {
    ref_impl_ = MakeTaggedImpl(&RefCountedStringImpl::Unsafe::kEmptyString);
  }
};

namespace static_string {

/// Subclass of String which is constructed from static constant C string.
/// StaticString always stores tagged impl pointer and its destructor could be
/// omitted by compiler.
class StaticString : public String {
 public:
  StaticString(RefCountedStringImpl* tagged_ref)
      : String(String::kCreateAsUninitializedTag) {
    // This constructor is used by GenericCache::operator StaticString() and
    // tagged_ref must be a tagged pointer.
    ref_impl_ = tagged_ref;
  }

  StaticString(StaticStringPayload* payload)
      : String(String::kCreateAsUninitializedTag) {
    if (payload->impl == nullptr) {
      // Directly stores tagged value to payload so that it could be
      // directly assigned to ref_impl_ later.
      payload->impl = MakeTaggedImpl(
          RefCountedStringImpl::RawCreateStatic(payload->string));
    }
    ref_impl_ = payload->impl;
  }

  inline __attribute__((always_inline)) ~StaticString() {
    // Hint for compiler to skip String::~String() because StaticString
    // never add reference count to its impl.
    // The compiler no longer outputs any assembly instructions for
    // destruction of StaticString.
    __builtin_assume(String::IsTaggedImpl(ref_impl_));
  }
};

// With C++20, we can do this more elegantly.
template <std::size_t N>
using CharArray = char const[N];

template <auto const& str, std::size_t iter = std::size(str),
          char const... chars>
struct CharArrayPack {
  using Type =
      typename CharArrayPack<str, iter - 1, str[iter - 1], chars...>::Type;
};

template <auto const& str, char const... chars>
struct CharArrayPack<str, 0, chars...> {
  using Type = CharArrayPack<str, 0, chars...>;

  template <template <char...> typename T>
  using Apply = T<chars...>;
};

// CharZeroSequence produces integer sequence of all char type zeros.
template <std::size_t... Is>
constexpr auto CharZeroHelper(std::index_sequence<Is...> const&)
    -> decltype(std::integer_sequence<char,
                                      (static_cast<void>(Is), '\0')...>{});

template <typename T>
using CharZeroSequence = decltype(CharZeroHelper(std::declval<T>()));

using NullPtrZeros = CharZeroSequence<std::make_index_sequence<sizeof(void*)>>;

/// Specialized for unique sequence of chars. It produces a payload byte array
/// which stores the source chars an a null-initialized pointer to the relative
/// RefCountedStringImpl.
template <char... chars>
struct BASE_HIDE Entry {
 private:
  // Char count including ending 0
  static constexpr auto kCharCount = sizeof...(chars);

  template <typename T>
  struct Payload;

  template <char... nullptr_zeros>
  struct Payload<std::integer_sequence<char, nullptr_zeros...>> {
    RefCountedStringImpl* impl;
    const char string[kCharCount];

    inline static StaticStringPayload* Data() {
      // It is declared as writable data in final binary section.
      alignas(void*) static char kBuffer[sizeof(Payload)] = {nullptr_zeros...,
                                                             chars...};
      return reinterpret_cast<StaticStringPayload*>(kBuffer);
    }
  };

 public:
  // Only with "always_inline" could this be actually inlined and get binary
  // size benefits.
  static inline __attribute__((always_inline)) StaticString GetString() {
    // At runtime, string impl is initialized once and stored to payload.
    // There should be no need to worry about multi-threading issues.
    // Even if different threads enter GetString() method at the same time
    // for the same Entry, it is OK to create multiple RefCountedStringImpl
    // instances and store any one to payload->impl.
    return StaticString(Payload<NullPtrZeros>::Data());
  }
};

/// This struct works as key of hash map for static strings to eliminate
/// unnecessary type conversion or string data copy.
/// For example:
///   std::unordered_map<base::String, V> table1;
///   std::unordered_map<std::string, V> table2;
///   std::unordered_map<GenericCacheKey, V> table3;
/// Searching in table1 by `std::string` or `const char*` will implicitly
/// construct base::String.
/// Searching in table2 by `const char*` will implicitly construct
/// std::string.
/// Searching in table3, you just need to convert 'base::String',
/// 'std::string' or `const char*` to GenericCacheKey which is light-weight.
struct GenericCacheKey {
  std::string_view content;
  std::size_t hash;

  // Standard guarantees that
  // std::hash<std::string>()(s) == std::hash<std::string_view>()(s)
  GenericCacheKey() = default;
  GenericCacheKey(const base::String& s)
      : content(s.string_view()), hash(s.hash()) {}
  GenericCacheKey(const char* s)
      : content(s), hash(std::hash<std::string_view>()(content)) {}
  GenericCacheKey(const char* s, std::size_t len)
      : content(s, len), hash(std::hash<std::string_view>()(content)) {}
  GenericCacheKey(const std::string& s)
      : content(s), hash(std::hash<std::string_view>()(content)) {}

  bool operator==(const GenericCacheKey& other) const {
    return content == other.content;
  }
};

/// GenericCache is constructed from a C string pointer and caches its base
/// string impl internally.
class GenericCache {
 public:
  GenericCache(const char* s) : s_(s) {}
  GenericCache(const GenericCache&) = delete;
  GenericCache& operator=(const GenericCache&) = delete;
  GenericCache(GenericCache&&) = delete;
  GenericCache& operator=(GenericCache&&) = delete;

  const char* c_str() const { return s_; }

  const std::string& str() const { return ref_impl()->str(); }

  operator StaticString() const {
    // Implicitly convertable to base::StaticString.
    // And forwarding tagged impl pointer directly to StaticString constructor.
    return StaticString(tagged_ref_impl());
  }

  RefCountedStringImpl* ref_impl() const {
    return String::UntagImpl(tagged_ref_impl());
  }

  // Create RefCountedStringImpl if null and stores tagged pointer directly.
  RefCountedStringImpl* tagged_ref_impl() const {
    if (ref_impl_ == nullptr) {
      ref_impl_ =
          String::MakeTaggedImpl(RefCountedStringImpl::RawCreateStatic(s_));
    }
    return ref_impl_;
  }

 protected:
  // Must be a static constant C string and the first member of class.
  const char* s_;

  // Lazily created
  mutable RefCountedStringImpl* ref_impl_{nullptr};
};

/// @brief Use this to declare a base string variable `v` with C string
/// `s`. `s` should be a string literal.
/// The defined variable is of type base::static_string::StaticString.
///
/// Example:
///   BASE_STATIC_STRING_DECL(kTag, "tag")
#define BASE_STATIC_STRING_DECL(v, s)                                       \
  static constexpr lynx::base::static_string::CharArray<std::size(s)>       \
      v##_cstr_storage{s};                                                  \
  using v##StaticStringEntryType =                                          \
      typename lynx::base::static_string::CharArrayPack<v##_cstr_storage>:: \
          Type::template Apply<lynx::base::static_string::Entry>;           \
  auto v = v##StaticStringEntryType::GetString()

/// @brief Convert a char array to base::String and require `s` to be
/// defined as `char[]` not `char*`.
///
/// Example:
///   static constexpr const char kPosition[] = "position";
///   static constexpr const char* kPosition = "position"; // Won't compile
///   auto kPos = BASE_STATIC_STRING(kPosition);
///   auto kPos2 = BASE_STATIC_STRING("position"); // Won't compile
#define BASE_STATIC_STRING(s)                                        \
  lynx::base::static_string::CharArrayPack<s>::Type::template Apply< \
      lynx::base::static_string::Entry>::GetString()

};  // namespace static_string

// used for encode
class StringTable {
 public:
  size_t NewString(const char* str) {
    if (!str) {
      str = "";
    }
    auto iter = string_map_.find(str);
    if (iter != string_map_.end()) {
      return iter->second;
    }

    std::string std_str(str);
    string_list.push_back(std_str);
    size_t index = string_list.size() - 1;
    string_map_.insert(std::make_pair(std_str, index));
    return index;
  }

 public:
  std::unordered_map<std::string, size_t> string_map_;
  std::vector<base::String> string_list;
};

class StringConvertHelper {
 public:
  static constexpr int kMaxInt = 0x7FFFFFFF;
  static constexpr int kMinInt = -kMaxInt - 1;
  static constexpr long long kMaxInt64 = 0x7fffffffffffffff;
  static constexpr long long kMinInt64 = -kMaxInt64 - 1;

  static bool IsMinusZero(double value);

  static inline double FastI2D(int x) { return static_cast<double>(x); }

  static inline int FastD2I(double x) { return static_cast<int32_t>(x); }

  static inline double FastI642D(int64_t x) { return static_cast<double>(x); }

  static inline int64_t FastD2I64(double x) { return static_cast<int64_t>(x); }

  static bool IsInt32Double(double value) {
    return value >= kMinInt && value <= kMaxInt && !IsMinusZero(value) &&
           value == FastI2D(FastD2I(value));
  }

  static bool IsInt64Double(double value) {
    return FastD2I64(value) >= kMinInt64 && FastD2I64(value) <= kMaxInt64 &&
           !IsMinusZero(value) && value == FastI642D(FastD2I64(value));
  }

  static const char* IntToCString(int n, char buffer[], size_t buffer_size) {
    bool negative = true;
    if (n >= 0) {
      n = -n;
      negative = false;
    }
    // Build the string backwards from the least significant digit.
    size_t i = buffer_size;
    buffer[--i] = '\0';
    do {
      // We ensured n <= 0, so the subtraction does the right addition.
      buffer[--i] = '0' - (n % 10);
      n /= 10;
    } while (n);
    if (negative) buffer[--i] = '-';
    return &buffer[0] + i;
  }

  static const char* NumberToString(double double_value, char buffer[],
                                    size_t buffer_size) {
    if (IsInt32Double(double_value)) {
      return IntToCString(double_value, buffer, buffer_size);
    }
    return nullptr;
  }

  static std::string DoubleToString(double double_value) {
    std::ostringstream double2str;
    double2str << std::setprecision(std::numeric_limits<double>::digits10)
               << double_value;
    return double2str.str();
  }
};

}  // namespace base
}  // namespace lynx

namespace std {
template <>
struct hash<lynx::base::String> {
  std::size_t operator()(const lynx::base::String& k) const { return k.hash(); }
};

template <>
struct hash<lynx::base::static_string::GenericCacheKey> {
  std::size_t operator()(
      const lynx::base::static_string::GenericCacheKey& k) const {
    return k.hash;
  }
};
}  // namespace std

#endif  // BASE_INCLUDE_VALUE_BASE_STRING_H_
