// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_H_
#define CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_H_

#include <memory>
#include <string>
#include <utility>

#include "core/base/lynx_export.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "third_party/binding/napi/shim/shim_napi.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace piper {

class NapiRuntimeProxyV8Factory;
class NapiRuntimeProxyQuickjsFactory;
class NapiRuntimeProxyJSVMFactory;

class DelegateObserver {
 public:
  DelegateObserver(runtime::TemplateDelegate* delegate) : delegate_(delegate) {}
  void PostJSTask(base::closure closure) {
    delegate_->RunOnJSThread(std::move(closure));
  }

 private:
  runtime::TemplateDelegate* delegate_;
};

class LYNX_EXPORT NapiRuntimeProxyInterface {
 public:
  virtual ~NapiRuntimeProxyInterface() = default;
  virtual void Attach() = 0;
  virtual void Detach() = 0;
  virtual Napi::Env Env() = 0;
  virtual void SetJSRuntime(std::shared_ptr<Runtime> runtime) = 0;
  virtual std::weak_ptr<Runtime> GetJSRuntime() = 0;
  virtual void SetupLoader() = 0;
  virtual void RemoveLoader() = 0;
  virtual void SetUncaughtExceptionHandler() = 0;
};

class LYNX_EXPORT NapiRuntimeProxy : public NapiRuntimeProxyInterface {
 public:
  static std::unique_ptr<NapiRuntimeProxy> Create(
      std::shared_ptr<Runtime> runtime,
      runtime::TemplateDelegate* delegate = nullptr);
  NapiRuntimeProxy(runtime::TemplateDelegate* delegate);
  virtual ~NapiRuntimeProxy();

  void Attach() override;
  void Detach() override;

  Napi::Env Env() override { return env_; }
  void SetJSRuntime(std::shared_ptr<Runtime> runtime) override {
    js_runtime_ = runtime;
  }

  std::weak_ptr<Runtime> GetJSRuntime() override { return js_runtime_; }

  static void SetFactory(NapiRuntimeProxyV8Factory* factory);
  static void SetQuickjsFactory(NapiRuntimeProxyQuickjsFactory* factory);
  static void SetJSVMRuntimeProxyFactory(NapiRuntimeProxyJSVMFactory* factory);

  void SetupLoader() override;
  void RemoveLoader() override;

  void SetUncaughtExceptionHandler() override;

 protected:
  Napi::Env env_;
  std::shared_ptr<DelegateObserver> delegate_observer_;
  std::weak_ptr<Runtime> js_runtime_;
  std::string loader_;

 private:
  static NapiRuntimeProxyV8Factory* s_factory;
  static NapiRuntimeProxyJSVMFactory* s_jsvm_factory;
};

// A decorator for NapiRuntimeProxy, used to provide a restricted napi_env
// (disabling capabilities like napi_run_script and napi_get_global) to external
// users
class RestrictedNapiRuntimeProxyDecorator : public NapiRuntimeProxyInterface {
 public:
  RestrictedNapiRuntimeProxyDecorator(
      std::unique_ptr<NapiRuntimeProxyInterface> proxy)
      : proxy_(std::move(proxy)) {}
  ~RestrictedNapiRuntimeProxyDecorator() = default;
  void Attach() override { proxy_->Attach(); }
  void Detach() override { proxy_->Detach(); }
  Napi::Env Env() override { return proxy_->Env(); }
  void SetJSRuntime(std::shared_ptr<Runtime> runtime) override {
    proxy_->SetJSRuntime(std::move(runtime));
  }
  std::weak_ptr<Runtime> GetJSRuntime() override {
    return proxy_->GetJSRuntime();
  }
  void SetupLoader() override;
  void RemoveLoader() override;
  void SetUncaughtExceptionHandler() override {
    proxy_->SetUncaughtExceptionHandler();
  }
  Napi::Object GetGlobal();

 private:
  typedef napi_status (*NapiGetGlobalFunc)(napi_env env, napi_value* result);
  std::unique_ptr<NapiRuntimeProxyInterface> proxy_;
  std::string loader_;
  NapiGetGlobalFunc get_global_func_{nullptr};
};

}  // namespace piper
}  // namespace lynx

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_undefs.h"
#endif

#endif  // CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_H_
