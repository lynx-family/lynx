// Copyright 2024 The Lynx Authors. All rights reserved.
// Licensed under the Apache License Version 2.0 that can be found in the
// LICENSE file in the root directory of this source tree.
#ifndef CORE_RUNTIME_JSI_JSVM_JSVM_UTIL_H_
#define CORE_RUNTIME_JSI_JSVM_JSVM_UTIL_H_

#include <ark_runtime/jsvm.h>

#include <cstdlib>

#include "base/include/log/logging.h"
#include "core/runtime/jsi/jsvm/jsvm_dyn_load.h"
#include "core/runtime/jsi/jsvm/jsvm_helper.h"

namespace lynx {
namespace piper {
#define LYNX_GET_FIRST_ARG(first, ...) first

#define PRINT_EXCEPTION_INFO(jsvm_env)                                        \
  bool isPending = false;                                                     \
  DynamicLoader::GetFuncTable()->OH_JSVM_IsExceptionPending(jsvm_env,         \
                                                            &isPending);      \
  if (isPending) {                                                            \
    JSVM_Value error;                                                         \
    DynamicLoader::GetFuncTable()->OH_JSVM_GetAndClearLastException(jsvm_env, \
                                                                    &error);  \
    JSVM_Value stack;                                                         \
    DynamicLoader::GetFuncTable()->OH_JSVM_GetNamedProperty(jsvm_env, error,  \
                                                            "stack", &stack); \
    char stack_str[256];                                                      \
    DynamicLoader::GetFuncTable()->OH_JSVM_GetValueStringUtf8(                \
        jsvm_env, stack, stack_str, 256, nullptr);                            \
    LOGE("JSVM error stack: " << stack_str);                                  \
    JSVM_Value message;                                                       \
    DynamicLoader::GetFuncTable()->OH_JSVM_GetNamedProperty(                  \
        jsvm_env, error, "message", &message);                                \
    char message_str[256];                                                    \
    DynamicLoader::GetFuncTable()->OH_JSVM_GetValueStringUtf8(                \
        jsvm_env, message, message_str, 256, nullptr);                        \
    LOGE("JSVM error message: " << message_str);                              \
  }

#define JSVM_CALL_NO_ENV(name, ...)                                     \
  do {                                                                  \
    auto status = (DynamicLoader::GetFuncTable()) -> name(__VA_ARGS__); \
    if (status != JSVM_Status::JSVM_OK) {                               \
      LOGE("jsvm call failed status:" << status);                       \
    }                                                                   \
  } while (0)

#define JSVM_CALL(name, ...)                                            \
  do {                                                                  \
    auto status = (DynamicLoader::GetFuncTable()) -> name(__VA_ARGS__); \
    if (status != JSVM_Status::JSVM_OK) {                               \
      LOGE("jsvm call failed status:" << status);                       \
      auto jsvm_env = LYNX_GET_FIRST_ARG(__VA_ARGS__);                  \
      PRINT_EXCEPTION_INFO(jsvm_env);                                   \
    }                                                                   \
  } while (0)

#define JSVM_CALL_RETURN(name, ret, ...)                                \
  do {                                                                  \
    auto status = (DynamicLoader::GetFuncTable()) -> name(__VA_ARGS__); \
    if (status != JSVM_Status::JSVM_OK) {                               \
      LOGE("jsvm call failed status:" << status);                       \
      auto jsvm_env = LYNX_GET_FIRST_ARG(__VA_ARGS__);                  \
      PRINT_EXCEPTION_INFO(jsvm_env);                                   \
      return ret;                                                       \
    }                                                                   \
  } while (0)

class HandleScopeWrapper {
 public:
  explicit HandleScopeWrapper(JSVM_Env env) : env(env) {
    JSVM_CALL(OH_JSVM_OpenHandleScope, env, &handleScope);
  }

  ~HandleScopeWrapper() {
    JSVM_CALL(OH_JSVM_CloseHandleScope, env, handleScope);
  }

  HandleScopeWrapper(const HandleScopeWrapper&) = delete;
  HandleScopeWrapper& operator=(const HandleScopeWrapper&) = delete;
  HandleScopeWrapper(HandleScopeWrapper&&) = delete;
  void* operator new(size_t) = delete;
  void* operator new[](size_t) = delete;

 protected:
  JSVM_Env env;
  JSVM_HandleScope handleScope;
};
}  // namespace piper
}  // namespace lynx

#endif  // CORE_RUNTIME_JSI_JSVM_JSVM_UTIL_H_
