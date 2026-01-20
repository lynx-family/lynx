// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_LEPUS_BINDINGS_RENDERER_FUNCTIONS_H_
#define CORE_RUNTIME_LEPUS_BINDINGS_RENDERER_FUNCTIONS_H_

#include <memory>
#include <string>

#include "base/include/value/base_value.h"
#include "core/runtime/lepus/bindings/renderer_functions_def.h"

namespace lynx {
namespace tasm {
#define NORMAL_FUNCTION_DEF(name) \
  static lepus::Value name(lepus::Context* ctx, lepus::Value* argv, int argc);

class RendererFunctions {
 public:
  NORMAL_RENDERER_FUNCTIONS(NORMAL_FUNCTION_DEF)
};

#undef NORMAL_FUNCTION_DEF
}  // namespace tasm
}  // namespace lynx

#endif  // CORE_RUNTIME_LEPUS_BINDINGS_RENDERER_FUNCTIONS_H_
