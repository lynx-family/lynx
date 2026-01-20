// Copyright 2020 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "core/runtime/lepus/context.h"

#include <memory>
#include <unordered_set>
#include <utility>

#include "base/include/value/array.h"
#include "base/include/value/path_parser.h"
#include "base/include/value/ref_counted_class.h"
#include "base/include/value/table.h"
#include "base/trace/native/trace_event.h"
#include "core/renderer/utils/base/base_def.h"
#include "core/renderer/utils/base/tasm_constants.h"
#include "core/renderer/utils/base/tasm_utils.h"
#include "core/renderer/utils/lynx_env.h"
#include "core/runtime/common/js_error_reporter.h"
#include "core/runtime/common/utils.h"
#include "core/runtime/lepus/js_object.h"
#include "core/runtime/lepus/tasks/lepus_callback_manager.h"
#include "core/runtime/lepus/tasks/lepus_raf_manager.h"
#include "core/runtime/lepus/vm_context.h"
#include "core/runtime/lepusng/jsvalue_helper.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/template_bundle/template_codec/binary_decoder/lynx_config_constant_auto_gen.h"

namespace lynx {
namespace lepus {

Context::Context(ContextType type) : type_(type) {}

void Context::EnsureDelegate() {
  if (delegate_ != nullptr) {
    return;
  }
  Value delegate_point =
      GetGlobalData(BASE_STATIC_STRING(tasm::kTemplateAssembler));
  if (delegate_point.IsCPointer()) {
    delegate_ = reinterpret_cast<Context::Delegate*>(delegate_point.CPoint());
  } else {
    LOGE("Not Found TemplateAssembler Instance");
  }
}

void Context::PushScriptingScope(ScriptingScope* scope) {
  scripting_scope_stack_.push(scope);
}
void Context::PopScriptingScope() { scripting_scope_stack_.pop(); }

Context::ScriptingScope::ScriptingScope(Context* context) : ctx_(context) {
  CheckOnScriptingStart();
  ctx_->PushScriptingScope(this);
}

Context::ScriptingScope::~ScriptingScope() {
  ctx_->PopScriptingScope();
  CheckOnScriptingEnd();
}

void Context::ScriptingScope::CheckOnScriptingStart() {
  if (!ctx_->scripting_scope_stack_.empty()) {
    return;
  }
  ctx_->EnsureDelegate();
  if (ctx_->delegate_ == nullptr) {
    return;
  }
  ctx_->delegate_->OnScriptingStart();
}

void Context::ScriptingScope::CheckOnScriptingEnd() {
  if (!ctx_->scripting_scope_stack_.empty()) {
    return;
  }
  if (ctx_->delegate_ == nullptr) {
    return;
  }
  ctx_->delegate_->OnScriptingEnd();
}

Context::Delegate* Context::GetDelegate() {
  EnsureDelegate();
  return delegate_;
}

std::shared_ptr<Context> Context::CreateContext(
    bool use_lepusng, bool disable_tracing_gc, int runtime_mode,
    const tasm::PageOptions& page_options) {
  if (use_lepusng) {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CONTEXT_CREATE_QUICK_CONTEXT);
    return std::make_shared<QuickContext>(disable_tracing_gc, runtime_mode,
                                          page_options);
  } else {
    TRACE_EVENT(LYNX_TRACE_CATEGORY, CONTEXT_CREATE_VM_CONTEXT);
#if !ENABLE_JUST_LEPUSNG
    return std::make_shared<VMContext>();
#else
    LOGE("lepusng sdk do not support vm context");
    assert(false);
    return NULL;
#endif
  }
}

void Context::EnsureLynx() {
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

void Context::SetPropertyToLynx(const base::String& key,
                                const lepus::Value& value) {
  EnsureLynx();
  lynx_.SetProperty(key, value);
}

const std::shared_ptr<tasm::LepusCallbackManager>& Context::GetCallbackManager()
    const {
  if (!callback_manager_) {
    callback_manager_ = std::make_shared<tasm::LepusCallbackManager>();
  }
  return callback_manager_;
}

const std::shared_ptr<tasm::AnimationFrameManager>&
Context::GetAnimationFrameManager() const {
  if (!animate_frame_manager_) {
    animate_frame_manager_ = std::make_shared<tasm::AnimationFrameManager>();
  }
  return animate_frame_manager_;
}

void Context::RegisterLynx(bool enable_signal_api) {
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

void Context::ReportError(const std::string& exception_info, int32_t err_code,
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
    common::FormatErrorUrl(error, name_);
  }
  BeforeReportError(error);
  delegate_->ReportError(std::move(error));
#endif
}

void Context::OnBTSConsoleEvent(const std::string& func_name,
                                const std::string& args) {
  EnsureDelegate();

  if (!delegate_) {
    return;
  }

  delegate_->OnBTSConsoleEvent(func_name, args);
}

void Context::InitInspector(
    const std::shared_ptr<InspectorLepusObserver>& observer,
    const std::string& context_name) {
  if (observer != nullptr) {
    inspector_manager_ = observer->CreateLepusInspectorManager();
    if (inspector_manager_ != nullptr) {
      inspector_manager_->InitInspector(this, observer, context_name);
    }
  }
}

void Context::DestroyInspector() {
  if (inspector_manager_ != nullptr) {
    inspector_manager_->DestroyInspector();
  }
}

void Context::SetDebugInfoURL(const std::string& url,
                              const std::string& file_name) {
  if (inspector_manager_ != nullptr) {
    inspector_manager_->SetDebugInfo(url, file_name);
  }
}

CellManager::~CellManager() {
  for (auto* itr : cells_) {
    delete itr;
  }
}

ContextCell* CellManager::AddCell(lepus::QuickContext* qctx) {
  LEPUSContext* ctx = qctx->context();
  ContextCell* ret = new ContextCell(qctx, ctx, LEPUS_GetRuntime(ctx));
  cells_.emplace_back(ret);
  return ret;
}

bool Context::UpdateTopLevelVariable(const std::string& name,
                                     const Value& val) {
  auto path = ParseValuePath(name);
  return UpdateTopLevelVariableByPath(path, val);
}

Value Context::CallArgs(const base::String& name,
                        const std::vector<Value>& args,
                        bool pause_suppression_mode) {
  const Value* p_args[args.size()];
  for (size_t i = 0; i < args.size(); ++i) {
    p_args[i] = &args[i];
  }
  return CallArgs(name, p_args, args.size(), pause_suppression_mode);
}

Value Context::CallClosureArgs(const Value& closure,
                               const std::vector<Value>& args) {
  const Value* p_args[args.size()];
  for (size_t i = 0; i < args.size(); ++i) {
    p_args[i] = &args[i];
  }
  return CallClosureArgs(closure, p_args, args.size());
}

bool Context::TryExecute() {
  TRACE_EVENT(LYNX_TRACE_CATEGORY, CONTEXT_TRY_EXECUTE);
  has_pre_execute_success_ = Execute();
  return has_pre_execute_success_;
}

bool Context::HasPreExecuteSuccess() {
  if (has_pre_execute_success_) {
    // can only use once.
    has_pre_execute_success_ = false;
    return true;
  }
  return false;
}

std::unique_ptr<ContextBundle> ContextBundle::Create(bool is_lepusng_binary) {
  if (is_lepusng_binary) {
    return std::make_unique<QuickContextBundle>();
  }
#if !ENABLE_JUST_LEPUSNG
  return std::make_unique<VMContextBundle>();
#else
  return nullptr;
#endif
}

}  // namespace lepus
}  // namespace lynx
