// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#include "core/runtime/vm/lepus/quick_context.h"

#include <assert.h>

#include <utility>

#include "base/include/string/string_number_convert.h"
#include "base/include/value/array.h"
#include "base/include/value/base_value.h"
#include "base/include/value/path_parser.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/build/gen/lynx_sub_error_code.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/renderer/utils/value_utils.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/profile/runtime_profiler_manager.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/runtime/vm/lepus/exception.h"
#include "core/runtime/vm/lepus/jsvalue_helper.h"
#include "core/runtime/vm/lepus/lepus_error_helper.h"
#ifdef OS_IOS
#include "gc/trace-gc.h"
#else
#include "quickjs/include/trace-gc.h"
#endif

namespace lynx {
namespace lepus {
inline constexpr char kRawRuntimeMemoryInfo[] = "raw_memory_info_json_str";

#define RENDERER_FUNCTION(name)                                       \
  static LEPUSValue name(LEPUSContext* ctx, LEPUSValueConst this_val, \
                         int argc, LEPUSValueConst* argv)

static std::string GetPrintStr(LEPUSContext* ctx, int32_t argc,
                               LEPUSValueConst* argv) {
  std::ostringstream ss;
  for (int32_t i = 0; i < argc; i++) {
    MK_JS_LEPUS_VALUE(ctx, argv[i]).PrintValue(ss);
    if (i < argc - 1) {
      ss << " ";
    }
  }
  return ss.str();
}

RENDERER_FUNCTION(Console_Log) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("log", "[main-thread.js] " + result);

#if defined(LEPUS_PC)
  LOGI("[main-thread.js] " + result);
#endif
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Profile) {
  if (argc == 0) {
    return LEPUS_UNDEFINED;
  }
  std::ostringstream ss;
  ss << "Lepus::";
  MK_JS_LEPUS_VALUE(ctx, argv[0]).PrintValue(ss);

  TRACE_EVENT_BEGIN(LYNX_TRACE_CATEGORY, nullptr,
                    [&](lynx::perfetto::EventContext ctx) {
                      ctx.event()->set_name(ss.str());
                    });

  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_ProfileEnd) {
  TRACE_EVENT_END(LYNX_TRACE_CATEGORY);

  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_ALog) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("alog", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Report) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("report", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}
RENDERER_FUNCTION(Console_Error) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("error", "[main-thread.js] " + result);
  const std::string result_msg = "console.error: \n\n" + result;
  lctx->ReportErrorWithMsg(result_msg, error::E_MTS_RUNTIME_ERROR);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Warn) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("warn", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Debug) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("debug", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Info) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("info", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Count) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("count", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_CountReset) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("countReset", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Group) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("group", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_GroupCollapsed) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("groupCollapsed", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_GroupEnd) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("groupEnd", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Time) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("time", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_TimeLog) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("timeLog", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_TimeEnd) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("timeEnd", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

RENDERER_FUNCTION(Console_Table) {
  lepus::Context* lctx = lepus::QuickContext::GetFromJsContext(ctx);
  std::string result = GetPrintStr(ctx, argc, argv);
  lctx->OnBTSConsoleEvent("table", "[main-thread.js] " + result);
  return LEPUS_UNDEFINED;
}

void RegisterQuickCFun(QuickContext* ctx, LEPUSValue& obj, const char* name,
                       int argc, LEPUSCFunction* func) {
  LEPUSValue cf = LEPUS_NewCFunction(ctx->context(), func, name, argc);
  auto holder = MK_JS_LEPUS_VALUE(ctx->context(), cf);
  LEPUSValueHelper::SetProperty(ctx->context(), obj, name, cf);
}

void RegisterConsole(QuickContext* ctx) {
  LEPUSValue obj = LEPUS_NewObject(ctx->context());
  auto holder = MK_JS_LEPUS_VALUE(ctx->context(), obj);
  RegisterQuickCFun(ctx, obj, "profile", 1, Console_Profile);
  RegisterQuickCFun(ctx, obj, "profileEnd", 0, Console_ProfileEnd);
  RegisterQuickCFun(ctx, obj, "log", 1, Console_Log);
  RegisterQuickCFun(ctx, obj, "alog", 1, Console_ALog);
  RegisterQuickCFun(ctx, obj, "report", 1, Console_Report);
  RegisterQuickCFun(ctx, obj, "info", 1, Console_Info);
  RegisterQuickCFun(ctx, obj, "warn", 1, Console_Warn);
  RegisterQuickCFun(ctx, obj, "error", 1, Console_Error);
  RegisterQuickCFun(ctx, obj, "debug", 1, Console_Debug);
  RegisterQuickCFun(ctx, obj, "count", 0, Console_Count);
  RegisterQuickCFun(ctx, obj, "countReset", 0, Console_CountReset);
  RegisterQuickCFun(ctx, obj, "group", 0, Console_Group);
  RegisterQuickCFun(ctx, obj, "groupCollapsed", 0, Console_GroupCollapsed);
  RegisterQuickCFun(ctx, obj, "groupEnd", 0, Console_GroupEnd);
  RegisterQuickCFun(ctx, obj, "time", 0, Console_Time);
  RegisterQuickCFun(ctx, obj, "timeLog", 0, Console_TimeLog);
  RegisterQuickCFun(ctx, obj, "timeEnd", 0, Console_TimeEnd);
  RegisterQuickCFun(ctx, obj, "table", 1, Console_Table);
  ctx->RegisterGlobalProperty("console", obj);
}

QuickContext::QuickContext(bool disable_tracing_gc, int runtime_mode)
    : LEPUSRuntimeData(disable_tracing_gc, runtime_mode),
      Context(ContextType::LepusNGContextType),
      top_level_function_(LEPUS_UNDEFINED),
      use_lepus_strict_mode_(false),
      debuginfo_outside_(false),
      current_this_(LEPUS_UNDEFINED) {
  // TODO(nihao.royal): maybe add a platform interface to enable the runtime
  // leak checker later;
  if (tasm::LynxEnv::GetInstance().IsDevToolEnabled()) {
    EnableRuntimeLeakCheck(true);
  }
  LEPUSLepusRefCallbacks callbacks = Context::GetLepusRefCall();
  RegisterLepusRefCallbacks(runtime_, &callbacks);
  LEPUS_SetMaxStackSize(context(), static_cast<size_t>(ULLONG_MAX));
  LEPUS_SetContextOpaque(lepus_context_, Context::RegisterContextCell(this));
  Initialize();
  RegisterLepusType(runtime_, Value_Array, Value_Table);
  // data associated with debugger need to be initialized
  gc_flag_ = LEPUS_IsGCModeRT(runtime_);
  LEPUS_SetGCObserver(runtime_, static_cast<GCObserver*>(this));
}

QuickContext::~QuickContext() {
  if (runtime_) {
    LEPUS_SetGCObserver(runtime_, nullptr);
  }
  if (!LEPUS_IsUndefined(top_level_function_)) {
    if (gc_flag_) {
      p_val_.Reset(context());
    } else {
      LEPUS_FreeValue(context(), top_level_function_);
    }
  }
  DestroyInspector();
}

void QuickContext::OnGC(std::string mem_info) {
  if (!delegate_) {
    return;
  }
  delegate_->OnRuntimeGC({{kRawRuntimeMemoryInfo, std::move(mem_info)}});
}

void QuickContext::ReportErrorWithMsg(const std::string& msg,
                                      const std::string& stack,
                                      int32_t error_code, int32_t error_level) {
  auto formatted_message = FormatExceptionMessage(msg, stack, "");
  ReportErrorWithMsg(formatted_message, error_code, error_level);
}

void QuickContext::SetSourceMapRelease(const lepus::Value& source_map_release) {
  BASE_STATIC_STRING_DECL(kStack, "stack");
  if (!(source_map_release.GetProperty(kStack).IsString())) {
    LOGI(
        "QuickContext::SetSourceMapRelease, can't found Error, stack is "
        "undefined");
    return;
  }
  BASE_STATIC_STRING_DECL(kMessage, "message");
  if (!(source_map_release.GetProperty(kMessage).IsString())) {
    LOGI(
        "QuickContext::SetSourceMapRelease, can't found Error, message is "
        "undefined");
    return;
  }

  common::JSErrorInfo args;
  args.message = source_map_release.GetProperty(kMessage).StdString();
  args.stack = source_map_release.GetProperty(kStack).StdString();
  OnBTSConsoleEvent("info", "SetSourceMapRelease.message:" + args.message);
  OnBTSConsoleEvent("info", "SetSourceMapRelease.stack:" + args.stack);
  js_error_reporter_.SetSourceMapRelease(std::move(args));
}

void QuickContext::ReportErrorWithMsg(const std::string& msg,
                                      int32_t error_code, int32_t error_level) {
  EnsureDelegate();

  // enable outside debug information only when engine version is bigger than
  // "2.7" and "debuginfo_outside_" is true when encode
  if (!delegate_) {
    return;
  }

  const auto& target_sdk_version = delegate_->TargetSdkVersion();
  OnBTSConsoleEvent("info",
                    "ReportErrorWithMsg.engine version:" + target_sdk_version);
  if (tasm::Config::IsHigherOrEqual(target_sdk_version, LYNX_VERSION_2_7) &&
      debuginfo_outside_) {
    auto error = js_error_reporter_.SendMTError(msg, error_code, error_level);
    if (error) {
      delegate_->ReportError(std::move(*error));
    }
  } else {
    ReportError(msg, error_code,
                static_cast<base::LynxErrorLevel>(error_level));
  }
  OnBTSConsoleEvent("error", msg);
}

void QuickContext::BeforeReportError(base::LynxError& error) {
  js_error_reporter_.AppendCustomInfo(error);
}

void QuickContext::AddReporterCustomInfo(
    const std::unordered_map<std::string, std::string>& info) {
  js_error_reporter_.AddCustomInfoToError(info);
}

QuickContext* QuickContext::Cast(Context* context) {
  assert(context->IsLepusNGContext());
  return static_cast<QuickContext*>(context);
}

void QuickContext::Initialize() {
  // If Lepus debug is on, console object will be reset by debugger.
  RegisterConsole(this);
  RegisterLepusVerion();
}

void QuickContext::RegisterMethodToLynx() {
#ifndef LEPUS_PC
  tasm::Utils::RegisterNGMethodToLynx(this, lynx_, GetSdkVersion());
#endif
}

void QuickContext::RegisterLepusVerion() {
  LEPUSContext* ctx = context();
  HandleScope func_scope(ctx);
  LEPUSAtom atom = LEPUS_NewAtom(ctx, "__lepus_version__");
  func_scope.PushLEPUSAtom(atom);
  LEPUSValue global_obj = LEPUS_GetGlobalObject(ctx);
  LEPUSValue str = LEPUS_NewString(ctx, LYNX_LEPUS_VERSION);
  func_scope.PushHandle(&str, HANDLE_TYPE_LEPUS_VALUE);
  LEPUS_DefinePropertyValue(ctx, global_obj, atom, str, LEPUS_PROP_ENUMERABLE);
  if (!LEPUS_IsGCMode(ctx)) {
    LEPUS_FreeAtom(ctx, atom);
    LEPUS_FreeValue(ctx, global_obj);
  }
}

void QuickContext::SetTopLevelFunction(LEPUSValue val) {
  if (!gc_flag_ && !LEPUS_IsUndefined(top_level_function_)) {
    LEPUS_FreeValue(context(), top_level_function_);
  }

  top_level_function_ = val;
  if (gc_flag_) p_val_.Reset(context(), top_level_function_);
  auto debug_delegate = debug_delegate_.lock();
  if (debug_delegate != nullptr) {
    debug_delegate->OnTopLevelFunctionReady();
  }
}

LEPUSValue QuickContext::GetTopLevelFunction() const {
  return top_level_function_;
}

void QuickContext::SetEnableStrictCheck(bool val) {
  use_lepus_strict_mode_ = val;
}

void QuickContext::SetFunctionFileName(LEPUSValue func_obj,
                                       const char* file_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, QUICK_CONTEXT_SET_FUNC_FILE_NAME,
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("file_name", file_name);
              });
  if (file_name != nullptr) {
    LEPUS_SetFuncFileName(context(), func_obj, file_name);
  }
}

