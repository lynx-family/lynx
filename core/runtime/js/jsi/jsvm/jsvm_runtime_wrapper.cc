// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/jsi/jsvm/jsvm_runtime_wrapper.h"

#include <cstring>
#include <mutex>

#include "core/runtime/js/jsi/jsvm/jsvm_util.h"

namespace lynx {
namespace runtime {
namespace js {
JSVMRuntimeInstance::~JSVMRuntimeInstance() {
  JSVM_CALL_NO_ENV(OH_JSVM_CloseVMScope, vm_, vm_scope_);
  JSVM_CALL_NO_ENV(OH_JSVM_DestroyVM, vm_);
}

void JSVMRuntimeInstance::InitInstance() {
  static std::once_flag flag;
  std::call_once(flag, [this]() {
    LOGI("lynx JSVMRuntimeInstance::InitInstance");
    enum class JSVMInitMode {
      kDefault,
      kMemorySensitive,
    };

    JSVMInitMode init_mode = JSVMInitMode::kDefault;
    JSVM_InitOptions initOptions;
    int argc = 0;
    const char* argv[2];

    switch (init_mode) {
      case JSVMInitMode::kMemorySensitive:
        argc = 2;
        argv[0] = "--jsvm";
        argv[1] = "--optimize-for-size";
        initOptions.argc = &argc;
        initOptions.argv = const_cast<char**>(argv);
        break;
      case JSVMInitMode::kDefault:
      default:
        memset(&initOptions, 0, sizeof(initOptions));
        break;
    }

    JSVM_CALL_NO_ENV(OH_JSVM_Init, &initOptions);

    JSVM_CreateVMOptions options;
    memset(&options, 0, sizeof(options));
    JSVM_CALL_NO_ENV(OH_JSVM_CreateVM, &options, &vm_);

    JSVM_CALL_NO_ENV(OH_JSVM_OpenVMScope, vm_, &vm_scope_);
  });
}
}  // namespace js
}  // namespace runtime
}  // namespace lynx
