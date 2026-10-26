// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/js/bindings/modules/module_invoke_scope.h"

#include "core/runtime/js/bindings/modules/module_interceptor.h"

namespace lynx {
namespace runtime {
namespace js {

InvokeMethodEndScope::~InvokeMethodEndScope() {
  if (interceptor) {
    interceptor->OnInvokeMethodEnd(context, delegate, callbacks,
                                   args_converted);
  }
}

}  // namespace js
}  // namespace runtime
}  // namespace lynx