void QuickContext::SetStackSize(uint32_t stack_size) {
  stack_size_ = stack_size;
  if (stack_size_ > 0) {
    LEPUS_SetVirtualStackSize(context(), stack_size_);
  }
}

bool QuickContext::Execute() {
  ScriptingScope scope(this);

  return ExecuteBinaryInternal(nullptr);
}

bool QuickContext::ExecuteBinaryInternal(Value* ret_val) {
  if (LEPUS_IsUndefined(top_level_function_)) {
    LOGE("no compiled function object");
    return false;
  }
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, QUICK_CONTEXT_EXECUTE);

  EnsureLynx();

  if (!use_lepus_strict_mode_) {
    LEPUS_SetNoStrictMode(context());
  }
  // dup function object to avoid free
  // it will free when :
  // 1. compile other top level function
  // 2. QuickContext was destruct
  LEPUS_DupValue(context(), top_level_function_);
  LEPUSValue global = LEPUS_GetGlobalObject(context());
  LEPUSValue ret = LEPUS_EvalFunction(context(), top_level_function_, global);
  if (LEPUS_IsException(ret)) {
    constexpr const static char* kErrorPrefix =
        "QuickContext::Execute() exception!!!\n";
    int32_t err_code;
    const std::string log = GetExceptionMessage(kErrorPrefix, &err_code);
    LOGE("Run error:\n" << log);
    ReportErrorWithMsg(log, err_code);
  }
  HandleScope func_scope(context(), &ret, HANDLE_TYPE_LEPUS_VALUE);
  EvalLepusPendingTask();

  if (!gc_flag_) LEPUS_FreeValue(context(), global);
  if (ret_val) {
    *ret_val = MK_JS_LEPUS_VALUE(lepus_context_, ret);
  } else if (!gc_flag_) {
    LEPUS_FreeValue(context(), ret);
  }
  return true;
}

