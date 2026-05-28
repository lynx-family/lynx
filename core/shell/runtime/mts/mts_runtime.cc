// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/shell/runtime/mts/mts_runtime.h"

#include <memory>
#include <utility>

#include "base/include/value/path_parser.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/common/lynx_console_helper.h"
#include "core/runtime/lepus/tasks/lepus_callback_manager.h"
#include "core/runtime/lepus/tasks/lepus_raf_manager.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/runtime/mts_context_factory.h"
#include "core/runtime/trace/runtime_trace_event_def.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_config_constant_auto_gen.h"

namespace lynx {
namespace runtime {

MTSContextHolder::MTSContextHolder(std::unique_ptr<MTSContext> mts_context)
    : mts_context_(std::move(mts_context)) {}

MTSRuntime::~MTSRuntime() { DestroyInspector(); }

lepus::VMContext* MTSRuntime::ToVMContext(MTSRuntime* context) {
  DCHECK(context->IsVMContext());

  return static_cast<lepus::VMContext*>(context->mts_context_.get());
}

lepus::QuickContext* MTSRuntime::ToQuickContext(MTSRuntime* context) {
  DCHECK(context->IsLepusNGContext());
  return static_cast<lepus::QuickContext*>(context->mts_context_.get());
}

MTSRuntime* MTSRuntime::ToContext(MTSContext* mts_context) {
  return reinterpret_cast<MTSRuntime*>(mts_context->GetRuntimePrivate());
}

void MTSRuntime::Initialize() { mts_context_->Initialize(); }

void MTSRuntime::EnsureDelegate() {
  if (delegate_ != nullptr) {
    return;
  }
  Value delegate_point =
      GetGlobalData(BASE_STATIC_STRING(tasm::kTemplateAssembler));
  if (delegate_point.IsCPointer()) {
    delegate_ =
        reinterpret_cast<MTSRuntime::Delegate*>(delegate_point.CPoint());
  } else {
    LOGE("Not Found TemplateAssembler Instance");
  }
}

void MTSRuntime::PushScriptingScope(ScriptingScope* scope) {
  scripting_scope_stack_.push(scope);
}
void MTSRuntime::PopScriptingScope() { scripting_scope_stack_.pop(); }

MTSRuntime::ScriptingScope::ScriptingScope(MTSRuntime* context)
    : ctx_(context) {
  CheckOnScriptingStart();
  ctx_->PushScriptingScope(this);
}

MTSRuntime::ScriptingScope::~ScriptingScope() {
  ctx_->PopScriptingScope();
  CheckOnScriptingEnd();
}

void MTSRuntime::ScriptingScope::CheckOnScriptingStart() {
  if (!ctx_->scripting_scope_stack_.empty()) {
    return;
  }
  ctx_->EnsureDelegate();
  if (ctx_->delegate_ == nullptr) {
    return;
  }
  ctx_->delegate_->OnScriptingStart();
}

void MTSRuntime::ScriptingScope::CheckOnScriptingEnd() {
  if (!ctx_->scripting_scope_stack_.empty()) {
    return;
  }
  if (ctx_->delegate_ == nullptr) {
    return;
  }
  ctx_->delegate_->OnScriptingEnd();
}

MTSRuntime::Delegate* MTSRuntime::GetDelegate() {
  EnsureDelegate();
  return delegate_;
}

std::shared_ptr<MTSRuntime> MTSRuntime::CreateContext(
    ContextType type, bool disable_tracing_gc, int runtime_mode,
    const tasm::PageOptions& page_options) {
  if (type == ContextType::VMContextType) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CONTEXT_CREATE_VM_CONTEXT);
#if !ENABLE_JUST_LEPUSNG
    return std::make_shared<MTSRuntime>(ContextType::VMContextType,
                                        disable_tracing_gc, runtime_mode,
                                        page_options);
#else
    LOGE("lepusng sdk do not support vm context");
    assert(false);
    return nullptr;
#endif
  } else {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CONTEXT_CREATE_MTS_RUNTIME);
    return std::make_shared<MTSRuntime>(type, disable_tracing_gc, runtime_mode,
                                        page_options);
  }
}

bool MTSRuntime::Execute(const ContextBundle* bundle) {
  if (HasPreExecuteSuccess()) {
    ResetPreExecuteSuccess();
    return true;
  }
  ScriptingScope scope(this);

  EnsureLynx();
  return mts_context_->ExecuteBinaryWithBundle(bundle, nullptr);
}

