// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTED_CONTEXT_H_
#define DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTED_CONTEXT_H_

#include <memory>
#include <string>

#ifdef __cplusplus
extern "C" {
#endif
#include "quickjs/include/quickjs.h"
#ifdef __cplusplus
}
#endif

#include "devtool/js_inspect/quickjs/quickjs_internal/quickjs_debugger_ng.h"

namespace quickjs_inspector {

class QJSInspectorImpl;

// Register callbacks to the Quickjs and initialize the inspector environment.
class QJSInspectedContext {
 public:
  QJSInspectedContext(QJSInspectorImpl* inspector, LEPUSContext* ctx,
                      const std::string& name);
  ~QJSInspectedContext();

  static QJSInspectedContext* GetFromJsContext(LEPUSContext* ctx);
  LEPUSContext* GetContext() { return ctx_; }
  QJSInspectorImpl* GetInspector() { return inspector_; }
  const std::unique_ptr<QuickjsDebugger>& GetDebugger() { return debugger_; }

 private:
  void PrepareQJSDebugger();

  QJSInspectorImpl* inspector_;
  LEPUSContext* ctx_;
  std::unique_ptr<QuickjsDebugger> debugger_;
};

}  // namespace quickjs_inspector

#endif  // DEVTOOL_JS_INSPECT_QUICKJS_QUICKJS_INTERNAL_QUICKJS_INSPECTED_CONTEXT_H_
