// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSC_JSC_RUNTIME_WRAPPER_H_
#define CORE_RUNTIME_JSI_JSC_JSC_RUNTIME_WRAPPER_H_

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {
class JSCRuntimeWrapper : public VMInstance {
 public:
  JSRuntimeType GetRuntimeType() { return piper::JSRuntimeType::jsc; }
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_RUNTIME_WRAPPER_H_
