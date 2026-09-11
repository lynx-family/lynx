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
#include "core/base/memory/unsafe_owning_ptr.h"
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
      base::UnsafeOwningPtr<runtime::js::Runtime>& js_runtime,
      std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
      const tasm::PageOptions& page_options) = 0;
  virtual void AddLifecycleListener(
      std::unique_ptr<RuntimeLifecycleListenerDelegate> listener){};
  virtual runtime::js::NapiEnvironment* GetNapiEnvironment() {
    return nullptr;
  };

  bool isGlobalInited() { return global_inited_; }
  bool isJSCoreLoaded() { return js_core_loaded_; }

  // Evaluate all scripts from `js_preload` if corejs hasn't been loaded for
  // this context wrapper yet. If `/lynx_core.js` is present in the list, this
  // method will also update the `js_core_loaded_` state.
  virtual void EnsureCoreJSLoaded(
      runtime::js::Runtime& js_runtime,
      std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
          js_preload);
  void prepareJSEnv(
      base::UnsafeWeakPtr<runtime::js::Runtime> js_runtime,
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
  virtual void InitNapi(base::UnsafeWeakPtr<runtime::js::Runtime> js_runtime){};
  std::weak_ptr<runtime::js::JSIContext> js_context_;
  // Whether we've run `prepareJSEnv()` once for this context wrapper.
  // This is different from `js_core_loaded_` because corejs might be deferred.
  bool js_env_prepared_;
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

  void initGlobal(base::UnsafeOwningPtr<runtime::js::Runtime>& rt,
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
  void InitNapi(base::UnsafeWeakPtr<runtime::js::Runtime> js_runtime) override;
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

  void initGlobal(base::UnsafeOwningPtr<runtime::js::Runtime>& js_runtime,
                  std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;

 protected:
  std::shared_ptr<runtime::js::SingleGlobal> global_;
  SharedJSContextWrapper::ReleaseListener* listener_ = nullptr;
};

// -------- New "shared Isolate/VM + per-page isolated Context" scheme --------
// The two wrappers below are dedicated to the opt-in new-share-group scheme and
// are intentionally kept separate from the legacy Shared/NoneShared wrappers so
// the legacy refcount-based teardown (JSIContext use_count() checks) is not
// reused here.

// Global context of a new share group. It owns the group's global runtime (the
// one that created the shared VM) and runs lynx_core.js exactly once. It does
// NOT install napi (per-page contexts reach their own runtime hooks through the
// page globalThis passed to loadCard) and is NOT registered as its own
// context's release observer; RuntimeManager tears it down explicitly once the
// group's last page is gone.
class LYNX_EXPORT_FOR_DEVTOOL NewShareGroupGlobalContextWrapper
    : public JSContextWrapper {
 public:
  NewShareGroupGlobalContextWrapper(
      std::shared_ptr<runtime::js::JSIContext> context,
      const std::string& group_id);
  ~NewShareGroupGlobalContextWrapper() override;

  // The global context is torn down explicitly by RuntimeManager, never through
  // the JSIContext release-observer path, so Def() is a no-op.
  void Def() override {}
  void EnsureCoreJSLoaded(
      runtime::js::Runtime& js_runtime,
      std::vector<std::pair<std::string, std::shared_ptr<runtime::js::Buffer>>>&
          js_preload) override;
  void EnsureConsole(
      std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
      const tasm::PageOptions& page_options) override;
  // Takes ownership of `rt` (keeping the shared VM + global context alive for
  // the whole group) and installs the full set of shared host objects so
  // per-page contexts can reference them.
  void initGlobal(base::UnsafeOwningPtr<runtime::js::Runtime>& rt,
                  std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;

  // The shared VM every page context in this group is created on.
  std::shared_ptr<runtime::js::VMInstance> GetVM();
  // The global runtime, used to read corejs exports when copying them onto a
  // page context. Owned by this wrapper via `owned_global_runtime_`.
  runtime::js::Runtime* GetGlobalRuntime() {
    return owned_global_runtime_.get();
  }
  void CopyGlobalsTo(runtime::js::Runtime& page_runtime);

  void IncLivePageCount() { ++live_page_count_; }
  // Returns the remaining live page count after the decrement.
  int DecLivePageCount() { return --live_page_count_; }

 private:
  // Owns the global runtime (and therefore the shared VM + global context),
  // keeping the group alive until RuntimeManager releases this wrapper.
  base::UnsafeOwningPtr<runtime::js::Runtime> owned_global_runtime_;
  base::UnsafeOwningPtr<runtime::js::SingleGlobal> global_;
  std::string group_id_;
  int live_page_count_ = 0;
};

// Per-page isolated context of a new share group. The page runtime owns the
// context; this wrapper only observes release to notify RuntimeManager so the
// group's live page count can be decremented. It installs page-local globals
// WITHOUT the shared host objects (copied from the global context instead) and
// WITHOUT napi.
class LYNX_EXPORT_FOR_DEVTOOL NewShareGroupPageContextWrapper
    : public JSContextWrapper {
 public:
  NewShareGroupPageContextWrapper(
      std::shared_ptr<runtime::js::JSIContext> context,
      const std::string& group_id,
      SharedJSContextWrapper::ReleaseListener* listener);
  ~NewShareGroupPageContextWrapper() override = default;

  // A page context is 1:1 with its page runtime, so releasing it always means
  // this page is gone; unconditionally notify the listener to decrement the
  // group's page count.
  void Def() override;
  void EnsureConsole(
      std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
      const tasm::PageOptions& page_options) override;
  // Installs page-local globals but skips the shared host objects; they are
  // copied by reference from the group's global context instead.
  void initGlobal(base::UnsafeOwningPtr<runtime::js::Runtime>& js_runtime,
                  std::shared_ptr<runtime::js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;

 private:
  base::UnsafeOwningPtr<runtime::js::SingleGlobal> global_;
  std::string group_id_;
  SharedJSContextWrapper::ReleaseListener* listener_ = nullptr;
};

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JS_CONTEXT_WRAPPER_H_
