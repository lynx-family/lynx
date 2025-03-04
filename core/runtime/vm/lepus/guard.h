// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_GUARD_H_
#define CORE_RUNTIME_VM_LEPUS_GUARD_H_

namespace lynx {
namespace lepus {
template <class Class>
class Guard {
 public:
  typedef void (Class::*CallbackFunc)();
  Guard(Class* ptr, CallbackFunc enter, CallbackFunc leave)
      : ptr_(ptr), enter_(enter), leave_(leave) {
    if (ptr_ && enter_) (ptr_->*enter_)();
  }

  ~Guard() {
    if (ptr_ && enter_) (ptr_->*leave_)();
  }

 private:
  Class* ptr_;
  CallbackFunc enter_;
  CallbackFunc leave_;
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_GUARD_H_
