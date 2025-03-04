// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_RUNTIME_MANAGER_DELEGATE_IMPL_H_
#define DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_RUNTIME_MANAGER_DELEGATE_IMPL_H_

#include "core/runtime/piper/js/runtime_manager.h"

namespace lynx {
namespace devtool {

class RuntimeManagerDelegateImpl : public runtime::RuntimeManagerDelegate {
 public:
  RuntimeManagerDelegateImpl() = default;
  ~RuntimeManagerDelegateImpl() override;

  void BeforeRuntimeCreate(bool force_use_lightweight_js_engine) override;
  void OnRuntimeReady(piper::JSExecutor& executor,
                      std::shared_ptr<piper::Runtime>& current_runtime,
                      const std::string& group_id) override;
  void AfterSharedContextCreate(const std::string& group_id,
                                piper::JSRuntimeType type) override;
  void OnRelease(const std::string& group_id) override;
  std::shared_ptr<piper::Runtime> MakeRuntime(
      bool force_use_lightweight_js_engine) override;
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<profile::RuntimeProfiler> MakeRuntimeProfiler(
      std::shared_ptr<piper::JSIContext> js_context,
      bool force_use_lightweight_js_engine) override;
#endif

  void SetReleaseContextCallback(
      piper::JSRuntimeType type,
      const ReleaseContextCallback& callback) override;
  void SetReleaseVMCallback(piper::JSRuntimeType type,
                            const ReleaseVMCallback& callback) override;

 private:
  std::unordered_map<piper::JSRuntimeType, ReleaseVMCallback>
      release_vm_callback_;
  std::unordered_map<piper::JSRuntimeType, ReleaseContextCallback>
      release_context_callback_;
  std::unordered_map<std::string, piper::JSRuntimeType> group_to_engine_type_;
};

}  // namespace devtool
}  // namespace lynx

#endif  // DEVTOOL_LYNX_DEVTOOL_JS_DEBUG_JS_RUNTIME_MANAGER_DELEGATE_IMPL_H_
