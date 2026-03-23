// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_REVERSEITERATION_H_
#define CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_REVERSEITERATION_H_

#include "core/runtime/lepus/ir/llvh/include/llvh/Support/PointerLikeTypeTraits.h"

namespace llvh {

template <class T = void*>
bool shouldReverseIterate() {
#if LLVM_ENABLE_REVERSE_ITERATION
  return detail::IsPointerLike<T>::value;
#else
  return false;
#endif
}

}  // namespace llvh
#endif  // CORE_RUNTIME_LEPUS_IR_LLVH_INCLUDE_LLVH_SUPPORT_REVERSEITERATION_H_
