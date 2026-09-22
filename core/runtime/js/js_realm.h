// Copyright 2023 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JS_JS_REALM_H_
#define CORE_RUNTIME_JS_JS_REALM_H_

#include <memory>
#include <string>
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

// A JS context and its global environment. Shared realms are owned by the
// thread-local JSRealmManager; page-local realms are owned by JSExecutor.
// Destruction is explicit and never inferred from JSIContext reference counts.
class LYNX_EXPORT_FOR_DEVTOOL JSRealm {
 public:
  explicit JSRealm(std::shared_ptr<js::JSIContext> context);
  virtual ~JSRealm();

  void EnsureConsole(std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                     const tasm::PageOptions& page_options);
  virtual void InitGlobal(base::UnsafeOwningPtr<js::Runtime>& runtime,
                          std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                          const tasm::PageOptions& page_options) = 0;
  // Embedder NAPI notifications; these listeners do not own or release realms.
  virtual void AddLifecycleListener(
      std::unique_ptr<RuntimeLifecycleListenerDelegate> listener) {}
  virtual js::NapiEnvironment* GetNapiEnvironment() { return nullptr; }

  bool IsCoreJSLoaded() const { return js_core_loaded_; }
  virtual void EnsureCoreJSLoaded(
      js::Runtime& runtime,
      std::vector<std::pair<std::string, std::shared_ptr<js::Buffer>>>&
          sources);
  void PrepareJSEnv(
      base::UnsafeWeakPtr<js::Runtime> runtime,
      std::vector<std::pair<std::string, std::shared_ptr<js::Buffer>>>&
          sources);
  const std::shared_ptr<js::JSIContext>& GetJSContext() const {
    return js_context_;
  }
#if ENABLE_TRACE_PERFETTO
  void SetRuntimeProfiler(std::shared_ptr<profile::RuntimeProfiler> profiler);
#endif

 protected:
  virtual void InitNapi(base::UnsafeWeakPtr<js::Runtime> runtime) {}
  void InitGlobalObject(base::UnsafeOwningPtr<js::Runtime>& runtime,
                        std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                        const tasm::PageOptions& page_options,
                        bool install_shared_host_objects);

  // JSI contexts remain shared with backend runtimes and profiler interfaces.
  std::shared_ptr<js::JSIContext> js_context_;
  // Shared realms own a dedicated runtime. Page-local realms borrow the
  // executor's runtime through SingleGlobal's UnsafeWeakPtr.
  base::UnsafeOwningPtr<js::Runtime> owned_global_runtime_;
  base::UnsafeOwningPtr<js::SingleGlobal> global_;
  bool js_env_prepared_ = false;
  bool js_core_loaded_ = false;
  bool global_inited_ = false;
#if ENABLE_TRACE_PERFETTO
  // RuntimeProfilerManager accesses profilers from tracing threads.
  std::shared_ptr<profile::RuntimeProfiler> runtime_profiler_;
#endif
};

// One executor's binding to a realm. A shared context still has a distinct
// page runtime per executor, so that runtime cannot live in the shared realm.
// Keep the local realm after the runtime so its globals/profiler die first.
struct JSRealmState {
  enum class Sharing { kNone, kContext, kVM };

  base::UnsafeOwningPtr<js::Runtime> runtime;
  base::UnsafeOwningPtr<JSRealm> local_realm;
  Sharing sharing;
};

class LYNX_EXPORT_FOR_DEVTOOL SharedJSRealm : public JSRealm {
 public:
  explicit SharedJSRealm(std::shared_ptr<js::JSIContext> context);
  ~SharedJSRealm() override;

  void InitGlobal(base::UnsafeOwningPtr<js::Runtime>& runtime,
                  std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;
  void AddLifecycleListener(
      std::unique_ptr<RuntimeLifecycleListenerDelegate> listener) override;
  js::NapiEnvironment* GetNapiEnvironment() override {
#if ENABLE_NAPI_BINDING
    return napi_environment_.get();
#else
    return nullptr;
#endif
  }

 protected:
  void InitNapi(base::UnsafeWeakPtr<js::Runtime> runtime) override;
#if ENABLE_NAPI_BINDING
  std::unique_ptr<js::NapiEnvironment> napi_environment_;
  std::unique_ptr<RuntimeLifecycleObserverImpl> lifecycle_observer_;
#endif
};

class LYNX_EXPORT_FOR_DEVTOOL SingleJSRealm : public JSRealm {
 public:
  explicit SingleJSRealm(std::shared_ptr<js::JSIContext> context);
  void InitGlobal(base::UnsafeOwningPtr<js::Runtime>& runtime,
                  std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;
};

// Runs corejs once for a shared VM with isolated page contexts. The manager
// keeps this realm alive until every page runtime in the group is destroyed.
class LYNX_EXPORT_FOR_DEVTOOL SharedVMGlobalRealm : public JSRealm {
 public:
  SharedVMGlobalRealm(std::shared_ptr<js::JSIContext> context,
                      const std::string& group_id);
  void EnsureCoreJSLoaded(
      js::Runtime& runtime,
      std::vector<std::pair<std::string, std::shared_ptr<js::Buffer>>>& sources)
      override;
  void InitGlobal(base::UnsafeOwningPtr<js::Runtime>& runtime,
                  std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;
  std::shared_ptr<js::VMInstance> GetVM();
  js::Runtime* GetGlobalRuntime() { return owned_global_runtime_.get(); }
  void CopyGlobalsTo(js::Runtime& page_runtime);

 private:
  std::string group_id_;
};

// Page-local globals in a shared VM. Shared host objects and corejs exports
// are copied from SharedVMGlobalRealm; NAPI is owned by the page runtime shell.
class LYNX_EXPORT_FOR_DEVTOOL SharedVMPageRealm : public JSRealm {
 public:
  explicit SharedVMPageRealm(std::shared_ptr<js::JSIContext> context);
  void InitGlobal(base::UnsafeOwningPtr<js::Runtime>& runtime,
                  std::shared_ptr<js::ConsoleMessagePostMan> post_man,
                  const tasm::PageOptions& page_options) override;
};

}  // namespace runtime
}  // namespace lynx
#endif  // CORE_RUNTIME_JS_JS_REALM_H_