void MTSRuntime::EnsureLynx() {
  // Initialize and inject Lynx if it has not been set before.
  if (lynx_.IsEmpty()) {
#ifndef LEPUS_PC
    lynx_ = tasm::Utils::CreateLynx(this, GetSdkVersion());
#else
    lynx_ = lepus::LEPUSValueHelper::CreateObject(this);
#endif
    SetGlobalData(BASE_STATIC_STRING(tasm::kGlobalLynx), lynx_);
  }
}

void MTSRuntime::SetPropertyToLynx(const base::String& key,
                                   const lepus::Value& value) {
  EnsureLynx();
  lynx_.SetProperty(key, value);
}

const std::shared_ptr<tasm::LepusCallbackManager>&
MTSRuntime::GetCallbackManager() const {
  if (!callback_manager_) {
    callback_manager_ = std::make_shared<tasm::LepusCallbackManager>();
  }
  return callback_manager_;
}

const std::shared_ptr<tasm::AnimationFrameManager>&
MTSRuntime::GetAnimationFrameManager() const {
  if (!animate_frame_manager_) {
    animate_frame_manager_ = std::make_shared<tasm::AnimationFrameManager>();
  }
  return animate_frame_manager_;
}

void MTSRuntime::RegisterLynx(bool enable_signal_api) {
  BASE_STATIC_STRING_DECL(kPostDataBeforeUpdate, "postDataBeforeUpdate");
  BASE_STATIC_STRING_DECL(kTriggerReadyWhenReload, "triggerReadyWhenReload");
  BASE_STATIC_STRING_DECL(kEnableOnLayoutReadyHook, "enableOnLayoutReady");

  SetPropertyToLynx(BASE_STATIC_STRING(tasm::kSystemInfo),
                    tasm::GenerateSystemInfo(nullptr));
  SetPropertyToLynx(kTriggerReadyWhenReload, lepus::Value(true));
  if (tasm::LynxEnv::GetInstance().EnablePostDataBeforeUpdateTemplate()) {
    SetPropertyToLynx(kPostDataBeforeUpdate, lepus::Value(true));
  }
  SetPropertyToLynx(BASE_STATIC_STRING(tasm::config::kEnableSignalAPI),
                    lepus::Value(enable_signal_api));
  SetPropertyToLynx(kEnableOnLayoutReadyHook, lepus::Value(true));
}

void MTSRuntime::UpdateGCTiming(bool is_start) {
  mts_context_->UpdateGCTiming(is_start);
}

void MTSRuntime::TriggerVmGC() { mts_context_->TriggerVmGC(); }

void MTSRuntime::RegisterGlobalFunction(const RenderBindingFunction* funcs,
                                        size_t size) {
  mts_context_->RegisterGlobalFunction(funcs, size);
}
void MTSRuntime::RegisterObjectFunction(lepus::Value& obj,
                                        const RenderBindingFunction* funcs,
                                        size_t size) {
  mts_context_->RegisterObjectFunction(obj, funcs, size);
}

void MTSRuntime::ReportError(const std::string& exception_info,
                             int32_t err_code,
                             base::LynxErrorLevel error_level) {
#ifndef LEPUS_PC
  EnsureDelegate();

  if (!delegate_) {
    return;
  }

  base::LynxError error{err_code, exception_info, "", error_level};
  error.custom_info_ = {{"name", name_},
                        {"type", std::to_string(static_cast<int>(type_))}};
  if (name_.compare(LEPUS_DEFAULT_CONTEXT_NAME) != 0) {
    FormatErrorUrl(error, name_);
  }
  BeforeReportError(error);
  delegate_->ReportError(std::move(error));
#endif
}

void MTSRuntime::OnBTSConsoleEvent(const std::string& func_name,
                                   const std::string& args) {
  EnsureDelegate();

  if (!delegate_) {
    return;
  }

  delegate_->OnBTSConsoleEvent(func_name, args);
}