void QuickContext::UpdateGCTiming(bool is_start) {
  if (!gc_flag_) return;
  if (is_start) {
    gc_info_start_ = LEPUS_GetGCTimingInfo(context(), is_start);
  } else {
    char* gc_info_end_ = LEPUS_GetGCTimingInfo(context(), is_start);
    delegate_->ReportGCTimingEvent(gc_info_start_, gc_info_end_);
  }
}

class GCPauseSuppressionMode {
 public:
  GCPauseSuppressionMode(LEPUSRuntime* rt) : runtime_(rt) {
    origin_mode_ = LEPUS_GetGCPauseSuppressionMode(rt);
    LEPUS_SetGCPauseSuppressionMode(rt, true);
  }
  ~GCPauseSuppressionMode() {
    LEPUS_SetGCPauseSuppressionMode(runtime_, origin_mode_);
  }

 private:
  LEPUSRuntime* runtime_;
  bool origin_mode_;
};

Value QuickContext::CallArgs(const base::String& name, const Value* args[],
                             size_t args_count, bool pause_suppression_mode) {
  ScriptingScope scope(this);

  if (pause_suppression_mode) {
    GCPauseSuppressionMode mode(LEPUS_GetRuntime(lepus_context_));
    return CallArgs(name, args, args_count, false);
  }

  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, QUICK_CONTEXT_CALL,
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("name", name.str());
              });
  LEPUSValue quick_args[args_count];
  HandleScope func_scope(lepus_context_);
  func_scope.PushLEPUSValueArrayHandle(quick_args,
                                       static_cast<int>(args_count));
  for (size_t i = 0; i < args_count; ++i) {
    quick_args[i] = LEPUSValueHelper::ToJsValue(lepus_context_, *(args[i]));
  }
  Value ret = MK_JS_LEPUS_VALUE(lepus_context_,
                                GetAndCall(name.str(), quick_args, args_count));
  if (!gc_flag_) {
    for (auto it : quick_args) {
      LEPUS_FreeValue(lepus_context_, it);
    }
  }
  return ret;
}

