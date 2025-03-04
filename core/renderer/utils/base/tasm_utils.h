// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RENDERER_UTILS_BASE_TASM_UTILS_H_
#define CORE_RENDERER_UTILS_BASE_TASM_UTILS_H_

#include "core/runtime/vm/lepus/lepus_value.h"

namespace lynx {
namespace tasm {

lepus::Value GenerateSystemInfo(const lepus::Value* config);

}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RENDERER_UTILS_BASE_TASM_UTILS_H_
