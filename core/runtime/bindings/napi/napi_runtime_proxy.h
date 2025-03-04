// Copyright 2021 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#ifndef CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_H_
#define CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_H_

#include <memory>
#include <string>
#include <utility>

#include "base/include/base_export.h"
#include "core/runtime/jsi/jsi.h"
#include "core/runtime/piper/js/template_delegate.h"
#include "third_party/binding/napi/shim/shim_napi.h"

namespace lynx {
namespace piper {

class NapiRuntimeProxyV8Factory;
class NapiRuntimeProxyQuickjsFactory;

class DelegateObserver {
 public:
  DelegateObserver(runtime::TemplateDelegate* delegate) : delegate_(delegate) {}
  void PostJSTask(base::closure closure) {
    delegate_->RunOnJSThread(std::move(closure));
  }

 private:
  runtime::TemplateDelegate* delegate_;
};

class BASE_EXPORT NapiRuntimeProxy {
 public:
  static std::unique_ptr<NapiRuntimeProxy> Create(
      std::shared_ptr<Runtime> runtime,
      runtime::TemplateDelegate* delegate = nullptr);
  NapiRuntimeProxy(runtime::TemplateDelegate* delegate);
  virtual ~NapiRuntimeProxy();

  virtual void Attach();
  virtual void Detach();

  Napi::Env Env() { return env_; }
  void SetJSRuntime(std::shared_ptr<Runtime> runtime) { js_runtime_ = runtime; }

  std::weak_ptr<Runtime> GetJSRuntime() { return js_runtime_; }

  static void SetFactory(NapiRuntimeProxyV8Factory* factory);
  static void SetQuickjsFactory(NapiRuntimeProxyQuickjsFactory* factory);

  void SetupLoader();
  void RemoveLoader();

  void SetUncaughtExceptionHandler();

 protected:
  Napi::Env env_;
  std::shared_ptr<DelegateObserver> delegate_observer_;
  std::weak_ptr<Runtime> js_runtime_;
  std::string loader_;

 private:
  static NapiRuntimeProxyV8Factory* s_factory;
};

}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_BINDINGS_NAPI_NAPI_RUNTIME_PROXY_H_