void MTSRuntime::SetSourceMapRelease(const lepus::Value& source_map_release) {
  BASE_STATIC_STRING_DECL(kStack, "stack");
  if (!(source_map_release.GetProperty(kStack).IsString())) {
    LOGI(
        "MTSRuntime::SetSourceMapRelease, can't found Error, stack is "
        "undefined");
    return;
  }
  BASE_STATIC_STRING_DECL(kMessage, "message");
  if (!(source_map_release.GetProperty(kMessage).IsString())) {
    LOGI(
        "MTSRuntime::SetSourceMapRelease, can't found Error, message is "
        "undefined");
    return;
  }

  JSErrorInfo args;
  args.message = source_map_release.GetProperty(kMessage).StdString();
  args.stack = source_map_release.GetProperty(kStack).StdString();
  OnBTSConsoleEvent(ConsoleInfo, "SetSourceMapRelease.message:" + args.message);
  OnBTSConsoleEvent(ConsoleInfo, "SetSourceMapRelease.stack:" + args.stack);
  js_error_reporter_.SetSourceMapRelease(std::move(args));
}

void MTSRuntime::ReportErrorWithMsg(const std::string& msg,
                                    const std::string& stack,
                                    int32_t error_code, int32_t error_level) {
  auto formatted_message = mts_context_->FormatExceptionMessage(msg, stack, "");
  ReportErrorWithMsg(formatted_message, error_code, error_level);
}

void MTSRuntime::ReportErrorWithMsg(const std::string& msg, int32_t error_code,
                                    int32_t error_level) {
  if (is_handling_exception_) {
    return;
  }
  is_handling_exception_ = true;

  EnsureDelegate();

  // enable outside debug information only when engine version is bigger than
  // "2.7" and "debuginfo_outside_" is true when encode
  if (!delegate_) {
    is_handling_exception_ = false;
    return;
  }

  const auto& target_sdk_version = delegate_->TargetSdkVersion();
  OnBTSConsoleEvent(ConsoleInfo,
                    "ReportErrorWithMsg.engine version:" + target_sdk_version);
  if (tasm::Config::IsHigherOrEqual(target_sdk_version, LYNX_VERSION_2_7) &&
      mts_context_->debuginfo_outside()) {
    auto error = js_error_reporter_.SendMTError(msg, error_code, error_level);
    if (error) {
      delegate_->ReportError(std::move(*error));
    }
  } else {
    ReportError(msg, error_code,
                static_cast<base::LynxErrorLevel>(error_level));
  }
  OnBTSConsoleEvent(ConsoleError, msg);
  is_handling_exception_ = false;
}

void MTSRuntime::BeforeReportError(base::LynxError& error) {
  js_error_reporter_.AppendCustomInfo(error);
}

void MTSRuntime::AddReporterCustomInfo(
    const std::unordered_map<std::string, std::string>& info) {
  js_error_reporter_.AddCustomInfoToError(info);
}

void MTSRuntime::InitInspector(
    const std::shared_ptr<lepus::InspectorLepusObserver>& observer,
    const std::string& context_name) {
  if (observer != nullptr) {
    inspector_manager_ =
        observer->CreateLepusInspectorManager(mts_context_->Type());
    if (inspector_manager_ != nullptr) {
      mts_context_->set_is_debug_enabled(true);
      inspector_manager_->InitInspector(mts_context_.get(), observer,
                                        context_name);
    }
  }
}

void MTSRuntime::PrepareInspector(const char* file_name) {
  mts_context_->PrepareInspector(file_name);
}

void MTSRuntime::DestroyInspector() {
  if (inspector_manager_ != nullptr) {
    mts_context_->set_is_debug_enabled(false);
    inspector_manager_->DestroyInspector();
  }
}

std::shared_ptr<lepus::InspectorLepusObserver> MTSRuntime::UpdateInspector(
    const std::shared_ptr<lepus::InspectorLepusObserver>& observer) {
  if (inspector_manager_ != nullptr) {
    return inspector_manager_->UpdateInspector(observer);
  }
  return nullptr;
}

void MTSRuntime::SetDebugInfoURL(const std::string& url,
                                 const std::string& file_name) {
  if (inspector_manager_ != nullptr) {
    inspector_manager_->SetDebugInfo(url, file_name);
  }

  mts_context_->SetDebugInfoURL(url, file_name);
}

bool MTSRuntime::UpdateTopLevelVariable(const std::string& name,
                                        const Value& val) {
  auto path = lepus::ParseValuePath(name);
  return UpdateTopLevelVariableByPath(path, val);
}

bool MTSRuntime::UpdateTopLevelVariableByPath(base::Vector<std::string>& path,
                                              const Value& val) {
  return mts_context_->UpdateTopLevelVariableByPath(path, val);
}

