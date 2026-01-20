// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JS_CONTEXT_WRAPPER_H_
#define CORE_RUNTIME_JS_JS_CONTEXT_WRAPPER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "core/base/lynx_export.h"
#include "core/runtime/common/napi/napi_environment.h"
#include "core/runtime/js/bindings/global.h"
#include "core/runtime/js/jsi/jsi.h"
#include "core/runtime/js/runtime_lifecycle_listener_delegate.h"
#include "core/runtime/js/runtime_lifecycle_observer_impl.h"
#include "core/runtime/profile/runtime_profiler.h"

namespace lynx {
namespace runtime {

class LYNX_EXPORT_FOR_DEVTOOL JSContextWrapper
    : public runtime::js::JSIContext::Observer,
      public std::enable_shared_from_this<JSContextWrapper> {
 public:
  JSContextWrapper(std::shared_ptr<runtime::js::JSIContext>);
  ~JSContextWrapper() = default;

  virtual void Def() = 0;
  virtual void EnsureConsole(
      std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
      const tasm::PageOptions& page_options) = 0;
  virtual void initGlobal(
      std::shared_ptr<runtime::js::Runtime>& js_runtime,
      std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
      const tasm::PageOptions& page_options) = 0;
  virtual void AddLifecycleListener(
      std::unique_ptr<RuntimeLifecycleListenerDelegate> listener){};
  virtual runtime::js::NapiEnvironment* GetNapiEnvironment() {
    return nullptr;
  };

  bool isGlobalInited() { return global_inited_; }
  bool isJSCoreLoaded() { return js_core_loaded_; }
  void prepareJSEnv(
      std::weak_ptr<runtime::js::Runtime> js_runtime,
      std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
          js_preload);
  std::shared_ptr<runtime::js::JSIContext> getJSContext() {
    return js_context_.lock();
  }
#if ENABLE_TRACE_PERFETTO
  void SetRuntimeProfiler(
      std::shared_ptr<profile::RuntimeProfiler> runtime_profiler);
#endif
 protected:
  virtual void InitNapi(std::shared_ptr<runtime::js::Runtime>& js_runtime){};
  std::weak_ptr<runtime::js::JSIContext> js_context_;
  bool js_core_loaded_;
  bool global_inited_;
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<profile::RuntimeProfiler> runtime_profiler_;
#endif
};

class LYNX_EXPORT_FOR_DEVTOOL SharedJSContextWrapper : public JSContextWrapper {
 public:
  class ReleaseListener {
   public:
    virtual void OnRelease(const std::string& group_id) = 0;
    virtual ~ReleaseListener() = default;
  };
  SharedJSContextWrapper(std::shared_ptr<runtime::js::JSIContext>,
                         const std::string& group_id,
                         ReleaseListener* listener);
  ~SharedJSContextWrapper() override = default;

  virtual void Def() override;
  virtual void EnsureConsole(
      std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
      const tasm::PageOptions& page_options) override;

  void initGlobal(std::shared_ptr<runtime::js::Runtime>& rt,
                  std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;

  void AddLifecycleListener(
      std::unique_ptr<RuntimeLifecycleListenerDelegate> listener) override;
  runtime::js::NapiEnvironment* GetNapiEnvironment() override {
#if ENABLE_NAPI_BINDING
    return napi_environment_.get();
#else
    return nullptr;
#endif
  };

 protected:
  void InitNapi(std::shared_ptr<runtime::js::Runtime>& js_runtime) override;
  std::shared_ptr<runtime::js::SharedContextGlobal> global_;
  std::string group_id_;
  ReleaseListener* listener_;
#if ENABLE_NAPI_BINDING
  std::unique_ptr<runtime::js::NapiEnvironment> napi_environment_;
  std::unique_ptr<RuntimeLifecycleObserverImpl> lifecycle_observer_;
#endif
};

class LYNX_EXPORT_FOR_DEVTOOL NoneSharedJSContextWrapper
    : public JSContextWrapper {
 public:
  NoneSharedJSContextWrapper(std::shared_ptr<runtime::js::JSIContext>);
  NoneSharedJSContextWrapper(std::shared_ptr<runtime::js::JSIContext>,
                             SharedJSContextWrapper::ReleaseListener* listener);
  ~NoneSharedJSContextWrapper() = default;

  virtual void Def() override;
  virtual void EnsureConsole(
      std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
      const tasm::PageOptions& page_options) override;

  void initGlobal(std::shared_ptr<runtime::js::Runtime>& js_runtime,
                  std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;

 protected:
  std::shared_ptr<runtime::js::SingleGlobal> global_;
  SharedJSContextWrapper::ReleaseListener* listener_ = nullptr;
};

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JS_CONTEXT_WRAPPER_H_