Value QuickContext::CallClosureArgs(const Value& closure, const Value* args[],
                                    size_t args_count) {
  ScriptingScope scope(this);

  LEPUSValue lepus_closure =
      LEPUSValueHelper::ToJsValue(lepus_context_, closure);
  HandleScope func_scope(lepus_context_, &lepus_closure,
                         HANDLE_TYPE_LEPUS_VALUE);
  DCHECK(LEPUSValueHelper::IsJsFunction(lepus_context_, lepus_closure));
  LEPUSValue quick_args[args_count];
  func_scope.PushLEPUSValueArrayHandle(quick_args,
                                       static_cast<int>(args_count));
  for (size_t i = 0; i < args_count; ++i) {
    quick_args[i] = LEPUSValueHelper::ToJsValue(lepus_context_, *(args[i]));
  }
  Value ret = MK_JS_LEPUS_VALUE(
      lepus_context_, InternalCall(lepus_closure, quick_args, args_count));
  if (!LEPUS_IsGCModeRT(runtime_)) {
    for (auto it : quick_args) {
      LEPUS_FreeValue(context(), it);
    }
    LEPUS_FreeValue(lepus_context_, lepus_closure);
  }
  return ret;
}

long QuickContext::GetParamsSize() {
  assert(false);
  return 0;
}

Value* QuickContext::GetParam(long index) {
  assert(false);
  return nullptr;
}

const std::string& QuickContext::name() const { return name_; }

bool QuickContext::CheckTableShadowUpdatedWithTopLevelVariable(
    const lepus::Value& update) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, QUICK_CONTEXT_CHECK_TABLE_SHADOW_UPDATED);
  bool enable_deep_check = false;
#if ENABLE_INSPECTOR && (ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE)
  if (lynx::tasm::LynxEnv::GetInstance().IsTableDeepCheckEnabled()) {
    enable_deep_check = true;
  }
#endif
  auto update_type = update.Type();
  if (update_type != ValueType::Value_Table) {
    return true;
  }
  // page new data from setData
  auto update_table_value = update.Table();
  // shadow compare new_data_table && top level
  // if any top level data are different, need update;
  for (auto& update_data_iterator : *update_table_value) {
    const auto& key = update_data_iterator.first.str();
    auto val = update_data_iterator.second;
    auto result = ParseValuePath(key);
    if (result.empty()) {
      return true;
    }
    auto front_value = result.begin();
    LEPUSAtom atom = LEPUS_NewAtom(context(), front_value->c_str());
    Value value = MK_JS_LEPUS_VALUE(lepus_context_,
                                    LEPUS_GetGlobalVar(context(), atom, false));
    if (!gc_flag_) LEPUS_FreeAtom(context(), atom);
    result.erase(front_value);
    for (auto it = result.begin(); it != result.end(); ++it) {
      if (value.IsTable()) {
        base::String key(*it);
        if (!value.Table()->Contains(key)) {
          // target table did not have this new key
          return true;
        }
        value = (value.Table()->GetValue(key));
      } else if (value.IsArray()) {
        int index;
        if (lynx::base::StringToInt(*it, &index, 10)) {
          if (static_cast<size_t>(index) >= value.Array()->size()) {
            // the array's size is smaller.
            return true;
          }
          value = value.Array()->get(index);
        }
      }
    }

    if (value.IsUndefined() || value.IsJSUndefined() || value.IsNil()) {
      return true;
    }
    if (!enable_deep_check &&
        tasm::CheckTableValueNotEqual(value.ToLepusValue(), val)) {
      return true;
    }
#if ENABLE_INSPECTOR && (ENABLE_TRACE_PERFETTO || ENABLE_TRACE_SYSTRACE)
    if (enable_deep_check &&
        tasm::CheckTableDeepUpdated(value.ToLepusValue(), val, false)) {
      return true;
    }
#endif
  }
  return false;
}

