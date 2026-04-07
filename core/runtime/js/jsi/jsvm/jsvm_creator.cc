// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/jsvm/jsvm_creator.h"

#include "core/runtime/js/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace runtime {
namespace js {

void InitializeJSVM(const JSVM_InitOptions* options) {
  JSVM_CALL_NO_ENV(OH_JSVM_Init, options);
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
