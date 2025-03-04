// Copyright 2013 The Flutter Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_FML_UNIQUE_OBJECT_H_
#define BASE_INCLUDE_FML_UNIQUE_OBJECT_H_

#include <utility>

#include "base/include/compiler_specific.h"
#include "base/include/fml/macros.h"

namespace lynx {
namespace fml {

// struct UniqueFooTraits {
//   // This function should be fast and inline.
//   static int InvalidValue() { return 0; }
//
//   // This function should be fast and inline.
//   static bool IsValid(const T& value) { return value != InvalidValue(); }
//
//   // This free function will not be called if f == InvalidValue()!
//   static void Free(int f) { ::FreeFoo(f); }
// };

template <typename T, typename Traits>
class UniqueObject {
 private:
  // This must be first since it's used inline below.
  //
  // Use the empty base class optimization to allow us to have a Traits
  // member, while avoiding any space overhead for it when Traits is an
  // empty class.  See e.g. http://www.cantrip.org/emptyopt.html for a good
  // discussion of this technique.
  struct Data : public Traits {
    explicit Data(const T& in) : value_(in) {}
    Data(const T& in, const Traits& other) : Traits(other), value_(in) {}

    T value_;
  };

 public:
  using element_type = T;
  using traits_type = Traits;

  UniqueObject() : data_(Traits::InvalidValue()) {}
  explicit UniqueObject(const T& value) : data_(value) {}

  UniqueObject(const T& value, const Traits& traits) : data_(value, traits) {}

  UniqueObject(UniqueObject&& other)
      : data_(other.release(), other.get_traits()) {}

  ~UniqueObject() { FreeIfNecessary(); }

  UniqueObject& operator=(UniqueObject&& other) {
    reset(other.release());
    return *this;
  }

  void reset(const T& value = Traits::InvalidValue()) {
    LYNX_BASE_CHECK(data_.value_ == Traits::InvalidValue() ||
                    data_.value_ != value);
    FreeIfNecessary();
    data_.value_ = value;
  }

  void swap(UniqueObject& other) {
    // Standard swap idiom: 'using std::swap' ensures that std::swap is
    // present in the overload set, but we call swap unqualified so that
    // any more-specific overloads can be used, if available.
    using std::swap;
    swap(static_cast<Traits&>(data_), static_cast<Traits&>(other.data_));
    swap(data_.value_, other.data_.value_);
  }

  // Release the object. The return value is the current object held by this
  // object. After this operation, this object will hold an invalid value, and
  // will not own the object any more.
  [[nodiscard]] T release() {
    T old_value = data_.value_;
    data_.value_ = Traits::InvalidValue();
    return old_value;
  }

  const T& get() const { return data_.value_; }

  bool is_valid() const { return Traits::IsValid(data_.value_); }

  bool operator==(const T& value) const { return data_.value_ == value; }

  bool operator!=(const T& value) const { return data_.value_ != value; }

  Traits& get_traits() { return data_; }
  const Traits& get_traits() const { return data_; }

 private:
  void FreeIfNecessary() {
    if (data_.value_ != Traits::InvalidValue()) {
      data_.Free(data_.value_);
      data_.value_ = Traits::InvalidValue();
    }
  }

  // Forbid comparison. If U != T, it totally doesn't make sense, and if U ==
  // T, it still doesn't make sense because you should never have the same
  // object owned by two different UniqueObject.
  template <typename T2, typename Traits2>
  bool operator==(const UniqueObject<T2, Traits2>& p2) const = delete;

  template <typename T2, typename Traits2>
  bool operator!=(const UniqueObject<T2, Traits2>& p2) const = delete;

  Data data_;

  BASE_DISALLOW_COPY_AND_ASSIGN(UniqueObject);
};

template <class T, class Traits>
void swap(const UniqueObject<T, Traits>& a, const UniqueObject<T, Traits>& b) {
  a.swap(b);
}

template <class T, class Traits>
bool operator==(const T& value, const UniqueObject<T, Traits>& object) {
  return value == object.get();
}

template <class T, class Traits>
bool operator!=(const T& value, const UniqueObject<T, Traits>& object) {
  return !(value == object.get());
}

}  // namespace fml
}  // namespace lynx

namespace fml {
using lynx::fml::UniqueObject;
}

#endif  // BASE_INCLUDE_FML_UNIQUE_OBJECT_H_
