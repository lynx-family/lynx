// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#import "LynxService.h"
#import "LynxServiceI18nProtocol.h"

#include "core/renderer/tasm/i18n/i18n_binder_darwin.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace tasm {
void I18nBinderDarwin::Bind(intptr_t ptr) {
  [LynxService(LynxServiceI18nProtocol) registerNapiEnv:reinterpret_cast<napi_env>(ptr)];
}
}  // namespace tasm
}  // namespace lynx
