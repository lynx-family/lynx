// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_VM_LEPUS_UPVALUE_H_
#define CORE_RUNTIME_VM_LEPUS_UPVALUE_H_

#include <utility>

#include "base/include/value/base_string.h"

namespace lynx {
namespace lepus {
struct UpvalueInfo {
  base::String name_;
  long register_;
  bool in_parent_vars_;

  UpvalueInfo(base::String name, long register_id, bool in_parent_vars)
      : name_(std::move(name)),
        register_(register_id),
        in_parent_vars_(in_parent_vars) {}
};
}  // namespace lepus
}  // namespace lynx

#endif  // CORE_RUNTIME_VM_LEPUS_UPVALUE_H_
