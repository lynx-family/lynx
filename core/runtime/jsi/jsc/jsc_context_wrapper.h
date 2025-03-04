// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_WRAPPER_H_
#define CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_WRAPPER_H_

#include <JavaScriptCore/JavaScript.h>

#include <atomic>
#include <memory>
#include <unordered_map>

#include "core/runtime/jsi/jsi.h"

namespace lynx {
namespace piper {

class JSCContextWrapper : public JSIContext {
 public:
  JSCContextWrapper(std::shared_ptr<VMInstance> vm) : JSIContext(vm){};
  virtual ~JSCContextWrapper() = default;
  virtual void init() = 0;

  virtual const std::atomic<bool>& contextInvalid() const = 0;
  virtual std::atomic<intptr_t>& objectCounter() const = 0;

  virtual JSGlobalContextRef getContext() const = 0;
};

}  // namespace piper
}  // namespace lynx
#endif  // CORE_RUNTIME_JSI_JSC_JSC_CONTEXT_WRAPPER_H_
