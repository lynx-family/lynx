// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import <Lynx/LynxWasmFuncRegistry.h>
#include "core/runtime/js/jsi/jsc/jsc_context_wrapper.h"
#include "core/runtime/js/jsi/quickjs/quickjs_context_wrapper.h"

@implementation LynxWasmFuncRegistry

+ (void)registerWasmFunc:(void*)func {
  lynx::runtime::js::JSCContextWrapper::register_wasm_func_ =
      reinterpret_cast<void (*)(void*, void*)>(func);
}

+ (void)registerQjsWasmFunc:(void*)func {
  if (!func) {
    return;
  }
  lynx::runtime::js::QuickjsContextWrapper::register_wasm_func_ =
      reinterpret_cast<void (*)(void*, void*)>(func);
}

@end
