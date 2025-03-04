// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_BINDINGS_LEPUS_RENDERER_FUNCTIONS_H_
#define CORE_RUNTIME_BINDINGS_LEPUS_RENDERER_FUNCTIONS_H_

#include <memory>
#include <string>

#include "core/runtime/bindings/lepus/renderer_functions_def.h"
#include "core/runtime/vm/lepus/lepus_value.h"

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

#endif  // CORE_RUNTIME_BINDINGS_LEPUS_RENDERER_FUNCTIONS_H_