bool QuickContext::UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                                const lepus::Value& val) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, QUICK_CONTEXT_UPDATE_TOP_LEVEL_VARIABLE);
  if (!path.empty()) {
    // for performance.
    if (path.size() == 1) {
      HandleScope block_scope(context());
      LEPUSAtom atom = LEPUS_NewAtom(context(), path.begin()->c_str());
      block_scope.PushLEPUSAtom(atom);
      LEPUSValue lepus_val = LEPUSValueHelper::ToJsValue(lepus_context_, val);
      block_scope.PushHandle(&lepus_val, HANDLE_TYPE_LEPUS_VALUE);
      int ret = LEPUS_SetGlobalVar(context(), atom, lepus_val, 2);
      if (!gc_flag_) LEPUS_FreeAtom(context(), atom);
      return ret >= 0;
    }
    HandleScope block_scope(context());
    auto front_value = path.begin();
    LEPUSAtom atom = LEPUS_NewAtom(context(), front_value->c_str());
    block_scope.PushLEPUSAtom(atom);
    Value value = MK_JS_LEPUS_VALUE(lepus_context_,
                                    LEPUS_GetGlobalVar(context(), atom, false));
    if (value.IsUndefined() || value.IsJSUndefined() || value.IsNil()) {
      if (!gc_flag_) LEPUS_FreeAtom(context(), atom);
      return false;
    }
    path.erase(front_value);
    lepus::Value::UpdateValueByPath(value, val, path);
    LEPUSValue lepus_val_new =
        LEPUSValueHelper::ToJsValue(lepus_context_, value);
    block_scope.PushHandle(&lepus_val_new, HANDLE_TYPE_LEPUS_VALUE);
    int ret = LEPUS_SetGlobalVar(context(), atom, lepus_val_new, 2);
    if (!gc_flag_) LEPUS_FreeAtom(context(), atom);
    return ret >= 0;
  }
  return false;
}

void QuickContext::ResetTopLevelVariable() {
  // TODO(nihao) lepus NG support reset top var.
}

void QuickContext::ResetTopLevelVariableByVal(const Value& val) {
  // TODO(nihao) lepus NG support reset top var.
}

std::unique_ptr<lepus::Value> QuickContext::GetTopLevelVariable(
    bool ignore_callable) {
  // assert(false);
  LOGE("GetTopLevelVariable.... \n");
  return std::make_unique<lepus::Value>();
}

LEPUSValue QuickContext::GetProperty(const std::string& name,
                                     LEPUSValue this_obj) {
  LEPUSAtom atom = LEPUS_NewAtom(context(), name.c_str());
  LEPUSValue ret = LEPUS_GetProperty(context(), this_obj, atom);
  if (!gc_flag_) LEPUS_FreeAtom(context(), atom);
  if (LEPUS_IsException(ret)) return LEPUS_UNDEFINED;
  return ret;
}

bool QuickContext::GetTopLevelVariableByName(const base::String& name,
                                             lepus::Value* ret) {
  HandleScope func_scope(context());
  LEPUSAtom atom = LEPUS_NewAtom(context(), name.c_str());
  func_scope.PushLEPUSAtom(atom);
  Value variable =
      MK_JS_LEPUS_VALUE(context(), LEPUS_GetGlobalVar(context(), atom, false));
  if (!gc_flag_) LEPUS_FreeAtom(context(), atom);
  if (variable.IsEmpty()) {
    return false;
  }
  *ret = variable;
  return true;
}

void QuickContext::SetGlobalData(const base::String& name, Value value) {
  LEPUSValue v = LEPUSValueHelper::ToJsValue(context(), value);
  if (gc_flag_) {
    HandleScope block_scope(context(), &v, HANDLE_TYPE_LEPUS_VALUE);
    RegisterGlobalProperty(name.c_str(), v);
  } else {
    RegisterGlobalProperty(name.c_str(), v);
  }
}

void QuickContext::ResetGlobalData(const base::String& name, Value value) {
  SetGlobalData(name, std::move(value));
}

lepus::Value QuickContext::GetGlobalData(const base::String& name) {
  return MK_JS_LEPUS_VALUE(context(), SearchGlobalData(name.str()));
}

void QuickContext::SetGCThreshold(int64_t threshold) {
  LEPUS_SetGCThreshold(runtime_, threshold);
}

LEPUSValue QuickContext::GetAndCall(const std::string& name, LEPUSValue* args,
                                    size_t size) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, QUICK_CONTEXT_GET_AND_CALL, "name",
              name);
  LEPUSContext* ctx = context();
  HandleScope func_scope(ctx);
  LEPUSValue ret;
  LEPUSAtom name_atom = LEPUS_NewAtom(context(), name.c_str());
  func_scope.PushLEPUSAtom(name_atom);
  LEPUSValue caller = LEPUS_GetGlobalVar(context(), name_atom, 0);
  if (!LEPUS_IsFunction(lepus_context_, caller)) {
    LEPUS_ThrowTypeError(lepus_context_, "%s is not a function", name.c_str());
    ReportErrorWithMsg(GetExceptionMessage(), error::E_MTS_RUNTIME_ERROR);
    ret = LEPUS_EXCEPTION;
  } else {
    ret = InternalCall(caller, args, size);
  }
  if (!gc_flag_) {
    LEPUS_FreeAtom(context(), name_atom);
    LEPUS_FreeValue(context(), caller);
  }
  return ret;
}

std::string QuickContext::FormatExceptionMessage(const std::string& message,
                                                 const std::string& stack,
                                                 const std::string& prefix) {
  std::string ret(prefix);
  ret = ret + "main-thread.js exception: ";
  ret += message;

  ret += " backtrace:\n";
  ret += stack;

  return ret;
}

