// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/jsi/jsc/jsc_context_group_wrapper_impl.h"

#include "base/include/log/logging.h"
#include "core/renderer/tasm/config.h"

namespace lynx {
namespace piper {

JSCContextGroupWrapperImpl::JSCContextGroupWrapperImpl()
    : JSCContextGroupWrapper() {}

JSCContextGroupWrapperImpl::~JSCContextGroupWrapperImpl() {
  LOGI("~JSCContextGroupWrapperImpl " << this);
  if (group_ != nullptr) {
    LOGI("JSContextGroupRelease");
    JSContextGroupRelease(group_);
  }
}

void JSCContextGroupWrapperImpl::InitContextGroup() {
  LOGI("JSContextGroupCreate");
  group_ = JSContextGroupCreate();
}

}  // namespace piper
}  // namespace lynx
