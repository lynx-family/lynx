// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_PIPER_JS_JS_CONTEXT_WRAPPER_H_
#define CORE_RUNTIME_PIPER_JS_JS_CONTEXT_WRAPPER_H_

#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/include/base_export.h"
#include "core/runtime/bindings/jsi/global.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/profile/runtime_profiler.h"

namespace lynx {
namespace runtime {

class BASE_EXPORT_FOR_DEVTOOL JSContextWrapper
    : public piper::JSIContext::Observer,
      public std::enable_shared_from_this<JSContextWrapper> {
 public:
  JSContextWrapper(std::shared_ptr<piper::JSIContext>);
  ~JSContextWrapper() = default;

  virtual void Def() = 0;
  virtual void EnsureConsole(
      std::shared_ptr<piper::ConsoleMessagePostMan> post_man) = 0;
  virtual void initGlobal(
      std::shared_ptr<piper::Runtime>& js_runtime,
      std::shared_ptr<piper::ConsoleMessagePostMan> post_man) = 0;

  bool isGlobalInited() { return global_inited_; }
  bool isJSCoreLoaded() { return js_core_loaded_; }
  void loadPreJS(std::weak_ptr<piper::Runtime> js_runtime,
                 std::vector<std::pair<std::string, std::string>>& js_preload);
  std::shared_ptr<piper::JSIContext> getJSContext() {
    return js_context_.lock();
  }
#if ENABLE_TRACE_PERFETTO
  void SetRuntimeProfiler(
      std::shared_ptr<profile::RuntimeProfiler> runtime_profiler);
#endif
 protected:
  std::weak_ptr<piper::JSIContext> js_context_;
  bool js_core_loaded_;
  bool global_inited_;
#if ENABLE_TRACE_PERFETTO
  std::shared_ptr<profile::RuntimeProfiler> runtime_profiler_;
#endif
};

class BASE_EXPORT_FOR_DEVTOOL SharedJSContextWrapper : public JSContextWrapper {
 public:
  class ReleaseListener {
   public:
    virtual void OnRelease(const std::string& group_id) = 0;
    virtual ~ReleaseListener() = default;
  };
  SharedJSContextWrapper(std::shared_ptr<piper::JSIContext>,
                         const std::string& group_id,
                         ReleaseListener* listener);
  ~SharedJSContextWrapper() = default;

  virtual void Def() override;
  virtual void EnsureConsole(
      std::shared_ptr<piper::ConsoleMessagePostMan> post_man) override;

  void initGlobal(
      std::shared_ptr<piper::Runtime>& rt,
      std::shared_ptr<piper::ConsoleMessagePostMan> post_man) override;

 protected:
  std::shared_ptr<piper::SharedContextGlobal> global_;
  std::string group_id_;
  ReleaseListener* listener_;
};

class BASE_EXPORT_FOR_DEVTOOL NoneSharedJSContextWrapper
    : public JSContextWrapper {
 public:
  NoneSharedJSContextWrapper(std::shared_ptr<piper::JSIContext>);
  NoneSharedJSContextWrapper(std::shared_ptr<piper::JSIContext>,
                             SharedJSContextWrapper::ReleaseListener* listener);
  ~NoneSharedJSContextWrapper() = default;

  virtual void Def() override;
  virtual void EnsureConsole(
      std::shared_ptr<piper::ConsoleMessagePostMan> post_man) override;

  void initGlobal(
      std::shared_ptr<piper::Runtime>& js_runtime,
      std::shared_ptr<piper::ConsoleMessagePostMan> post_man) override;

 protected:
  std::shared_ptr<piper::SingleGlobal> global_;
  SharedJSContextWrapper::ReleaseListener* listener_ = nullptr;
};

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_PIPER_JS_JS_CONTEXT_WRAPPER_H_
