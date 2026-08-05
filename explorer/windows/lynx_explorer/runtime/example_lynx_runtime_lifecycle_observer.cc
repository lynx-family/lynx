// Copyright 2025 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.

#include "explorer/windows/lynx_explorer/runtime/example_lynx_runtime_lifecycle_observer.h"

#include <cstring>

#include "explorer/cpp/LynxNodeAPI.h"

#ifdef USE_WEAK_SUFFIX_NAPI
#include "third_party/weak-node-api/headers/weak_napi_defines.h"
#endif

namespace lynx {
namespace example {

napi_value TestGlobalJSB(napi_env env, napi_callback_info info) {
  const char* test_script = "console.log('hello, napi.')";
  napi_value script;
  napi_create_string_utf8(env, test_script, std::strlen(test_script), &script);
  napi_value r;
  napi_run_script(env, script, &r);
  return r;
}

void ExampleLynxRuntimeLifecycleObserver::OnRuntimeAttach(napi_env napi_env) {
  napi_handle_scope scope = nullptr;
  napi_open_handle_scope(napi_env, &scope);
  napi_value global;
  napi_get_global(napi_env, &global);
  napi_value func;
  napi_create_function(napi_env, "testGlobalJSB", 1, &TestGlobalJSB, 0, &func);
  // Inject testGlobalJSB to global object.
  // call globalThis.testGlobalJSB() in js.
  napi_set_named_property(napi_env, global, "testGlobalJSB", func);
  napi_close_handle_scope(napi_env, scope);

  if (token_id_ != 0) {
    lynx::explorer::LynxNodeAPI::GetInstance().PutEnvByToken(token_id_,
                                                             napi_env);
  }
}

void ExampleLynxRuntimeLifecycleObserver::OnRuntimeDetach() {
  if (token_id_ != 0) {
    lynx::explorer::LynxNodeAPI::GetInstance().RemoveEnvByToken(token_id_);
  }
}

}  // namespace example
}  // namespace lynx
