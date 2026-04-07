// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JSI_JSVM_JSVM_CREATOR_H_
#define CORE_RUNTIME_JS_JSI_JSVM_JSVM_CREATOR_H_

#include <ark_runtime/jsvm.h>
#include <ark_runtime/jsvm_types.h>

namespace lynx {
namespace runtime {
namespace js {

void InitializeJSVM(const JSVM_InitOptions* options);

}  // namespace js
}  // namespace runtime
}  // namespace lynx

#endif  // CORE_RUNTIME_JS_JSI_JSVM_JSVM_CREATOR_H_
