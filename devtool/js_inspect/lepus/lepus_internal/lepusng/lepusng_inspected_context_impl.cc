// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"

#include <utility>

#include "core/shell/runtime/mts/mts_runtime.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_callbacks.h"

namespace lepus_inspector {

LepusNGInspectedContextImpl::LepusNGInspectedContextImpl(
    LepusInspectorNGImpl* inspector, lynx::runtime::MTSContext* context,
    const std::string& name) {
  context_ = static_cast<lynx::lepus::QuickContext*>(context);
  runtime_ = nullptr;
  interrupt_.reset();
  if (context_ != nullptr && context_->context() != nullptr) {
    runtime_ = LEPUS_GetRuntime(context_->context());
  }
  if (runtime_ != nullptr) {
    interrupt_ =
        std::make_unique<lynx::devtool::InspectorPrimjsInterruptHelper>(
            runtime_);
  }
  debugger_ =
      std::make_unique<lynx::debug::LepusNGDebugger>(this, inspector, name);
  RegisterCallbacks();
}

void LepusNGInspectedContextImpl::Init() {
  context_->SetDebugDelegate(
      std::static_pointer_cast<LepusNGInspectedContextImpl>(
          shared_from_this()));
}

void LepusNGInspectedContextImpl::SetDebugInfo(
    const std::string& filename, const std::string& debug_info,
    int debug_info_id, const std::string& debug_info_url) {
  debugger_->SetDebugInfo(filename, debug_info, debug_info_id, debug_info_url);
}

void LepusNGInspectedContextImpl::ProcessMessage(const std::string& message) {
  debugger_->ProcessPausedMessages(message);
}

void LepusNGInspectedContextImpl::OnTopLevelFunctionReady() {
  debugger_->PrepareDebugInfo();
}

void LepusNGInspectedContextImpl::RequestInterrupt(
    lynx::base::closure&& closure) {
  if (runtime_ == nullptr) {
    return;
  }
  if (interrupt_ != nullptr) {
    interrupt_->Request(std::move(closure));
  }
}

void LepusNGInspectedContextImpl::RegisterCallbacks() {
  auto& funcs = GetDebuggerCallbackFuncs();
  RegisterQJSDebuggerCallbacks(runtime_, funcs.data(), funcs.size());
}

}  // namespace lepus_inspector
