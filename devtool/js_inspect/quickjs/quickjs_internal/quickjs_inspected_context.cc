// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspected_context.h"

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_debugger_ng.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspected_context_callbacks.h"
#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_inspector_impl.h"

namespace quickjs_inspector {

QJSInspectedContext::QJSInspectedContext(QJSInspectorImpl* inspector,
                                         LEPUSContext* ctx,
                                         const std::string& name)
    : inspector_(inspector), ctx_(ctx) {
  LEPUS_SetContextOpaque(ctx_, this);
  PrepareQJSDebugger();
  // debugger_ must be created after calling PrepareQJSDebugger().
  debugger_ = std::make_unique<QuickjsDebugger>(this, name);
}

QJSInspectedContext::~QJSInspectedContext() {
  LEPUS_SetContextOpaque(ctx_, nullptr);
}

QJSInspectedContext* QJSInspectedContext::GetFromJsContext(LEPUSContext* ctx) {
  if (ctx == nullptr || !LEPUS_GetContextOpaque(ctx)) {
    return nullptr;
  }
  return reinterpret_cast<QJSInspectedContext*>(LEPUS_GetContextOpaque(ctx));
}

void QJSInspectedContext::PrepareQJSDebugger() {
  auto& funcs = GetQJSCallbackFuncs();
  // register debugger related function callbacks
  PrepareQJSDebuggerForSharedContext(ctx_, funcs.data(), funcs.size(),
                                     inspector_->IsFullFuncEnabled());
}

}  // namespace quickjs_inspector
