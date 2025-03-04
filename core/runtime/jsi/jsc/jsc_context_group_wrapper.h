// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_GROUP_WRAPPER_H_
#define CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_GROUP_WRAPPER_H_

#include <JavaScriptCore/JavaScript.h>

#include <unordered_map>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

class JSCContextGroupWrapper : public VMInstance {
 public:
  JSCContextGroupWrapper() = default;
  virtual ~JSCContextGroupWrapper() = default;

  virtual void InitContextGroup() = 0;
  JSRuntimeType GetRuntimeType() { return piper::JSRuntimeType::jsc; }

 private:
  friend class JSCRuntime;
  friend class JSCContextWrapper;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_GROUP_WRAPPER_H_