// shadow equal for table
bool MTSRuntime::CheckTableShadowUpdatedWithTopLevelVariable(
    const lepus::Value& update) {
  return mts_context_->CheckTableShadowUpdatedWithTopLevelVariable(update);
}

void MTSRuntime::ResetTopLevelVariable() {
  mts_context_->ResetTopLevelVariable();
}
void MTSRuntime::ResetTopLevelVariableByVal(const Value& val) {
  mts_context_->ResetTopLevelVariableByVal(val);
}

lepus::Value MTSRuntime::GetTopLevelVariable(bool ignore_callable) {
  return mts_context_->GetTopLevelVariable(ignore_callable);
}
bool MTSRuntime::GetTopLevelVariableByName(const base::String& name,
                                           lepus::Value* ret) {
  return mts_context_->GetTopLevelVariableByName(name, ret);
}

void MTSRuntime::SetGlobalData(const base::String& name, Value value) {
  mts_context_->SetGlobalData(name, value);
}

/**
 * This value will overwrite the origin value
 */
void MTSRuntime::ResetGlobalData(const base::String& name, Value value) {
  mts_context_->ResetGlobalData(name, value);
}
lepus::Value MTSRuntime::GetGlobalData(const base::String& name) {
  return mts_context_->GetGlobalData(name);
}

void MTSRuntime::SetGCThreshold(int64_t threshold) {
  mts_context_->SetGCThreshold(threshold);
}

int64_t MTSRuntime::GetCurrentHeapSizeBytes() {
  return mts_context_ ? mts_context_->GetCurrentHeapSizeBytes() : 0;
}

inline constexpr char kRawRuntimeMemoryInfo[] = "raw_memory_info_json_str";

void MTSRuntime::OnGC(std::string mem_info) {
  if (!delegate_) {
    return;
  }
  delegate_->OnRuntimeGC({{kRawRuntimeMemoryInfo, std::move(mem_info)}});
}

void MTSRuntime::ReportGCTimingEvent(const char* start, const char* end) {
  if (!delegate_) {
    return;
  }
  delegate_->ReportGCTimingEvent(start, end);
}

void MTSRuntime::OnReload() { mts_context_->OnReload(); }

bool MTSRuntime::DeSerialize(const ContextBundle& bundle,
                             bool is_lepusng_binary, Value* ret,
                             const char* file_name) {
  return mts_context_->DeSerialize(bundle, is_lepusng_binary, ret, file_name);
}

void MTSRuntime::ApplyConfig(const std::shared_ptr<tasm::PageConfig>& config,
                             const tasm::CompileOptions& options) {
  mts_context_->ApplyConfig(config, options);
}

lepus::Value MTSRuntime::ReportFatalError(const std::string& error_message,
                                          bool exit, int32_t code) {
  return mts_context_->ReportFatalError(error_message, exit, code);
}

Value MTSRuntime::CallArgs(const base::String& name,
                           const std::vector<Value>& args,
                           bool pause_suppression_mode) {
  const Value* p_args[args.size()];
  for (size_t i = 0; i < args.size(); ++i) {
    p_args[i] = &args[i];
  }

  ScriptingScope scope(this);
  return mts_context_->CallArgs(name, p_args, args.size(),
                                pause_suppression_mode);
}

Value MTSRuntime::CallClosureArgs(const Value& closure,
                                  const std::vector<Value>& args) {
  const Value* p_args[args.size()];
  for (size_t i = 0; i < args.size(); ++i) {
    p_args[i] = &args[i];
  }

  ScriptingScope scope(this);
  return mts_context_->CallClosureArgs(closure, p_args, args.size());
}

bool MTSRuntime::TryExecute() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CONTEXT_TRY_EXECUTE);
  has_pre_execute_success_ = Execute(nullptr);
  return has_pre_execute_success_;
}

bool MTSRuntime::HasPreExecuteSuccess() { return has_pre_execute_success_; }

void MTSRuntime::ResetPreExecuteSuccess() { has_pre_execute_success_ = false; }

std::unique_ptr<ContextBundle> ContextBundle::Create(ContextType context_type) {
  return ContextBundleFactory::Create(context_type);
}

MTSRuntime::MTSRuntime(ContextType type, bool disable_tracing_gc,
                       int runtime_mode, const tasm::PageOptions& page_options)
    : MTSContextHolder(MTSContextFactory::Create(type, this, disable_tracing_gc,
                                                 runtime_mode, page_options)),
      type_(type) {}

}  // namespace runtime
}  // namespace lynx
