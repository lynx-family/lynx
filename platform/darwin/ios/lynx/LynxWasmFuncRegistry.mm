// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxWasmFuncRegistry.h"
#include "core/runtime/jsi/jsc/jsc_context_wrapper_impl.h"

@implementation LynxWasmFuncRegistry

+ (void)registerWasmFunc:(void*)func {
  lynx::piper::JSCContextWrapperImpl::register_wasm_func_ =
      reinterpret_cast<void (*)(void*, void*)>(func);
}

@end