// private
std::string QuickContext::GetExceptionMessage(const char* prefix,
                                              int32_t* err_code) {
  LEPUSValue exception_val = LEPUS_GetException(context());
  HandleScope block_scope(context(), &exception_val, HANDLE_TYPE_LEPUS_VALUE);

  auto message = LepusErrorHelper::GetErrorMessage(context(), exception_val);
  auto stack = LepusErrorHelper::GetErrorStack(context(), exception_val);

  if (err_code) {
    *err_code = LepusErrorHelper::GetErrorCode(lepus_context_, exception_val);
  }

  if (!gc_flag_) LEPUS_FreeValue(lepus_context_, exception_val);
  return FormatExceptionMessage(message, stack, prefix);
}

void QuickContext::SetProperty(const char* name, LEPUSValue obj,
                               LEPUSValue val) {
  LEPUSValueHelper::SetProperty(lepus_context_, obj, name, val);
}
void QuickContext::RegisterGlobalProperty(const char* name, LEPUSValue val) {
  LEPUSContext* ctx = context();
  HandleScope func_scope(ctx);
  LEPUSAtom name_atom = LEPUS_NewAtom(context(), name);
  func_scope.PushLEPUSAtom(name_atom);
  LEPUS_SetGlobalVar(context(), name_atom, val, 0);

  if (!LEPUS_IsGCModeRT(runtime_)) {
    LEPUS_FreeAtom(context(), name_atom);
  }
}

void QuickContext::RegisterGlobalFunction(const char* name,
                                          LEPUSCFunction* func, int argc) {
  LEPUSValue c_func = LEPUS_NewCFunction(context(), func, name, argc);
  HandleScope func_scope(context(), &c_func, HANDLE_TYPE_LEPUS_VALUE);
  RegisterGlobalProperty(name, c_func);
}

LEPUSValue QuickContext::NewBindingFunction(RenderBindingFunc func) {
  LEPUSValue binding_func =
      LEPUS_MKPTR(LEPUS_TAG_LEPUS_CPOINTER, reinterpret_cast<void*>(func));
  return LEPUS_NewCFunctionData(
      lepus_context_,
      [](LEPUSContext* ctx, LEPUSValue this_obj, int32_t argc,
         LEPUSValueConst* argv, int32_t magic, LEPUSValue* func_data) {
        auto* qctx = lepus::QuickContext::GetFromJsContext(ctx);
        RenderBindingFunc func = reinterpret_cast<RenderBindingFunc>(
            LEPUS_VALUE_GET_CPOINTER(func_data[0]));
        // 1. prepare args.
        char args_buf[sizeof(lepus::Value) * argc];
        lepus::Value* largv = reinterpret_cast<lepus::Value*>(args_buf);
        for (int32_t i = 0; i < argc; ++i) {
          // High frequency call
          LEPUSValue val = argv[i];
          if (LEPUS_IsLepusRef(val)) {
            new (largv + i) lepus::Value(
                lepus::LEPUSValueHelper::ConstructLepusRefToLynxValue(ctx,
                                                                      val));
          } else {
            new (largv + i)
                lepus::Value(lepus::Context::GetContextCellFromCtx(ctx)->env_,
                             LEPUS_VALUE_GET_INT64(val),
                             lepus::LEPUSValueHelper::CalculateTag(val));
          }
        }
        qctx->set_current_this(this_obj);
        // 2. call function
        LEPUSValue ret =
            LEPUSValueHelper::ToJsValue(ctx, func(qctx, largv, argc));

        // 3. free args.
        for (int32_t i = 0; i < argc; ++i) {
          largv[i].FreeValue();
        }
        qctx->set_current_this(LEPUS_UNDEFINED);
        return ret;
      },
      0, 0, 1, &binding_func);
}

void QuickContext::RegisterGlobalFunction(const RenderBindingFunction* funcs,
                                          size_t size) {
  for (size_t i = 0; i < size; ++i) {
    auto& func = funcs[i];
    auto c_func = NewBindingFunction(func.function);
    HandleScope block_scope{lepus_context_, &c_func, HANDLE_TYPE_LEPUS_VALUE};
    RegisterGlobalProperty(func.name, c_func);
  }
  return;
}

void QuickContext::RegisterObjectFunction(lepus::Value& obj,
                                          const RenderBindingFunction* funcs,
                                          size_t size) {
  for (size_t i = 0; i < size; ++i) {
    auto& func = funcs[i];
    auto c_func = NewBindingFunction(func.function);
    HandleScope block_scope{lepus_context_, &c_func, HANDLE_TYPE_LEPUS_VALUE};
    LEPUSValueHelper::SetProperty(lepus_context_, WRAP_AS_JS_VALUE(obj.value()),
                                  func.name, c_func);
  }
  return;
}

LEPUSValue QuickContext::SearchGlobalData(const std::string& name) {
  HandleScope func_scope(context());
  LEPUSAtom name_atom = LEPUS_NewAtom(context(), name.c_str());
  func_scope.PushLEPUSAtom(name_atom);
  LEPUSValue lepus_val = LEPUS_GetGlobalVar(context(), name_atom, 0);
  if (!gc_flag_) LEPUS_FreeAtom(context(), name_atom);
  return lepus_val;
}

