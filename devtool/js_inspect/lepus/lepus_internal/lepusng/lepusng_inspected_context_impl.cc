// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"

#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_callbacks.h"

namespace lepus_inspector {

LepusNGInspectedContextImpl::LepusNGInspectedContextImpl(
    LepusInspectorNGImpl* inspector, lynx::lepus::Context* context,
    const std::string& name) {
  context_ = static_cast<lynx::lepus::QuickContext*>(context);
  debugger_ =
      std::make_unique<lynx::debug::LepusNGDebugger>(this, inspector, name);
  RegisterCallbacks();
}

void LepusNGInspectedContextImpl::SetDebugInfo(const std::string& url,
                                               const std::string& debug_info) {
  context_->SetDebugDelegate(
      std::static_pointer_cast<LepusNGInspectedContextImpl>(
          shared_from_this()));
  debugger_->SetDebugInfo(url, debug_info);
}

void LepusNGInspectedContextImpl::ProcessMessage(const std::string& message) {
  debugger_->ProcessPausedMessages(message);
}

void LepusNGInspectedContextImpl::OnTopLevelFunctionReady() {
  debugger_->PrepareDebugInfo();
}

void LepusNGInspectedContextImpl::RegisterCallbacks() {
  auto& funcs = GetDebuggerCallbackFuncs();
  RegisterQJSDebuggerCallbacks(LEPUS_GetRuntime(GetLepusContext()),
                               funcs.data(), funcs.size());
}

}  // namespace lepus_inspector
