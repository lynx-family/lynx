// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_INSPECTED_CONTEXT_IMPL_H_
#define DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_INSPECTED_CONTEXT_IMPL_H_

#include <memory>
#include <string>

#include "core/runtime/vm/lepus/quick_context.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspected_context.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspector_impl.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_debugger.h"

namespace lepus_inspector {

class LepusNGInspectedContextImpl
    : public LepusInspectedContext,
      public lynx::lepus::QuickContext::DebugDelegate {
 public:
  LepusNGInspectedContextImpl(LepusInspectorNGImpl* inspector,
                              lynx::lepus::Context* context,
                              const std::string& name);
  ~LepusNGInspectedContextImpl() override = default;

  void SetDebugInfo(const std::string& url,
                    const std::string& debug_info) override;
  void ProcessMessage(const std::string& message) override;

  void OnTopLevelFunctionReady() override;

  lynx::lepus::QuickContext* GetContext() { return context_; }
  LEPUSContext* GetLepusContext() { return context_->context(); }
  const std::unique_ptr<lynx::debug::LepusNGDebugger>& GetDebugger() {
    return debugger_;
  }

 private:
  void RegisterCallbacks();

  lynx::lepus::QuickContext* context_;
  std::unique_ptr<lynx::debug::LepusNGDebugger> debugger_;
};

}  // namespace lepus_inspector

#endif  // DEVTOOL_JS_INSPECT_LEPUS_LEPUS_INTERNAL_LEPUSNG_LEPUSNG_INSPECTED_CONTEXT_IMPL_H_