bool QuickContext::DeSerialize(const ContextBundle& bundle, bool reuse_context,
                               Value* ret, const char* file_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, QUICK_CONTEXT_DO_SERIALIZE);
  const auto& quick_bundle = static_cast<const QuickContextBundle&>(bundle);

  if (reuse_context) {
    // file_name is only used for dynamic-components,
    // file_name of page is `lepus.js` by default.
    return EvalBinary(quick_bundle.lepusng_code_.data(),
                      quick_bundle.lepusng_code_.size(), *ret, file_name);
  }

  LEPUSValue val = LEPUS_EvalBinary(
      lepus_context_, quick_bundle.lepusng_code_.data(),
      quick_bundle.lepusng_code_.size(), LEPUS_EVAL_BINARY_LOAD_ONLY);

  if (LEPUS_IsException(val)) {
    std::string msg = GetExceptionMessage();
    LOGE("QuickContext deserialize error " << msg);
    return false;
  }
  if (inspector_manager_ &&
      (tasm::LynxEnv::GetInstance().IsDevToolConnected() ||
       tasm::LynxEnv::GetInstance().IsLogBoxEnabled())) {
    SetFunctionFileName(val, file_name);
  }
  SetTopLevelFunction(val);
  return true;
}

bool QuickContext::EvalBinary(const uint8_t* buf, uint64_t size, Value& ret,
                              const char* file_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, QUICK_CONTEXT_EVAL_BINARY);
  ScriptingScope scope(this);

  LEPUSValue val = LEPUS_EvalBinary(context(), buf, static_cast<size_t>(size),
                                    LEPUS_EVAL_BINARY_LOAD_ONLY);
  HandleScope func_scope(context(), &val, HANDLE_TYPE_LEPUS_VALUE);
  SetFunctionFileName(val, file_name);
  if (LEPUS_IsException(val)) {
    std::string msg = GetExceptionMessage();
    LOGE("QuickContext EvalBinary error: " << msg);
    return false;
  }
  LEPUSValue top_level_function =
      LEPUS_DupValue(context(), top_level_function_);
  func_scope.PushHandle(&top_level_function, HANDLE_TYPE_LEPUS_VALUE);
  SetTopLevelFunction(val);
  ExecuteBinaryInternal(&ret);
  SetTopLevelFunction(top_level_function);
  return true;
}

bool QuickContext::EvalBuf(const char* buf, uint64_t size, Value& ret,
                           const char* file_name) {
  TRACE_EVENT(LYNX_TRACE_CATEGORY_VITALS, QUICK_CONTEXT_EVAL_SCRIPT,
              [&](lynx::perfetto::EventContext ctx) {
                ctx.event()->add_debug_annotations("file_name", file_name);
                ctx.event()->add_debug_annotations("file_size",
                                                   std::to_string(size));
              });
  ScriptingScope scope(this);

  EnsureLynx();
  if (!use_lepus_strict_mode_) {
    LEPUS_SetNoStrictMode(context());
  }

  LEPUSValue val = LEPUS_Eval(context(), buf, static_cast<size_t>(size),
                              file_name, LEPUS_EVAL_TYPE_GLOBAL);
  HandleScope func_scope(context(), &val, HANDLE_TYPE_LEPUS_VALUE);
  if (LEPUS_IsException(val)) {
    std::string msg = GetExceptionMessage();
    LOGE("QuickContext EvalBuf error: " << msg);
    return false;
  }
  EvalLepusPendingTask();
  ret = MK_JS_LEPUS_VALUE(lepus_context_, val);
  return true;
}

void QuickContext::EvalLepusPendingTask() {
  LEPUSContext* ctx = nullptr;
  int result = 0;
  while ((result = LEPUS_ExecutePendingJob(runtime_, &ctx))) {
    if (unlikely(ctx != context())) {
      continue;
    }
    if (result < 0) {
      const std::string log = GetExceptionMessage();
      LOGE(" Call exception: " << log);
      ReportErrorWithMsg(log, error::E_MTS_RUNTIME_ERROR);
    }
  }
  while (LEPUS_MoveUnhandledRejectionToException(context())) {
    std::string log = GetExceptionMessage();
    LOGE("unhandled rejection:" << log);
    ReportErrorWithMsg(log, error::E_MTS_RUNTIME_ERROR);
    // If we caught unhandled rejections, we should still return the
    // real return value of the call instead of undefined.
    // Explicitly fallthrough here.
  }
}

LEPUSValue QuickContext::InternalCall(LEPUSValue caller, LEPUSValue* args,
                                      size_t size) {
  LEPUSValue global = LEPUS_GetGlobalObject(context());
  LEPUSValue ret =
      LEPUS_Call(context(), caller, global, static_cast<int>(size), args);
  HandleScope func_scope(context(), &ret, HANDLE_TYPE_LEPUS_VALUE);
  if (!gc_flag_) LEPUS_FreeValue(context(), global);
  if (LEPUS_IsException(ret)) {
    int32_t err_code;
    const std::string log = GetExceptionMessage("", &err_code);
    LOGE(" Call exception: " << log);
    ReportErrorWithMsg(log, err_code);
    return LEPUS_UNDEFINED;
  }
  LEPUSContext* ctx = nullptr;
  int result = 0;
  while ((result = LEPUS_ExecutePendingJob(runtime_, &ctx))) {
    if (unlikely(ctx != context())) {
      continue;
    }
    if (result < 0) {
      const std::string log = GetExceptionMessage();
      LOGE(" Call exception: " << log);
      ReportErrorWithMsg(log, error::E_MTS_RUNTIME_ERROR);
      return LEPUS_UNDEFINED;
    }
  }
  while (LEPUS_MoveUnhandledRejectionToException(context())) {
    std::string log = GetExceptionMessage();
    LOGE("unhandled rejection:" << log);
    ReportErrorWithMsg(log, error::E_MTS_RUNTIME_ERROR);
    // If we caught unhandled rejections, we should still return the
    // real return value of the call instead of undefined.
    // Explicitly fallthrough here.
  }
  return ret;
}

