// Copyright 2019 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include <jni.h>

#include <memory>
#include <mutex>

#include "core/runtime/common/napi/napi_runtime_proxy_v8.h"
#include "core/runtime/js/jsi/v8/v8_api.h"
#include "core/runtime/js/lynx_runtime_helper.h"

#if ENABLE_NAPI_BINDING
extern void RegisterV8RuntimeProxyFactory(
    lynx::runtime::js::NapiRuntimeProxyV8Factory *);
#endif

namespace lynx {
namespace runtime {

class LynxRuntimeHelperV8 : public runtime::LynxRuntimeHelper {
 public:
  std::unique_ptr<runtime::js::Runtime> MakeRuntime() override {
    return runtime::js::makeV8Runtime();
  }
  std::shared_ptr<profile::V8RuntimeProfilerWrapper> MakeRuntimeProfiler(
      std::shared_ptr<runtime::js::JSIContext> js_context) override {
    return runtime::js::makeV8RuntimeProfiler(js_context);
  }
};
extern void RegisterExternalRuntimeHelper(LynxRuntimeHelper *);
extern "C" __attribute__((visibility("default"))) int JNI_OnLoad(JavaVM *vm,
                                                                 void *) {
#if ENABLE_NAPI_BINDING
  static runtime::js::NapiRuntimeProxyV8FactoryImpl factory;
  LOGI("Setting napi factory from external");
  RegisterV8RuntimeProxyFactory(&factory);
#endif

  static LynxRuntimeHelperV8 instance;
  // register the helper
  RegisterExternalRuntimeHelper(&instance);

  return JNI_VERSION_1_6;
}
}  // namespace runtime
}  // namespace lynx
