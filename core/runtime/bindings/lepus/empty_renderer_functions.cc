// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/bindings/lepus/renderer_functions.h"

namespace lynx {
namespace tasm {

#define RENDERER_FUNCTION_CC(name)                          \
  lepus::Value RendererFunctions::name(lepus::Context* ctx, \
                                       lepus::Value* argv, int argc)

#define RETURN_UNDEFINED() return lepus::Value();

#define NORMAL_FUNCTION_DEF(name) \
  RENDERER_FUNCTION_CC(name) { RETURN_UNDEFINED(); }
NORMAL_RENDERER_FUNCTIONS(NORMAL_FUNCTION_DEF)

#undef NORMAL_FUNCTION_DEF

}  // namespace tasm
}  // namespace lynx