LEPUSValue QuickContext::ReportSetConstValueError(const LEPUSValue& val,
                                                  LEPUSValue prop) {
  constexpr const char* s =
      "You have attempted to modify an object that is not modifiable."
      "Please manually assign the object before making any changes.";
  /*
    To prevent thrown exceptions from being caught resulting in red screen
    failure.
  */
  std::string error_msg{s};
  const char* prop_name = LEPUS_ToCString(lepus_context_, prop);
  error_msg =
      error_msg + "\nThe property name is :" + (prop_name ? prop_name : "");
  ReportErrorWithMsg(error_msg, error::E_MTS_RUNTIME_ERROR);
  LOGE(error_msg << ", the object content is "
                 << MK_JS_LEPUS_VALUE(lepus_context_, val));
  if (!gc_flag_) LEPUS_FreeCString(lepus_context_, prop_name);
  if (lynx::tasm::LynxEnv::GetInstance().IsDevToolComponentAttach()) {
    return LEPUS_ThrowTypeError(lepus_context_, "%s", error_msg.c_str());
  } else {
    return LEPUS_UNDEFINED;
  }
}

void QuickContext::set_debuginfo_outside(bool val) {
  debuginfo_outside_ = true;
}

bool QuickContext::debuginfo_outside() const { return debuginfo_outside_; }

void QuickContext::RegisterCtxBuiltin(const tasm::ArchOption& option) {
#ifndef LEPUS_PC
  tasm::Utils::RegisterNGBuiltin(this);
  tasm::Renderer::RegisterNGBuiltin(this, option);
#endif
  return;
}

void QuickContext::ApplyConfig(
    const std::shared_ptr<tasm::PageConfig>& page_config,
    const tasm::CompileOptions& options) {
  SetEnableStrictCheck(page_config->GetEnableLepusStrictCheck());
  SetStackSize(page_config->GetLepusQuickjsStackSize());

  if (options.lepusng_debuginfo_outside_) {
    set_debuginfo_outside(true);
  }
  return;
}

bool QuickContextBundle::IsLepusNG() const { return true; }

lepus::Value QuickContext::ReportFatalError(const std::string& error_message,
                                            bool exit, int32_t code) {
  if (exit) {
    LOGE("QuickContext::ReportFatalError: " << error_message);
    abort();
  }
  auto err = LEPUS_NewError(lepus_context_);
  if (LEPUS_IsError(lepus_context_, err)) {
    LEPUS_DefinePropertyValueStr(
        lepus_context_, err, "message",
        LEPUS_NewString(lepus_context_, error_message.c_str()),
        LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_WRITABLE);
    LEPUS_DefinePropertyValueStr(
        lepus_context_, err, LepusErrorHelper::err_code_prop_,
        LEPUS_NewInt32(lepus_context_, code),
        LEPUS_PROP_CONFIGURABLE | LEPUS_PROP_ENUMERABLE);
  }

  return MK_JS_LEPUS_VALUE(lepus_context_, LEPUS_Throw(lepus_context_, err));
}

lepus::Value QuickContext::GetCurrentThis(lepus::Value* argv, int32_t offset) {
  return MK_JS_LEPUS_VALUE(context(), current_this_);
}

void QuickContext::EnableRuntimeLeakCheck(bool enable) {
  SetObjectCtxCheckStatus(context(), enable);
}

void QuickContext::PushContextValidTid() {
  LEPUS_PushObjectCheckTid(context());
}

void QuickContext::UpdateVMOuterObjSize(int size) {
  auto lepus_context = context();
  if (lepus_context != nullptr) {
    auto lepus_runtime = LEPUS_GetRuntime(lepus_context);
    if (lepus_runtime != nullptr) {
      UpdateOuterObjSize(lepus_runtime, size);
    }
  }
}

bool QuickContext::IsTracingGCEnabled() {
  auto lepus_context = context();
  if (lepus_context != nullptr) {
    return LEPUS_IsGCMode(lepus_context);
  }
  return false;
}

#if ENABLE_TRACE_PERFETTO
void QuickContext::SetRuntimeProfiler(
    std::shared_ptr<profile::RuntimeProfiler> runtime_profile) {
  runtime_profiler_ = runtime_profile;
  profile::RuntimeProfilerManager::GetInstance()->AddRuntimeProfiler(
      runtime_profiler_);
}
void QuickContext::RemoveRuntimeProfiler() {
  profile::RuntimeProfilerManager::GetInstance()->RemoveRuntimeProfiler(
      runtime_profiler_);
  runtime_profiler_ = nullptr;
}
#endif

}  // namespace lepus
}  // namespace lynx
