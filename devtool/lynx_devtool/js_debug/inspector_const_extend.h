// Copyright 2022 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_INSPECTOR_CONST_EXTEND_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_INSPECTOR_CONST_EXTEND_H_

#include "devtool/js_inspect/inspector_const.h"

namespace lynx {
namespace devtool {

constexpr int kDefaultGlobalRuntimeID = -1;
constexpr char kLepusDefaultContextName[] = "__Card__";

constexpr char kScriptUrlPrefix[] = "file://view";

// parameters defined in CDP
constexpr char kKeyScriptId[] = "scriptId";
constexpr char kKeyExecutionContextId[] = "executionContextId";
constexpr char kKeyArgs[] = "args";
constexpr char kKeyValue[] = "value";
constexpr char kKeyStringType[] = "string";
constexpr char kKeyFunctionDeclaration[] = "functionDeclaration";
constexpr char kKeyObjectId[] = "objectId";

// parameters extended by ourselves
constexpr char kKeyViewId[] = "viewId";
constexpr char kKeyConsoleId[] = "consoleId";
constexpr char kKeyRuntimeId[] = "runtimeId";
constexpr char kKeyGroupId[] = "groupId";
constexpr char kKeyLepusRuntimeId[] = "lepusRuntimeId";
constexpr char kKeyConsoleTag[] = "consoleTag";
constexpr char kTargetJSPrefix[] = "Background:";
constexpr char kTargetLepus[] = "Main";
constexpr char kTargetLepusPrefix[] = "Main:";

// Methods
constexpr char kMethodDebuggerResume[] = "Debugger.resume";
constexpr char kMethodDebuggerSetSkipAllPauses[] = "Debugger.setSkipAllPauses";
constexpr char kMethodRuntimeCallFunctionOn[] = "Runtime.callFunctionOn";

// Events
constexpr char kEventDebuggerScriptParsed[] = "Debugger.scriptParsed";
constexpr char kEventDebuggerRemoveScriptsForLynxView[] =
    "Debugger.removeScriptsForLynxView";
constexpr char kEventRuntimeConsoleAPICalled[] = "Runtime.consoleAPICalled";
constexpr char kEventRuntimeExecutionContextDestroyed[] =
    "Runtime.executionContextDestroyed";
constexpr char kEventRuntimeExecutionContextsCleared[] =
    "Runtime.executionContextsCleared";
constexpr char kEventProfilerEnabled[] = "Profiler.enabled";

constexpr char kStringifyObjectScript[] =
    "function stringify() {return JSON.stringify(this,null,'\t');}";

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_INSPECTOR_CONST_EXTEND_H_
