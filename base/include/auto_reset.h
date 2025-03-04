// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef BASE_INCLUDE_AUTO_RESET_H_
#define BASE_INCLUDE_AUTO_RESET_H_

#include <utility>

// base::AutoReset<> is useful for setting a variable to a new value only within
// a particular scope. An base::AutoReset<> object resets a variable to its
// original value upon destruction, making it an alternative to writing
// "var = false;" or "var = old_val;" at all of a block's exit points.
//
// This should be obvious, but note that an base::AutoReset<> instance should
// have a shorter lifetime than its scoped_variable, to prevent invalid memory
// writes when the base::AutoReset<> object is destroyed.

namespace lynx {
namespace base {

template <typename T>
class AutoReset {
 public:
  template <typename U>
  AutoReset(T* scoped_variable, U&& new_value)
      : scoped_variable_(scoped_variable),
        original_value_(
            std::exchange(*scoped_variable_, std::forward<U>(new_value))) {}

  AutoReset(AutoReset&& other)
      : scoped_variable_(std::exchange(other.scoped_variable_, nullptr)),
        original_value_(std::move(other.original_value_)) {}

  AutoReset& operator=(AutoReset&& rhs) {
    scoped_variable_ = std::exchange(rhs.scoped_variable_, nullptr);
    original_value_ = std::move(rhs.original_value_);
    return *this;
  }

  ~AutoReset() {
    if (scoped_variable_) *scoped_variable_ = std::move(original_value_);
  }

 private:
  // `scoped_variable_` is not a raw_ptr<T> for performance reasons: Large
  // number of non-PartitionAlloc pointees + AutoReset is typically short-lived
  // (e.g. allocated on the stack).
  T* scoped_variable_;

  T original_value_;
};
}  // namespace base
}  // namespace lynx

#endif  // BASE_INCLUDE_AUTO_RESET_H_
