// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/darwin/macos/lynx_explorer/LynxExplorer/runtime/ExampleLynxRuntimeLifecycleObserver.h"

#ifdef USE_PRIMJS_NAPI
#include "third_party/napi/include/primjs_napi_defines.h"
#endif

namespace lynx {
namespace example {

napi_value TestGlobalJSB(napi_env env, napi_callback_info info) {
  const char* test_script = "console.log('hello, napi.')";
  napi_value r;
  env->napi_run_script(env, test_script, strlen(test_script), "", &r);
  return r;
}

void ExampleLynxRuntimeLifecycleObserver::OnRuntimeAttach(napi_env napi_env) {
  napi_handle_scope scope = nullptr;
  napi_env->napi_open_handle_scope(napi_env, &scope);
  napi_value global;
  napi_env->napi_get_global(napi_env, &global);
  napi_value func;
  napi_env->napi_create_function(napi_env, "testGlobalJSB", 1, &TestGlobalJSB, 0, &func);
  // Inject testGlobalJSB to global object.
  // call globalThis.testGlobalJSB() in js.
  napi_env->napi_set_named_property(napi_env, global, "testGlobalJSB", func);
  napi_env->napi_close_handle_scope(napi_env, scope);
}

void ExampleLynxRuntimeLifecycleObserver::OnRuntimeDetach() {}

}  // namespace example
}  // namespace lynx
