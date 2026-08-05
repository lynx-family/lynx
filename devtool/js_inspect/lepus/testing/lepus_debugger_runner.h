// Copyright 2026 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_JS_INSPECT_LEPUS_TESTING_LEPUS_DEBUGGER_RUNNER_H_
#define DEVTOOL_JS_INSPECT_LEPUS_TESTING_LEPUS_DEBUGGER_RUNNER_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "core/runtime/lepus/bytecode_generator.h"
#include "core/runtime/lepusng/quick_context.h"
#include "core/runtime/lepusng/quickjs_debug_info.h"
#include "core/shell/runtime/mts/mts_runtime.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspector_impl.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepus_inspector_ng.h"
#include "devtool/js_inspect/lepus/lepus_internal/lepusng/lepusng_inspected_context_impl.h"
#include "third_party/googletest/googletest/include/gtest/gtest.h"

namespace lynx {
namespace debug {
namespace testing {

// Records protocol messages emitted by the debugger so tests can assert on
// the exact CDP payloads.
class MockLepusChannel
    : public lepus_inspector::LepusInspectorNG::LepusChannel {
 public:
  void SendResponse(int call_id, const std::string& message) override {
    responses_.emplace_back(call_id, message);
  }
  void SendNotification(const std::string& message) override {
    notifications_.push_back(message);
  }

  void Clear() {
    responses_.clear();
    notifications_.clear();
  }

  // (call_id, message) pairs in arrival order.
  std::vector<std::pair<int, std::string>> responses_;
  // Notification payloads in arrival order.
  std::vector<std::string> notifications_;
};

// Minimal client stub that records lifecycle callbacks so tests can verify
// pause/quit interactions without running a real message loop.
class MockLepusInspectorClient
    : public lepus_inspector::LepusInspectorClientNG {
 public:
  void RunMessageLoopOnPause() override { run_message_loop_calls_++; }
  void QuitMessageLoopOnPause() override { quit_message_loop_calls_++; }

  int run_message_loop_calls_{0};
  int quit_message_loop_calls_{0};
};

// Creates a real LepusNG runtime and wires it to a LepusInspectorNG backed
// by mock channel/client. Tests exercise the full debugger flow (script
// compilation, debug info preparation, breakpoints, pause/resume,
// evaluation) against a real VM instead of mocking the runtime.
class LepusDebuggerRunner : public ::testing::Test {
 protected:
  void SetUp() override {
    runtime_ = runtime::MTSRuntime::CreateContext(
        runtime::ContextType::LepusNGContextType);
    runtime_->Initialize();
    inspector_ = lepus_inspector::LepusInspectorNG::Create(
        runtime_->GetMTSContext(), &client_, "test");
    session_ = inspector_->Connect(&channel_);
  }

  void TearDown() override {
    session_.reset();
    inspector_.reset();
    runtime_.reset();
  }

  // Compiles |source| into bytecode. Compilation triggers
  // OnTopLevelFunctionReady so the debugger prepares debug info.
  void CompileScript(const std::string& source) {
    lepus::BytecodeGenerator::GenerateBytecode(
        runtime_->GetMTSContext(), source, runtime_->GetSdkVersion(), "");
  }

  // Mirrors the production DeSerialize flow: after bytecode is loaded the
  // runtime notifies the debugger that the top-level function is ready,
  // which triggers PrepareDebugInfo for the registered debug info.
  void PrepareInspector() { runtime_->PrepareInspector(nullptr); }

  // Supplies debug info for the compiled script, mirroring the production
  // flow where the devtool downloads and forwards debug-info.json.
  void SetDebugInfo(const std::string& filename, const std::string& debug_info,
                    int debug_info_id, const std::string& debug_info_url) {
    inspector_->SetDebugInfo(filename, debug_info, debug_info_id,
                             debug_info_url);
  }

  // Executes the compiled top-level function.
  void Execute() { runtime_->Execute(nullptr); }

  // Dispatches a CDP protocol message to the debugger.
  void Dispatch(const std::string& message) {
    session_->DispatchProtocolMessage(message);
  }

  // Compiles |source| with debug info enabled and returns the generated
  // debug-info JSON, mirroring the production devtool flow. The generated
  // document is wrapped in the "lepusNG_debug_info" entry that the
  // debugger expects for the default (first) script.
  std::string BuildDebugInfo(const std::string& source) {
    lepus::QuickContext* qctx =
        runtime::MTSRuntime::ToQuickContext(runtime_.get());
    qctx->set_debuginfo_outside(true);
    lepus::BytecodeGenerator::GenerateBytecode(
        runtime_->GetMTSContext(), source, runtime_->GetSdkVersion(), "");
    std::string inner = lepus::QuickjsDebugInfoBuilder::BuildJsDebugInfo(
        qctx->context(), qctx->GetTopLevelFunction(), source,
        /*debuginfo_outside=*/true, /*var_defs_outside=*/true);
    return R"({"lepusNG_debug_info":)" + inner + "}";
  }

  // Direct access to the underlying debugger, for scenarios that need to
  // invoke debugger internals that are normally driven by the VM callbacks.
  lynx::debug::LepusNGDebugger* GetDebugger() {
    auto* impl =
        static_cast<lepus_inspector::LepusInspectorNGImpl*>(inspector_.get());
    auto context = impl->GetContext();
    if (!context) {
      return nullptr;
    }
    return static_cast<lepus_inspector::LepusNGInspectedContextImpl*>(
               context.get())
        ->GetDebugger()
        .get();
  }

  lepus_inspector::LepusInspectorNGImpl* GetInspectorImpl() {
    return static_cast<lepus_inspector::LepusInspectorNGImpl*>(
        inspector_.get());
  }

  MockLepusChannel channel_;
  MockLepusInspectorClient client_;
  std::shared_ptr<runtime::MTSRuntime> runtime_;
  std::unique_ptr<lepus_inspector::LepusInspectorNG> inspector_;
  std::unique_ptr<lepus_inspector::LepusInspectorSessionNG> session_;
};

}  // namespace testing
}  // namespace debug
}  // namespace lynx

#endif  // DEVTOOL_JS_INSPECT_LEPUS_TESTING_LEPUS_DEBUGGER_RUNNER_H_
