// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_INSPECTOR_CONST_H_
#define DEVTOOL_JS_INSPECT_INSPECTOR_CONST_H_

namespace lynx {
namespace devtool {

constexpr int kDefaultViewID = -1;
constexpr int kErrorViewID = -2;
constexpr int kSingleGroupID = -1;
constexpr char kSingleGroupStr[] = "-1";
constexpr char kErrorGroupStr[] = "-2";
constexpr char kSingleGroupPrefix[] = "single_group_";

constexpr char kStopAtEntryReason[] = "stopAtEntry";

// parameters defined in CDP
constexpr char kKeyId[] = "id";
constexpr char kKeyMethod[] = "method";
constexpr char kKeyParams[] = "params";
constexpr char kKeySessionId[] = "sessionId";
constexpr char kKeyResult[] = "result";
constexpr char kKeyUrl[] = "url";
constexpr char kKeyUrlRegex[] = "urlRegex";
constexpr char kKeyLineNumber[] = "lineNumber";
constexpr char kKeyColumnNumber[] = "columnNumber";
constexpr char kKeyCondition[] = "condition";
constexpr char kKeyActive[] = "active";
constexpr char kKeyBreakpointId[] = "breakpointId";
constexpr char kKeyDebuggerId[] = "debuggerId";
constexpr char kKeyError[] = "error";
constexpr char kKeyTargetId[] = "targetId";
constexpr char kKeyType[] = "type";
constexpr char kKeyTypeWorker[] = "worker";
constexpr char kKeyTitle[] = "title";
constexpr char kKeyAttached[] = "attached";
constexpr char kKeyCanAccessOpener[] = "canAccessOpener";
constexpr char kKeyTargetInfo[] = "targetInfo";
constexpr char kKeyWaitingForDebugger[] = "waitingForDebugger";

// parameters extended by ourselves
constexpr char kKeyEngineType[] = "engineType";
constexpr char kKeyEngineV8[] = "V8";
constexpr char kKeyEngineQuickjs[] = "PrimJS";
constexpr char kKeyEngineLepus[] = "Lepus";

// Methods
constexpr char kMethodDebuggerEnable[] = "Debugger.enable";
constexpr char kMethodDebuggerDisable[] = "Debugger.disable";
constexpr char kMethodDebuggerSetBreakpointByUrl[] =
    "Debugger.setBreakpointByUrl";
constexpr char kMethodDebuggerSetBreakpointsActive[] =
    "Debugger.setBreakpointsActive";
constexpr char kMethodDebuggerRemoveBreakpoint[] = "Debugger.removeBreakpoint";
constexpr char kMethodRuntimeEnable[] = "Runtime.enable";
constexpr char kMethodProfilerEnable[] = "Profiler.enable";

// Events
constexpr char kEventTargetCreated[] = "Target.targetCreated";
constexpr char kEventAttachedToTarget[] = "Target.attachedToTarget";
constexpr char kEventTargetDestroyed[] = "Target.targetDestroyed";
constexpr char kEventDetachedFromTarget[] = "Target.detachedFromTarget";

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_INSPECTOR_CONST_H_
