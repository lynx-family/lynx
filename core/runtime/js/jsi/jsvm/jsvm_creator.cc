// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/js/jsi/jsvm/jsvm_creator.h"

#include "core/runtime/js/jsi/jsvm/jsvm_util.h"
#include "platform/harmony/lynx_jsvm_initializer/src/main/cpp/jsvm_initializer.h"

namespace lynx {
namespace runtime {
namespace js {

void InitializeJSVM(const JSVM_InitOptions* options) {
  auto status = Lynx_JSVM_Common_Init(options);
  if (status != JSVM_Status::JSVM_OK) {
    LOGE("jsvm init failed status:" << status);
  }
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
